#include <efi.h>
#include <efilib.h>

typedef struct
{
    void* next;
    uint8_t id;
    uint8_t size;
} Event;
typedef struct
{
    void* next;
    int64_t x;
    int64_t y;
    uint32_t width;
    uint32_t height;
    CHAR16* title;
    EFI_GRAPHICS_OUTPUT_BLT_PIXEL* buffer;
    BOOLEAN hideMouse;
    BOOLEAN fullscreen;
    BOOLEAN minimised;
    Event* events;
} Window;
typedef struct
{
    void* next;
    uint8_t id;
    uint8_t size;
    uint8_t scancode;
    BOOLEAN pressed;
} KeyEvent;
typedef struct
{
    void* next;
    uint8_t id;
    uint8_t size;
    BOOLEAN left;
    BOOLEAN pressed;
} ClickEvent;

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

void moveItemEnd(void** list, void* item)
{
    while (*list != item)
    {
        list = *list;
    }
    *list = *(void**)item;
    while (*list)
    {
        list = *list;
    }
    *list = item;
    *(void**)item = NULL;
}

uint64_t listLength(void** list)
{
    void** current = (void**)&list;
    uint64_t i = 0;
    while (*current)
    {
        *current = **(void***)current;
        i++;
    }
    return i - 1;
}