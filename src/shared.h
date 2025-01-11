#include <efi.h>
#include <efilib.h>

struct Event
{
    struct Event* next;
    uint8_t id;
};
struct Window
{
    struct Window* next;
    int64_t x;
    int64_t y;
    uint32_t width;
    uint32_t height;
    CHAR16* title;
    EFI_GRAPHICS_OUTPUT_BLT_PIXEL* buffer;
    struct Event events;
};
struct KeyEvent
{
    struct Event* next;
    uint8_t id;
    uint8_t scancode;
    BOOLEAN unpressed;
};

void* allocate(uint64_t amount);

void unallocate(void* pointer, uint64_t amount);

void* addItem(void* list, uint64_t size)
{
    while (*(void**)list)
    {
        list = *(void**)list;
    }
    *(void**)list = allocate(size);
    **(void***)list = NULL;
    return *(void**)list;
}

void removeItem(void* list, void* item, uint64_t size)
{
    while (*(void**)list != item)
    {
        list = *(void**)list;
    }
    unallocate(*(void**)list, size);
    *(void**)list = **(void***)list;
}

BOOLEAN iterateList(void** iterator)
{
    *iterator = **(void***)iterator;
    return *iterator;
}