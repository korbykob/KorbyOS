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
    EFI_GRAPHICS_OUTPUT_BLT_PIXEL white;
    white.Red = 255;
    white.Green = 255;
    white.Blue = 255;
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
                        
                        colour.Red = 0;
                        colour.Green = 0;
                        colour.Blue = 0;
                    }
                    else
                    {
                        colour.Red = 255;
                        colour.Green = 255;
                        colour.Blue = 255;
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