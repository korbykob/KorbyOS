#include <efi.h>
#include <efilib.h>

const char* lastDebug = NULL;
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
    uint8_t mouseMode;
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
typedef struct
{
    Event header;
    int64_t x;
    int64_t y;
} MouseEvent;
EFI_GRAPHICS_OUTPUT_BLT_PIXEL* graphicsBuffer = NULL;
uint32_t graphicsPitch = 0;
uint8_t* graphicsFont = NULL;
EFI_GRAPHICS_OUTPUT_BLT_PIXEL white = { 255, 255, 255 };
EFI_GRAPHICS_OUTPUT_BLT_PIXEL black = { 0, 0, 0 };
EFI_GRAPHICS_OUTPUT_BLT_PIXEL grey = { 128, 128, 128 };
EFI_GRAPHICS_OUTPUT_BLT_PIXEL red = { 0, 0, 255 };

void* allocate(uint64_t amount);

void unallocate(void* pointer);

void outb(uint16_t port, uint8_t value)
{
    __asm__ volatile ("outb %b0, %w1" : : "a"(value), "Nd"(port) : "memory");
}

uint8_t inb(uint16_t port)
{
    uint8_t value = 0;
    __asm__ volatile ("inb %w1, %b0" : "=a"(value) : "Nd"(port) : "memory");
    return value;
}

#define SERIAL_DEBUG

void debug(const char* string)
{
    lastDebug = string;
    #ifdef SERIAL_DEBUG
    while (*string != 0)
    {
        while ((inb(0x3ED) & 0x20) == 0);
        outb(0x3E8, *string++);
    }
    #endif
}

void copyMemory(uint8_t* source, uint8_t* destination, uint64_t size)
{
    for (uint64_t i = 0; i < size; i++)
    {
        *destination++ = *source++;
    }
}

void splitTask(void (*task)(uint64_t id), uint64_t count)
{
    for (uint64_t i = 0; i < count - 1; i++)
    {
        *(uint64_t*)(0x5008 + i * 8) = (uint64_t)task;
    }
    task(0);
    while (TRUE)
    {
        uint64_t done = 0;
        for (uint64_t i = 0; i < count - 1; i++)
        {
            done += *(uint64_t*)(0x5008 + i * 8);
        }
        if (done == 0)
        {
            break;
        }
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

void initGraphics(EFI_GRAPHICS_OUTPUT_BLT_PIXEL* buffer, uint32_t pitch, uint8_t* font)
{
    graphicsBuffer = buffer;
    graphicsPitch = pitch;
    graphicsFont = font;
}

void blit(EFI_GRAPHICS_OUTPUT_BLT_PIXEL* source, EFI_GRAPHICS_OUTPUT_BLT_PIXEL* destination, uint32_t width, uint32_t height)
{
    uint64_t* to = (uint64_t*)destination;
    uint64_t* from = (uint64_t*)source;
    for (uint64_t i = 0; i < (width * height) / 2; i++)
    {
        *to++ = *from++;
    }
}

void drawRectangle(uint32_t x, uint32_t y, uint32_t width, uint32_t height, EFI_GRAPHICS_OUTPUT_BLT_PIXEL colour)
{
    EFI_GRAPHICS_OUTPUT_BLT_PIXEL colours[2];
    colours[0] = colour;
    colours[1] = colour;
    uint64_t* address = (uint64_t*)(graphicsBuffer + y * graphicsPitch + x);
    uint32_t drop = (graphicsPitch - width) / 2;
    for (uint32_t y = 0; y < height; y++)
    {
        for (uint32_t x = 0; x < width / 2; x++)
        {
            *address++ = *(uint64_t*)colours;
        }
        address += drop;
    }
}

void drawImage(uint32_t x, uint32_t y, uint32_t width, uint32_t height, EFI_GRAPHICS_OUTPUT_BLT_PIXEL* buffer)
{
    uint64_t* from = (uint64_t*)buffer;
    uint64_t* to = (uint64_t*)(graphicsBuffer + y * graphicsPitch + x);
    uint64_t drop = (graphicsPitch - width) / 2;
    for (uint32_t y = 0; y < height; y++)
    {
        for (uint32_t x = 0; x < width / 2; x++)
        {
            *to++ = *from++;
        }
        to += drop;
    }
}

void drawCharacter(CHAR16 character, uint32_t x, uint32_t y, EFI_GRAPHICS_OUTPUT_BLT_PIXEL colour)
{
    EFI_GRAPHICS_OUTPUT_BLT_PIXEL* address = graphicsBuffer + y * graphicsPitch + x;
    uint8_t* glyph = graphicsFont + ((uint32_t*)graphicsFont)[2] + ((uint32_t*)graphicsFont)[5] * character;
    for (uint8_t y = 0; y < 64; y += 2)
    {
        for (uint8_t x = 0; x < 8; x++)
        {
            if (glyph[y] & (0b10000000 >> x))
            {
                *address = colour;
            }
            address++;
        }
        for (uint8_t x = 0; x < 8; x++)
        {
            if (glyph[y + 1] & (0b10000000 >> x))
            {
                *address = colour;
            }
            address++;
        }
        address += graphicsPitch - 16;
    }
}

void drawString(const CHAR16* string, uint32_t x, uint32_t y, EFI_GRAPHICS_OUTPUT_BLT_PIXEL colour)
{
    EFI_GRAPHICS_OUTPUT_BLT_PIXEL* address = graphicsBuffer + y * graphicsPitch + x;
    while (*string != 0)
    {
        uint8_t* glyph = graphicsFont + 32 + 64 * *string++;
        for (uint8_t y = 0; y < 32; y++)
        {
            for (uint8_t x = 0; x < 8; x++)
            {
                if (*glyph & (0b10000000 >> x))
                {
                    *address = colour;
                }
                address++;
            }
            glyph++;
            for (uint8_t x = 0; x < 8; x++)
            {
                if (*glyph & (0b10000000 >> x))
                {
                    *address = colour;
                }
                address++;
            }
            glyph++;
            address += graphicsPitch - 16;
        }
        address -= graphicsPitch * 32 - 16;
    }
}

#define max(a,b) (((a) > (b)) ? (a) : (b))
#define min(a,b) (((a) < (b)) ? (a) : (b))

double sqrt(double x)
{
    double guess = x / 2.0;
    double epsilon = 0.00001;
    while ((guess * guess - x) > epsilon || (x - guess * guess) > epsilon)
    {
        guess = (guess + x / guess) / 2.0;
    }
    return guess;
}

double sin(double x)
{
    double term = x;
    double sum = x;
    uint64_t i = 1;
    double epsilon = 0.00001;
    while (term > epsilon || -term > epsilon)
    {
        term *= -x * x / ((2 * i) * (2 * i + 1));
        sum += term;
        i++;
    }
    return sum;
}

double cos(double x)
{
    double term = 1;
    double sum = 1;
    uint64_t i = 1;
    double epsilon = 0.00001;
    while (term > epsilon || -term > epsilon)
    {
        term *= -x * x / ((2 * i - 1) * (2 * i));
        sum += term;
        i++;
    }
    return sum;
}

double tan(double x)
{
    double sine = sin(x);
    double cosine = cos(x);
    return sine / cosine;
}

double atan(double x)
{
    if (x > 1 || x < -1)
    {
        return (x > 0 ? 1 : -1) * (3.14159265358979323846 / 2 - atan(1 / x));
    }
    double term = x;
    double sum = x;
    uint64_t i = 1;
    double epsilon = 0.00001;
    while (term > epsilon || -term > epsilon)
    {
        term *= -x * x * (2 * i - 1) / (2 * i + 1);
        sum += term;
        i++;
    }
    return sum;
}
