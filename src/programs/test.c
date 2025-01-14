#include "../program.h"

uint64_t pid = 0;
struct Window* window = NULL;
BOOLEAN w = FALSE;
BOOLEAN a = FALSE;
BOOLEAN s = FALSE;
BOOLEAN d = FALSE;
BOOLEAN shift = FALSE;
int64_t x = 0;
int64_t y = 0;

void _start(uint64_t id)
{
    pid = id;
    window = allocateWindow(640, 360, L"Game");
    uint64_t* clear = (uint64_t*)window->buffer;
    for (uint64_t i = 0; i < (window->width * window->height) / 2; i++)
    {
        *clear++ = 0;
    }
    window->hideMouse = TRUE;
    x = window->width / 2 - 16;
    y = window->height / 2 - 16;
}

void update()
{
    struct Event* event = (struct Event*)&window->events;
    struct Event* lastEvent = event;
    while (iterateList((void**)&event))
    {
        switch (event->id)
        {
            case 0:
                unallocateWindow(window);
                quit(pid);
                break;
            case 1:
                switch (((struct KeyEvent*)event)->scancode)
                {
                    case 17:
                        w = ((struct KeyEvent*)event)->pressed;
                        break;
                    case 30:
                        a = ((struct KeyEvent*)event)->pressed;
                        break;
                    case 31:
                        s = ((struct KeyEvent*)event)->pressed;
                        break;
                    case 32:
                        d = ((struct KeyEvent*)event)->pressed;
                        break;
                    case 42:
                        shift = ((struct KeyEvent*)event)->pressed;
                        break;
                }
                break;
        }
        removeItem((void**)&window->events, event, event->size);
        event = lastEvent;
    }
    EFI_GRAPHICS_OUTPUT_BLT_PIXEL* address = window->buffer + y * window->width + x;
    EFI_GRAPHICS_OUTPUT_BLT_PIXEL black = { 0, 0, 0 };
    for (uint32_t y = 0; y < 32; y++)
    {
        for (uint32_t x = 0; x < 32; x++)
        {
            *address++ = black;
        }
        address += window->width - 32;
    }
    uint8_t speed = 5;
    if (shift)
    {
        speed = 10;
    }
    if (w)
    {
        y -= speed;
        if (y < 0)
        {
            y = 0;
        }
    }
    if (a)
    {
        x -= speed;
        if (x < 0)
        {
            x = 0;
        }
    }
    if (s)
    {
        y += speed;
        if (y > window->height - 32)
        {
            y = window->height - 32;
        }
    }
    if (d)
    {
        x += speed;
        if (x > window->width - 32)
        {
            x = window->width - 32;
        }
    }
    address = window->buffer + y * window->width + x;
    EFI_GRAPHICS_OUTPUT_BLT_PIXEL white = { 255, 255, 255 };
    for (uint32_t y = 0; y < 32; y++)
    {
        for (uint32_t x = 0; x < 32; x++)
        {
            *address++ = white;
        }
        address += window->width - 32;
    }
}