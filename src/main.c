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
int64_t mouseX = 0;
int64_t mouseY = 0;
struct Button
{
    uint32_t x;
    uint32_t y;
    uint32_t width;
    uint32_t height;
    uint64_t id;
};
struct Button buttons[100];
uint8_t buttonCount = 0;
struct Window windows;
struct Window* dragging = NULL;
struct Window* focus = NULL;

struct Window* allocateWindow(uint32_t width, uint32_t height, CHAR16* title)
{
    struct Window* window = addItem(&windows, sizeof(struct Window));
    window->x = GOP->Mode->Info->HorizontalResolution / 2 - (width + 10) / 2;
    window->y = GOP->Mode->Info->VerticalResolution / 2 - (height + 47) / 2;
    window->width = width;
    window->height = height;
    window->title = title;
    window->buffer = allocate(width * height * 4);
    window->events.next = NULL;
    focus = window;
    return window;
}

void unallocateWindow(struct Window* window)
{
    if (focus == window)
    {
        focus = NULL;
    }
    unallocate(window->buffer, window->width * window->height * 4);
    removeItem(&windows, window, sizeof(struct Window));
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
    if (focus)
    {
        struct KeyEvent* event = addItem(&focus->events, sizeof(struct KeyEvent));
        event->id = 0;
        event->scancode = scancode;
        event->unpressed = unpressed;
    }
}

void mouseMove(int8_t x, int8_t y)
{
    mouseX += x;
    if (mouseX < 0)
    {
        mouseX = 0;
    }
    if (mouseX > GOP->Mode->Info->HorizontalResolution - 16)
    {
        mouseX = GOP->Mode->Info->HorizontalResolution - 16;
    }
    mouseY -= y;
    if (mouseY < 0)
    {
        mouseY = 0;
    }
    if (mouseY > GOP->Mode->Info->VerticalResolution - 16)
    {
        mouseY = GOP->Mode->Info->VerticalResolution - 16;
    }
    if (dragging)
    {
        dragging->x += x;
        if (dragging->x < 0)
        {
            dragging->x = 0;
        }
        if (dragging->x > GOP->Mode->Info->HorizontalResolution - dragging->width - 10)
        {
            dragging->x = GOP->Mode->Info->HorizontalResolution - dragging->width - 10;
        }
        dragging->y -= y;
        if (dragging->y < 0)
        {
            dragging->y = 0;
        }
        if (dragging->y > GOP->Mode->Info->VerticalResolution - dragging->height - 79)
        {
            dragging->y = GOP->Mode->Info->VerticalResolution - dragging->height - 79;
        }
    }
}

void mouseClick(BOOLEAN left, BOOLEAN unpressed)
{
    if (left)
    {
        if (!unpressed)
        {
            for (uint8_t i = 0; i < buttonCount; i++)
            {
                if (mouseX >= buttons[i].x && mouseX < buttons[i].x + buttons[i].width && mouseY >= buttons[i].y && mouseY < buttons[i].y + buttons[i].height)
                {
                    if (programs[buttons[i].id].running)
                    {
                        programs[buttons[i].id].stop();
                    }
                    else
                    {
                        programs[buttons[i].id].start();
                    }
                    programs[buttons[i].id].running = !programs[buttons[i].id].running;
                }
            }
            struct Window* window = &windows;
            while (iterateList((void**)&window))
            {
                if (mouseX >= window->x && mouseX < window->x + window->width + 20 && mouseY >= window->y && mouseY < window->y + 42)
                {
                    dragging = window;
                    focus = window;
                    break;
                }
            }
        }
        else
        {
            dragging = NULL;
        }
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
    mouseX = GOP->Mode->Info->HorizontalResolution / 2;
    mouseY = GOP->Mode->Info->VerticalResolution / 2;
    while (TRUE)
    {
        blit(wallpaper, videoBuffer);
        if (focus)
        {
            drawRectangle(focus->x, focus->y, focus->width + 10, focus->height + 47, white);
        }
        struct Window* window = &windows;
        while (iterateList((void**)&window))
        {
            if (window != focus)
            {
                drawRectangle(window->x, window->y, window->width + 10, window->height + 47, grey);
            }
            else
            {
                drawRectangle(window->x + 5, window->y + 5, window->width, window->height + 37, grey);
            }
            drawString(window->title, window->x + 5, window->y + 5, black);
            drawImage(window->x + 5, window->y + 42, window->width, window->height, window->buffer);
        }
        drawRectangle(0, GOP->Mode->Info->VerticalResolution - 32, GOP->Mode->Info->HorizontalResolution, 32, grey);
        struct Button mainButton;
        mainButton.x = 4;
        mainButton.y = GOP->Mode->Info->VerticalResolution - 28;
        mainButton.width = 24;
        mainButton.height = 24;
        mainButton.id = 0;
        drawRectangle(mainButton.x, mainButton.y, mainButton.width, mainButton.height, white);
        uint8_t buttonCountBuffer = 0;
        for (uint8_t i = 0; i < 1; i++)
        {
            struct Button button;
            button.x = 4 + i * 24 + i * 8;
            button.y = GOP->Mode->Info->VerticalResolution - 28;
            button.width = 24;
            button.height = 24;
            button.id = i;
            if (programs[i].running)
            {
                programs[i].update();
                drawRectangle(button.x - 2, GOP->Mode->Info->VerticalResolution - 30, 28, 28, black);
            }
            drawImage(button.x, button.y, button.width, button.height, programs[i].icon);
            buttons[buttonCountBuffer] = button;
            buttonCountBuffer++;
        }
        buttonCount = buttonCountBuffer;
        drawMouse();
        waitForPit();
        blit(videoBuffer, (EFI_GRAPHICS_OUTPUT_BLT_PIXEL*)GOP->Mode->FrameBufferBase);
    }
}
