#include <efi.h>
#include <efilib.h>

struct Window
{
    struct Window* next;
    int64_t x;
    int64_t y;
    uint32_t width;
    uint32_t height;
    CHAR16* title;
    EFI_GRAPHICS_OUTPUT_BLT_PIXEL* buffer;
    BOOLEAN dragging;
};