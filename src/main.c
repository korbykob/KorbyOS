#include "main.h"

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
    BOOLEAN flash = FALSE;
    while (1)
    {
        uint8_t colour = flash ? 0xFF : 0;
        flash = !flash;
        uint8_t* address = (uint8_t*)videoBuffer;
        for (UINTN i = 0; i < GOP->Mode->FrameBufferSize; i++)
        {
            *address++ = colour;
        }
        waitForPit();
        blit();
    }
}