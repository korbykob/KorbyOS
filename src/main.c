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
BOOLEAN waitPit = FALSE;
BOOLEAN waitKey = FALSE;
uint8_t mouseCycle = 2;
int8_t mouseBytes[3];
int64_t mouseX = 0;
int64_t mouseY = 0;
BOOLEAN leftClick = FALSE;
BOOLEAN rightClick = FALSE;

__attribute__((interrupt)) void pit(struct interruptFrame* frame)
{
    waitPit = FALSE;
    outb(0x20, 0x20);
}

__attribute__((interrupt)) void keyboard(struct interruptFrame* frame)
{
    if (!(inb(0x60) & 0x80))
    {
        waitKey = FALSE;
    }
    outb(0x20, 0x20);
}

__attribute__((interrupt)) void mouse(struct interruptFrame* frame)
{
    mouseBytes[mouseCycle] = inb(0x60);
    mouseCycle++;
    if (mouseCycle == 3)
    {
        mouseCycle = 0;
        leftClick = mouseBytes[0] & 0b00000001;
        rightClick = mouseBytes[0] & 0b00000010;
        mouseX += mouseBytes[1];
        if (mouseX < 0)
        {
            mouseX = 0;
        }
        if (mouseX > GOP->Mode->Info->HorizontalResolution - 16)
        {
            mouseX = GOP->Mode->Info->HorizontalResolution - 16;
        }
        mouseY -= mouseBytes[2];
        if (mouseY < 0)
        {
            mouseY = 0;
        }
        if (mouseY > GOP->Mode->Info->VerticalResolution - 16)
        {
            mouseY = GOP->Mode->Info->VerticalResolution - 16;
        }
    }
    outb(0xA0, 0x20);
    outb(0x20, 0x20);
}

void interrupts()
{
    outb(0x64, 0xA8);
    outb(0x64, 0x20);
    uint8_t status = inb(0x60) | 2;
    outb(0x64, 0x60);
    outb(0x60, status);
    installInterrupt(0, pit);
    installInterrupt(1, keyboard);
    outb(0x64, 0xD4);
    outb(0x60, 0xF6);
    inb(0x60);
    outb(0x64, 0xD4);
    outb(0x60, 0xF4);
    inb(0x60);
    installInterrupt(12, mouse);
}

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
    waitKey = TRUE;
    while (waitKey);
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
        waitPit = TRUE;
        while (waitPit);
        blit();
    }
}