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
    initGraphics(window->buffer, window->width, readFile(L"fonts/font.psf", NULL));
    drawRectangle(0, 0, window->width, window->height, black);
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
        removeItem((void**)&window->events, event);
        event = lastEvent;
    }
    drawRectangle(x, y, 32, 32, black);
    uint8_t speed = 5 * frameSkips;
    if (shift)
    {
        speed = 10 * frameSkips;
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
    drawRectangle(x, y, 32, 32, white);
}
