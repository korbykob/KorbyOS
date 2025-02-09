#include "../program.h"

Window* window = NULL;
uint64_t id = 0;
uint32_t x = 0;
uint32_t y = 0;
BOOLEAN pressed = FALSE;
EFI_GRAPHICS_OUTPUT_BLT_PIXEL* buffer = NULL;

void _start(uint64_t pid)
{
    id = pid;
    window = allocateWindow(640, 360, L"Game", L"programs/test/test.bmp");
    window->hideMouse = TRUE;
    x = 0;
    y = 0;
    pressed = FALSE;
    buffer = allocate(window->width * window->height * 4);
    uint64_t* destination = (uint64_t*)buffer;
    for (uint64_t i = 0; i < (window->width * window->height) / 2; i++)
    {
        *destination++ = 0;
    }
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
                unallocate(buffer);
                quit(id);
                break;
            case 2:
                if (((ClickEvent*)event)->left)
                {
                    pressed = ((ClickEvent*)event)->pressed;
                }
                break;
            case 3:
                if (pressed)
                {
                    while (x != ((MouseEvent*)event)->x || y != ((MouseEvent*)event)->y)
                    {
                        int64_t differenceX = ((MouseEvent*)event)->x - (int64_t)x;
                        int64_t differenceY = ((MouseEvent*)event)->y - (int64_t)y;
                        if (differenceX != 0)
                        {
                            x += differenceX > 0 ? 1 : -1;
                        }
                        if (differenceY != 0)
                        {
                            y += differenceY > 0 ? 1 : -1;
                        }
                        buffer[y * window->width + x] = white;
                    }
                }
                x = ((MouseEvent*)event)->x;
                y = ((MouseEvent*)event)->y;
                break;
        }
        removeItem((void**)&window->events, event);
        event = lastEvent;
    }
    blit(buffer, window->buffer, window->width, window->height);
    window->buffer[y * window->width + x] = white;
}
