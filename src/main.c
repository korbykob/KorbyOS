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
struct Window windows;

struct Window* allocateWindow(uint32_t width, uint32_t height, CHAR16* title)
{
    struct Window* window = &windows;
    while (window->next)
    {
        window = window->next;
    }
    window->next = allocate(sizeof(struct Window));
    window->next->x = GOP->Mode->Info->HorizontalResolution / 2 - width / 2;
    window->next->y = GOP->Mode->Info->VerticalResolution / 2 - height / 2;
    window->next->width = width;
    window->next->height = height;
    window->next->title = title;
    window->next->buffer = allocate(width * height * 4);
    window->next->next = NULL;
    return window->next;
}

void unallocateWindow(struct Window* window)
{
    struct Window* current = &windows;
    while (current->next != window)
    {
        current = current->next;
    }
    unallocate(current->next->buffer, current->next->width * current->next->height * 4);
    unallocate(current->next, sizeof(struct Window));
    current->next = current->next->next;
}

uint64_t syscallHandle(uint64_t code, uint64_t arg1, uint64_t arg2, uint64_t arg3)
{
    switch (code)
    {
        case 0:
            return (uint64_t)allocate(arg1);
            break;
        case 1:
            unallocate((void*)arg1, arg2);
            return 0;
            break;
        case 2:
            return (uint64_t)allocateWindow(arg1, arg2, (CHAR16*)arg3);
            break;
        case 3:
            unallocateWindow((struct Window*)arg1);
            return 0;
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
        blit(videoBuffer, (EFI_GRAPHICS_OUTPUT_BLT_PIXEL*)GOP->Mode->FrameBufferBase);
        endButtons();
    }
    while (TRUE)
    {
        startButtons();
        blit(wallpaper, videoBuffer);
        drawRectangle(0, GOP->Mode->Info->VerticalResolution - 32, GOP->Mode->Info->HorizontalResolution, 32, grey);
        if (mainButtonActivated)
        {
            drawRectangle(0, GOP->Mode->Info->VerticalResolution - 432, 300, 400, grey);
            drawRectangle(0, GOP->Mode->Info->VerticalResolution - 33, 300, 1, black);
            drawRectangle(2, GOP->Mode->Info->VerticalResolution - 30, 28, 28, black);
        }
        struct Button mainButton;
        mainButton.x = 4;
        mainButton.y = GOP->Mode->Info->VerticalResolution - 28;
        mainButton.width = 24;
        mainButton.height = 24;
        mainButton.id = 1;
        drawRectangle(mainButton.x, mainButton.y, mainButton.width, mainButton.height, white);
        registerButton(&mainButton);
        drawRectangle(32, GOP->Mode->Info->VerticalResolution - 28, 1, 24, black);
        for (uint8_t i = 0; i < 1; i++)
        {
            struct Button button;
            button.x = 37 + i * 24 + i * 8;
            button.y = GOP->Mode->Info->VerticalResolution - 28;
            button.width = 24;
            button.height = 24;
            button.id = i + 2;
            if (programs[i].running)
            {
                programs[i].update();
                drawRectangle(button.x - 2, GOP->Mode->Info->VerticalResolution - 30, 28, 28, black);
            }
            drawImage(button.x, button.y, button.width, button.height, programs[i].icon);
            registerButton(&button);
        }
        struct Window* window = &windows;
        while (TRUE)
        {
            window = window->next;
            if (window == NULL)
            {
                break;
            }
            drawRectangle(window->x, window->y, window->width + 10, window->height + 52, grey);
            drawString(window->title, window->x + 10, window->y + 10, black);
            drawImage(window->x + 5, window->y + 47, window->width, window->height, window->buffer);
        }
        drawMouse();
        waitForPit();
        blit(videoBuffer, (EFI_GRAPHICS_OUTPUT_BLT_PIXEL*)GOP->Mode->FrameBufferBase);
        endButtons();
    }
}
