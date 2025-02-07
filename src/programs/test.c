#include "../program.h"

uint64_t id = 0;
Window* window = NULL;
BOOLEAN w = FALSE;
BOOLEAN a = FALSE;
BOOLEAN s = FALSE;
BOOLEAN d = FALSE;
BOOLEAN shift = FALSE;
int64_t x = 0;
int64_t y = 0;

void _start(uint64_t pid)
{
    id = pid;
    window = allocateWindow(640, 360, L"Game", L"programs/test/test.bmp");
    uint64_t* clear = (uint64_t*)window->buffer;
    for (uint64_t i = 0; i < (window->width * window->height) / 2; i++)
    {
        *clear++ = 0;
    }
    x = window->width / 2 - 16;
    y = window->height / 2 - 16;
}

void update(uint64_t frameSkips)
{
    Event* event = (Event*)&window->events;
    Event* lastEvent = event;
    while (iterateList((void**)&event))
    {
        switch (event->id)
        {
            case 0:
                unallocateWindow(window);
                quit(id);
                break;
            case 1:
                switch (((KeyEvent*)event)->scancode)
                {
                    case 17:
                        w = ((KeyEvent*)event)->pressed;
                        break;
                    case 30:
                        a = ((KeyEvent*)event)->pressed;
                        break;
                    case 31:
                        s = ((KeyEvent*)event)->pressed;
                        break;
                    case 32:
                        d = ((KeyEvent*)event)->pressed;
                        break;
                    case 42:
                        shift = ((KeyEvent*)event)->pressed;
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