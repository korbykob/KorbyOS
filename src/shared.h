#include <efi.h>
#include <efilib.h>

typedef struct
{
    void* next;
    uint8_t id;
} Event;
typedef struct
{
    void* next;
    int64_t x;
    int64_t y;
    uint32_t width;
    uint32_t height;
    CHAR16* title;
    EFI_GRAPHICS_OUTPUT_BLT_PIXEL* icon;
    EFI_GRAPHICS_OUTPUT_BLT_PIXEL* buffer;
    BOOLEAN hideMouse;
    BOOLEAN fullscreen;
    BOOLEAN minimised;
    Event* events;
} Window;
typedef struct
{
    Event header;
    uint8_t scancode;
    BOOLEAN pressed;
} KeyEvent;
typedef struct
{
    Event header;
    BOOLEAN left;
    BOOLEAN pressed;
} ClickEvent;

void* allocate(uint64_t amount);

void unallocate(void* pointer);

void copyMemory(uint8_t* source, uint8_t* destination, uint64_t size)
{
    for (uint64_t i = 0; i < size; i++)
    {
        *destination++ = *source++;
    }
}

void readBitmap(uint8_t* bitmap, EFI_GRAPHICS_OUTPUT_BLT_PIXEL* destination)
{
    uint8_t* fileBuffer = bitmap + 0x36;
    int32_t width = *(int32_t*)(bitmap + 0x12);
    int32_t height = *(int32_t*)(bitmap + 0x16);
    for (uint64_t y = 0; y < height; y++)
    {
        for (uint64_t x = 0; x < width; x++)
        {
            uint64_t index = ((height - y - 1) * width + x) * 3;
            destination->Blue = fileBuffer[index];
            destination->Green = fileBuffer[index + 1];
            destination->Red = fileBuffer[index + 2];
            destination++;
        }
    }
}

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

void removeItem(void** list, void* item)
{
    while (*list != item)
    {
        list = *list;
    }
    unallocate(*list);
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
