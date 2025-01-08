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

uint64_t syscallHandle(uint64_t code, uint64_t arg)
{
    switch (code)
    {
        case 0:
            return (uint64_t)alloc(arg);
            break;
    }
    return 0;
}

void keyPress(uint8_t scancode, BOOLEAN unpressed)
{
    if (!unpressed && scancode == 91)
    {
        mainButtonActivated = !mainButtonActivated;
    }
}

void buttonClick(uint64_t id)
{
    switch (id)
    {
        case 0:
            started = TRUE;
            break;
        case 1:
            mainButtonActivated = !mainButtonActivated;
            break;
        default:
            if (programs[id - 2].running)
            {
                programs[id - 2].stop();
            }
            else
            {
                programs[id - 2].start();
            }
            programs[id - 2].running = !programs[id - 2].running;
            break;
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
        button.id = 0;
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
        struct Button mainButton;
        mainButton.x = 4;
        mainButton.y = GOP->Mode->Info->VerticalResolution - 28;
        mainButton.width = 24;
        mainButton.height = 24;
        mainButton.id = 1;
        drawRectangle(mainButton.x, mainButton.y, mainButton.width, mainButton.height, mainButtonActivated ? black : white);
        registerButton(&mainButton);
        if (mainButtonActivated)
        {
            drawRectangle(0, GOP->Mode->Info->VerticalResolution - 432, 300, 400, grey);
            drawRectangle(0, GOP->Mode->Info->VerticalResolution - 33, 300, 1, black);
        }
        drawRectangle(32, GOP->Mode->Info->VerticalResolution - 28, 1, 24, black);
        for (uint8_t i = 0; i < 1; i++)
        {
            struct Button button;
            button.x = 37 + i * 24 + i * 8;
            button.y = GOP->Mode->Info->VerticalResolution - 28;
            button.width = 24;
            button.height = 24;
            button.id = i + 2;
            drawImage(button.x, button.y, button.width, button.height, programs[i].icon);
            if (programs[i].running)
            {
                drawRectangle(button.x + 6, GOP->Mode->Info->VerticalResolution - 2, 12, 1, black);
                programs[i].update();
            }
            registerButton(&button);
        }
        drawMouse();
        waitForPit();
        blit(videoBuffer, (void*)GOP->Mode->FrameBufferBase);
        endButtons();
    }
}
