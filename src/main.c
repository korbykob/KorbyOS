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
struct Window* windows = NULL;
struct Window* dragging = NULL;
struct Window* focus = NULL;

struct Window* allocateWindow(uint32_t width, uint32_t height, CHAR16* title)
{
    struct Window* window = addItem((void**)&windows, sizeof(struct Window));
    window->x = GOP->Mode->Info->HorizontalResolution / 2 - (width + 10) / 2;
    window->y = GOP->Mode->Info->VerticalResolution / 2 - (height + 47) / 2;
    window->width = width;
    window->height = height;
    window->title = title;
    window->buffer = allocate(width * height * 4);
    window->events = NULL;
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
    removeItem((void**)&windows, window, sizeof(struct Window));
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
        struct KeyEvent* event = addItem((void**)&focus->events, sizeof(struct KeyEvent));
        event->id = 0;
        event->size = sizeof(struct KeyEvent);
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
        if (dragging->x > GOP->Mode->Info->HorizontalResolution - dragging->width - 20)
        {
            dragging->x = GOP->Mode->Info->HorizontalResolution - dragging->width - 20;
        }
        dragging->y -= y;
        if (dragging->y < 0)
        {
            dragging->y = 0;
        }
        if (dragging->y > GOP->Mode->Info->VerticalResolution - dragging->height - 89)
        {
            dragging->y = GOP->Mode->Info->VerticalResolution - dragging->height - 89;
        }
    }
    if (focus && mouseX >= focus->x + 10 && mouseX < focus->x + focus->width + 10 && mouseY >= focus->y + 47 && mouseY < focus->y + 47 + focus->height)
    {
        struct MouseEvent* event = addItem((void**)&focus->events, sizeof(struct MouseEvent));
        event->id = 2;
        event->size = sizeof(struct MouseEvent);
        event->x = mouseX - (focus->x + 10);
        event->y = mouseY - (focus->y + 47);
    }
}

void mouseClick(BOOLEAN left, BOOLEAN unpressed)
{
    if (left)
    {
        if (!unpressed)
        {
            for (uint8_t i = 0; i < 1; i++)
            {
                uint64_t x = 4 + i * 24 + i * 8;
                if (mouseX >= x && mouseX < x + 24 && mouseY >= GOP->Mode->Info->VerticalResolution - 28 && mouseY < GOP->Mode->Info->VerticalResolution - 4)
                {
                    if (programs[i].running)
                    {
                        programs[i].stop();
                    }
                    else
                    {
                        programs[i].start();
                    }
                    programs[i].running = !programs[i].running;
                }
            }
            struct Window* window = (struct Window*)&windows;
            while (iterateList((void**)&window))
            {
                if (mouseX >= window->x && mouseX < window->x + window->width + 20 && mouseY >= window->y && mouseY < window->y + 47)
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
    if (focus && mouseX >= focus->x + 10 && mouseX < focus->x + focus->width + 10 && mouseY >= focus->y + 47 && mouseY < focus->y + 47 + focus->height)
    {
        struct ClickEvent* event = addItem((void**)&focus->events, sizeof(struct ClickEvent));
        event->id = 1;
        event->size = sizeof(struct ClickEvent);
        event->left = left;
        event->unpressed = unpressed;
    }
}

void start()
{
    mouseX = GOP->Mode->Info->HorizontalResolution / 2;
    mouseY = GOP->Mode->Info->VerticalResolution / 2;
    while (TRUE)
    {
        blit(wallpaper, videoBuffer);
        struct Window* window = (struct Window*)&windows;
        while (iterateList((void**)&window))
        {
            drawRectangle(focus->x, focus->y, focus->width + 20, focus->height + 57, focus == window ? white : black);
            drawRectangle(window->x + 5, window->y + 5, window->width + 10, window->height + 47, grey);
            drawString(window->title, window->x + 10, window->y + 10, black);
            drawImage(window->x + 10, window->y + 47, window->width, window->height, window->buffer);
        }
        drawRectangle(0, GOP->Mode->Info->VerticalResolution - 32, GOP->Mode->Info->HorizontalResolution, 32, grey);
        for (uint8_t i = 0; i < 1; i++)
        {
            uint64_t x = 4 + i * 24 + i * 8;
            if (programs[i].running)
            {
                programs[i].update();
                drawRectangle(x - 2, GOP->Mode->Info->VerticalResolution - 30, 28, 28, black);
            }
            drawImage(x, GOP->Mode->Info->VerticalResolution - 28, 24, 24, programs[i].icon);
        }
        EFI_GRAPHICS_OUTPUT_BLT_PIXEL* address = videoBuffer + mouseY * GOP->Mode->Info->HorizontalResolution + mouseX;
        uint8_t* buffer = mouseIcon;
        for (uint32_t y = 0; y < 16; y++)
        {
            for (uint32_t x = 0; x < 16; x++)
            {
                if (*buffer != 2)
                {
                    *address = *buffer == 0 ? black : white;
                }
                buffer++;
                address++;
            }
            address += GOP->Mode->Info->HorizontalResolution - 16;
        }
        waitForPit();
        blit(videoBuffer, (EFI_GRAPHICS_OUTPUT_BLT_PIXEL*)GOP->Mode->FrameBufferBase);
    }
}
