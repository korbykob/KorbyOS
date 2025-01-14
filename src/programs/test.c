#include "../program.h"

struct Window* window;
BOOLEAN w;
BOOLEAN a;
BOOLEAN s;
BOOLEAN d;
int64_t x;
int64_t y;

void _start()
{
    window = allocateFullscreenWindow();
    window->hideMouse = TRUE;
    w = FALSE;
    a = FALSE;
    s = FALSE;
    d = FALSE;
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
                }
                break;
        }
        removeItem((void**)&window->events, event, event->size);
        event = lastEvent;
    }
    if (w)
    {
        y -= 5;
        if (y < 0)
        {
            y = 0;
        }
    }
    if (a)
    {
        x -= 5;
        if (x < 0)
        {
            x = 0;
        }
    }
    if (s)
    {
        y += 5;
        if (y > window->height - 32)
        {
            y = window->height - 32;
        }
    }
    if (d)
    {
        x += 5;
        if (x > window->width - 32)
        {
            x = window->width - 32;
        }
    }
    uint64_t* clear = (uint64_t*)window->buffer;
    for (uint64_t i = 0; i < (window->width * window->height) / 2; i++)
    {
        *clear++ = 0;
    }
    EFI_GRAPHICS_OUTPUT_BLT_PIXEL* address = window->buffer + y * window->width + x;
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

void stop()
{
    unallocateWindow(window);
}