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
typedef struct
{
    Event header;
    uint32_t x;
    uint32_t y;
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

const double PI=3.1415926535897932384650288;

double sin(double x){
  double sign=1;
  if (x<0){
    sign=-1.0;
    x=-x;
  }
  if (x>360) x -= (int)(x/360)*360;
  x*=PI/180.0;
  double res=0;
  double term=x;
  int k=1;
  while (res+term!=res){
    res+=term;
    k+=2;
    term*=-x*x/k/(k-1);
  }

  return sign*res;
}

double cos(double x){
  if (x<0) x=-x;
  if (x>360) x -= (int)(x/360)*360;
  x*=PI/180.0;
  double res=0;
  double term=1;
  int k=0;
  while (res+term!=res){
    res+=term;
    k+=2;
    term*=-x*x/k/(k-1);
  }  
  return res;
}

#define min(a,b) (((a)<(b))?(a):(b))
#define max(a,b) (((a)>(b))?(a):(b))


