#include "main.h"

uint8_t mouseIcon[] = {
    00,00,00,02,02,02,02,02,02,02,02,02,02,02,02,02,
    00,01,00,00,00,02,02,02,02,02,02,02,02,02,02,02,
    00,01,01,01,01,00,00,00,02,02,02,02,02,02,02,02,
    00,01,01,01,01,01,01,00,00,02,02,02,02,02,02,02,
    00,00,01,01,01,01,01,01,01,00,00,00,02,02,02,02,
    00,00,01,01,01,01,01,01,01,01,00,00,02,02,02,02,
    02,00,01,01,01,01,01,01,01,01,00,02,02,02,02,02,
    02,00,01,01,01,01,01,01,01,00,02,02,02,02,02,02,
    02,00,00,01,01,01,01,00,00,00,00,02,02,02,02,02,
    02,02,00,01,00,00,00,00,01,01,00,00,02,02,02,02,
    02,02,00,00,00,02,00,00,01,01,01,00,00,00,00,02,
    02,02,02,00,02,02,02,00,00,00,01,01,01,01,00,02,
    02,02,02,02,02,02,02,02,02,02,00,00,00,01,00,02,
    02,02,02,02,02,02,02,02,02,02,02,02,00,00,00,02,
    02,02,02,02,02,02,02,02,02,02,02,02,02,00,02,02,
    02,02,02,02,02,02,02,02,02,02,02,02,02,02,02,02,
};
EFI_GRAPHICS_OUTPUT_BLT_PIXEL white = { 255, 255, 255 };
EFI_GRAPHICS_OUTPUT_BLT_PIXEL black = { 0, 0, 0 };
EFI_GRAPHICS_OUTPUT_BLT_PIXEL grey = { 128, 128, 128 };
BOOLEAN started = FALSE;
BOOLEAN mainButtonActivated = FALSE;

uint64_t syscallHandle(uint64_t code)
{
    switch (code)
    {
        case 0:
            return 1234;
            break;
    }
    return 1;
}

void keyPress(uint8_t scancode, BOOLEAN unpressed)
{
    if (!unpressed && scancode == 91)
    {
        mainButtonActivated = !mainButtonActivated;
    }
}

void drawMouse()
{
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
}

void startButtonClick()
{
    started = TRUE;
}

void mainButtonClick()
{
    mainButtonActivated = !mainButtonActivated;
}

void start()
{
    while (!started)
    {
        startButtons();
        blit(wallpaper, videoBuffer);
        drawRectangle(GOP->Mode->Info->HorizontalResolution / 2 - 160, GOP->Mode->Info->VerticalResolution / 2 - 40, 320, 48, grey);
        drawString(L"Welcome to KorbyOS!", GOP->Mode->Info->HorizontalResolution / 2 - 152, GOP->Mode->Info->VerticalResolution / 2 - 32, black);
        struct Button button;
        button.x = GOP->Mode->Info->HorizontalResolution / 2 - 48;
        button.y = GOP->Mode->Info->VerticalResolution / 2 + 24;
        button.width = 96;
        button.height = 48;
        button.action = startButtonClick;
        drawRectangle(button.x, button.y, button.width, button.height, grey);
        drawString(L"Start", GOP->Mode->Info->HorizontalResolution / 2 - 40, GOP->Mode->Info->VerticalResolution / 2 + 32, black);
        registerButton(&button);
        drawMouse();
        waitForPit();
        blit(videoBuffer, (void*)GOP->Mode->FrameBufferBase);
        endButtons();
    }
    while (1)
    {
        startButtons();
        blit(wallpaper, videoBuffer);
        drawRectangle(0, GOP->Mode->Info->VerticalResolution - 32, GOP->Mode->Info->HorizontalResolution, 32, grey);
        struct Button button;
        button.x = 4;
        button.y = GOP->Mode->Info->VerticalResolution - 28;
        button.width = 24;
        button.height = 24;
        button.action = mainButtonClick;
        drawRectangle(button.x, button.y, button.width, button.height, mainButtonActivated ? black : white);
        registerButton(&button);
        if (mainButtonActivated)
        {
            drawRectangle(0, GOP->Mode->Info->VerticalResolution - 432, 300, 400, grey);
            drawRectangle(0, GOP->Mode->Info->VerticalResolution - 33, 300, 1, black);
        }
        test();
        drawMouse();
        waitForPit();
        blit(videoBuffer, (void*)GOP->Mode->FrameBufferBase);
        endButtons();
    }
}
