#include "../program.h"

struct Window* window;
uint64_t x;
uint64_t y;
BOOLEAN pressed;
EFI_GRAPHICS_OUTPUT_BLT_PIXEL* buffer;

void _start()
{
    window = allocateWindow(640, 360, L"Window test");
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

void update()
{
    struct Event* event = (struct Event*)&window->events;
    struct Event* lastEvent = event;
    while (iterateList((void**)&event))
    {
        switch (event->id)
        {
            case 1:
                if (((struct ClickEvent*)event)->left == TRUE)
                {
                    pressed = !((struct ClickEvent*)event)->unpressed;
                }
                break;
            case 2:
                if (pressed)
                {
                    while (x != ((struct MouseEvent*)event)->x || y != ((struct MouseEvent*)event)->y)
                    {
                        int64_t differenceX = ((struct MouseEvent*)event)->x - x;
                        int64_t differenceY = ((struct MouseEvent*)event)->y - y;
                        if (differenceX != 0)
                        {
                            x += differenceX > 0 ? 1 : -1;
                        }
                        if (differenceY != 0)
                        {
                            y += differenceY > 0 ? 1 : -1;
                        }
                        ((uint32_t*)buffer)[y * window->width + x] = 0xFFFFFFFF;
                    }
                }
                x = ((struct MouseEvent*)event)->x;
                y = ((struct MouseEvent*)event)->y;
                break;
        }
        removeItem((void**)&window->events, event, event->size);
        event = lastEvent;
    }
    uint64_t* to = (uint64_t*)window->buffer;
    uint64_t* from = (uint64_t*)buffer;
    for (uint64_t i = 0; i < (window->width * window->height) / 2; i++)
    {
        *to++ = *from++;
    }
    ((uint32_t*)window->buffer)[y * window->width + x] = 0xFFFFFFFF;
}

void stop()
{
    unallocateWindow(window);
    unallocate(buffer, window->width * window->height * 4);
}