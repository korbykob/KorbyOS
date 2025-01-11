#include "../program.h"

struct Window* window;
EFI_GRAPHICS_OUTPUT_BLT_PIXEL colour;
BOOLEAN shift;

void _start()
{
    window = allocateWindow(640, 360, L"Cyberpunk 2077 (REAL11!!1)");
    colour.Red = 0;
    colour.Green = 0;
    colour.Blue = 0;
    shift = TRUE;
}

void update()
{
    struct Event* event = &window->events;
    struct Event* lastEvent = event;
    while (iterateList((void**)&event))
    {
        switch (event->id)
        {
            case 0:
                if (!((struct KeyEvent*)event)->unpressed)
                {
                    shift = !shift;
                }
                removeItem(&window->events, event, sizeof(struct KeyEvent));
                event = lastEvent;
                break;
        }
        lastEvent = event;
    }
    if (shift)
    {
        for (uint32_t y = 0; y < window->height; y++)
        {
            for (uint32_t x = 0; x < window->width; x++)
            {
                window->buffer[y * window->width + x] = colour;
            }
        }
        if (colour.Red != 0xFF && colour.Green == 0x00)
        {
            colour.Red += 17;
        }
        else if (colour.Blue != 0x00 && colour.Green == 0x00)
        {
            colour.Blue -= 17;
        }
        else if (colour.Green != 0xFF && colour.Blue != 0xFF)
        {
            colour.Green += 17;
        }
        else if (colour.Red != 0x00)
        {
            colour.Red -= 17;
        }
        else if (colour.Blue != 0xFF)
        {
            colour.Blue += 17;
        }
        else if (colour.Green != 0x00)
        {
            colour.Green -= 17;
        }
    }
}

void stop()
{
    unallocateWindow(window);
}