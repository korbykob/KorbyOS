#include "main.h"

uint8_t mouseIcon[] = {
    00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,
    00,01,01,01,01,01,01,01,01,01,01,01,01,01,00,02,
    00,01,01,01,01,01,01,01,01,01,01,01,01,00,02,02,
    00,01,01,01,01,01,01,01,01,01,01,01,00,02,02,02,
    00,01,01,01,01,01,01,01,01,01,01,00,02,02,02,02,
    00,01,01,01,01,01,01,01,01,01,00,02,02,02,02,02,
    00,01,01,01,01,01,01,01,01,00,02,02,02,02,02,02,
    00,01,01,01,01,01,01,01,01,01,00,02,02,02,02,02,
    00,01,01,01,01,01,01,01,01,01,01,00,02,02,02,02,
    00,01,01,01,01,01,00,01,01,01,01,01,00,02,02,02,
    00,01,01,01,01,00,02,00,01,01,01,01,01,00,02,02,
    00,01,01,01,00,02,02,02,00,01,01,01,01,01,00,02,
    00,01,01,00,02,02,02,02,02,00,01,01,01,01,01,00,
    00,01,00,02,02,02,02,02,02,02,00,01,01,01,00,02,
    00,00,02,02,02,02,02,02,02,02,02,00,01,00,02,02,
    00,02,02,02,02,02,02,02,02,02,02,02,00,02,02,02,
};

void start()
{
    EFI_GRAPHICS_OUTPUT_BLT_PIXEL normal;
    normal.Red = 152;
    normal.Green = 152;
    normal.Blue = 152;
    drawString(L"Welcome to ", GOP->Mode->Info->HorizontalResolution / 2 - 152, GOP->Mode->Info->VerticalResolution / 2 - 32, normal);
    EFI_GRAPHICS_OUTPUT_BLT_PIXEL white;
    white.Red = 255;
    white.Green = 255;
    white.Blue = 255;
    drawString(L"KorbyOS", GOP->Mode->Info->HorizontalResolution / 2 + 24, GOP->Mode->Info->VerticalResolution / 2 - 32, white);
    drawString(L"!", GOP->Mode->Info->HorizontalResolution / 2 + 136, GOP->Mode->Info->VerticalResolution / 2 - 32, normal);
    drawString(L"Press any key to continue...", GOP->Mode->Info->HorizontalResolution / 2 - 224, GOP->Mode->Info->VerticalResolution / 2, normal);
    blit();
    waitForKey();
    EFI_GRAPHICS_OUTPUT_BLT_PIXEL black;
    black.Red = 0;
    black.Green = 0;
    black.Blue = 0;
    while (1)
    {
        uint64_t* to = (uint64_t*)videoBuffer;
        uint64_t* from = (uint64_t*)wallpaper;
        for (uint64_t i = 0; i < (GOP->Mode->Info->HorizontalResolution * GOP->Mode->Info->VerticalResolution) / 2; i++)
        {
            *to++ = *from++;
        }
        EFI_GRAPHICS_OUTPUT_BLT_PIXEL* address = videoBuffer + mouseY * GOP->Mode->Info->HorizontalResolution + mouseX;
        uint8_t* buffer = mouseIcon;
        for (uint32_t y = 0; y < 16; y++)
        {
            for (uint32_t x = 0; x < 16; x++)
            {
                EFI_GRAPHICS_OUTPUT_BLT_PIXEL colour;
                if (*buffer != 2)
                {
                    if (*buffer == 0)
                    {
                        colour = black;
                    }
                    else
                    {
                        colour = white;
                    }
                    *address = colour;
                }
                buffer++;
                address++;
            }
            address += GOP->Mode->Info->HorizontalResolution - 16;
        }
        waitForPit();
        blit();
    }
}