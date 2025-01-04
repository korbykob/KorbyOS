#include "../program.h"

void _start()
{
    uint8_t* address = alloc(1000000);
    for (size_t i = 0; i < 1000000; i++)
    {
        address[i] = 0xFF;
    }
}