#include <efi.h>
#include <efilib.h>

EFI_GRAPHICS_OUTPUT_PROTOCOL* GOP = NULL;
EFI_GRAPHICS_OUTPUT_BLT_PIXEL* videoBuffer = NULL;
uint8_t* font = NULL;

void blit()
{
    EFI_GRAPHICS_OUTPUT_BLT_PIXEL* to = (EFI_GRAPHICS_OUTPUT_BLT_PIXEL*)GOP->Mode->FrameBufferBase;
    EFI_GRAPHICS_OUTPUT_BLT_PIXEL* from = videoBuffer;
    for (UINTN i = 0; i < GOP->Mode->FrameBufferSize; i++)
    {
        *to++ = *from++;
    }
}

void drawCharacter(CHAR16 character, uint32_t x, uint32_t y, EFI_GRAPHICS_OUTPUT_BLT_PIXEL colour)
{
    EFI_GRAPHICS_OUTPUT_BLT_PIXEL* address = videoBuffer + y * GOP->Mode->Info->HorizontalResolution + x;
    uint8_t* glyph = font + ((uint32_t*)font)[2] + ((uint32_t*)font)[5] * character;
    for (uint8_t y = 0; y < 64; y += 2)
    {
        for (uint8_t x = 0; x < 8; x++)
        {
            if (glyph[y] & (0b10000000 >> x))
            {
                *address = colour;
            }
            address++;
        }
        for (uint8_t x = 0; x < 8; x++)
        {
            if (glyph[y + 1] & (0b10000000 >> x))
            {
                *address = colour;
            }
            address++;
        }
        address += GOP->Mode->Info->HorizontalResolution - 16;
    }
}

void drawString(const CHAR16* string, uint32_t x, uint32_t y, EFI_GRAPHICS_OUTPUT_BLT_PIXEL colour)
{
    EFI_GRAPHICS_OUTPUT_BLT_PIXEL* address = videoBuffer + y * GOP->Mode->Info->HorizontalResolution + x;
    while (*string != 0)
    {
        uint8_t* glyph = font + ((uint32_t*)font)[2] + ((uint32_t*)font)[5] * *string++;
        for (uint8_t y = 0; y < 64; y += 2)
        {
            for (uint8_t x = 0; x < 8; x++)
            {
                if (glyph[y] & (0b10000000 >> x))
                {
                    *address = colour;
                }
                address++;
            }
            for (uint8_t x = 0; x < 8; x++)
            {
                if (glyph[y + 1] & (0b10000000 >> x))
                {
                    *address = colour;
                }
                address++;
            }
            address += GOP->Mode->Info->HorizontalResolution - 16;
        }
        address -= GOP->Mode->Info->HorizontalResolution * 32 - 16;
    }
}

void drawRectangle(uint32_t x, uint32_t y, uint32_t width, uint32_t height, EFI_GRAPHICS_OUTPUT_BLT_PIXEL color)
{
    EFI_GRAPHICS_OUTPUT_BLT_PIXEL* address = videoBuffer + y * GOP->Mode->Info->HorizontalResolution + x;
    for (unsigned short y = 0; y < height; y++)
    {
        for (unsigned short x = 0; x < width; x++)
        {
            *address++ = color;
        }
        address += GOP->Mode->Info->HorizontalResolution - width;
    }
}

void start();

EFI_STATUS efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE* SystemTable)
{
    InitializeLib(ImageHandle, SystemTable);
    LibLocateProtocol(&GraphicsOutputProtocol, (void**)&GOP);
    videoBuffer = AllocateZeroPool(GOP->Mode->FrameBufferSize);
    blit();
    EFI_LOADED_IMAGE* image = NULL;
    uefi_call_wrapper(BS->HandleProtocol, 3, ImageHandle, &LoadedImageProtocol, (void**)&image);
    EFI_FILE_HANDLE fs = LibOpenRoot(image->DeviceHandle);
    EFI_FILE_HANDLE file = NULL;
    uefi_call_wrapper(fs->Open, 5, fs, &file, L"font.psf", EFI_FILE_MODE_READ, EFI_FILE_READ_ONLY | EFI_FILE_HIDDEN | EFI_FILE_SYSTEM);
    EFI_FILE_INFO* info = LibFileInfo(file);
    uint64_t fontSize = info->FileSize;
    FreePool(info);
    font = AllocatePool(fontSize);
    uefi_call_wrapper(file->Read, 3, file, &fontSize, font);
    uefi_call_wrapper(file->Close, 1, file);
    UINTN entries;
    UINTN key;
    UINTN size;
    uint32_t version;
    LibMemoryMap(&entries, &key, &size, &version);
    uefi_call_wrapper(BS->ExitBootServices, 2, ImageHandle, key);
    start();
    while (1);
    return EFI_SUCCESS;
}