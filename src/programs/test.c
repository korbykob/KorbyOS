#include "../program.h"

struct Window* window;
uint8_t gradient;

void _start()
{
    window = allocateWindow(500, 500, L"Window test");
    gradient = 0;
}

void update()
{
    EFI_GRAPHICS_OUTPUT_BLT_PIXEL colour;
    colour.Red = gradient;
    colour.Green = gradient;
    colour.Blue = gradient;
    EFI_GRAPHICS_OUTPUT_BLT_PIXEL colours[2];
    colours[0] = colour;
    colours[1] = colour;
    uint64_t* buffer = (uint64_t*)window->buffer;
    for (uint64_t i = 0; i < (window->width * window->height) / 2; i++)
    {
        *buffer++ = *(uint64_t*)colours;
    }
    gradient++;
}

void stop()
{
    unallocateWindow(window);
}