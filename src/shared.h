#include <efi.h>
#include <efilib.h>

struct Event
{
    struct Event* next;
    uint8_t id;
    uint8_t size;
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
    BOOLEAN hideMouse;
    struct Event* events;
};
struct KeyEvent
{
    struct Event* next;
    uint8_t id;
    uint8_t size;
    uint8_t scancode;
    BOOLEAN unpressed;
};
struct ClickEvent
{
    struct Event* next;
    uint8_t id;
    uint8_t size;
    BOOLEAN left;
    BOOLEAN unpressed;
};

void* allocate(uint64_t amount);

void unallocate(void* pointer, uint64_t amount);

void* addItem(void** list, uint64_t size)
{
    while (*list)
    {
        list = *list;
    }
    *list = allocate(size);
    **(void***)list = NULL;
    return *list;
}

void removeItem(void** list, void* item, uint64_t size)
{
    while (*list != item)
    {
        list = *list;
    }
    unallocate(*list, size);
    *list = **(void***)list;
}

BOOLEAN iterateList(void** iterator)
{
    *iterator = **(void***)iterator;
    return *iterator;
}