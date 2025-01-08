#include <efi.h>
#include <efilib.h>

struct Window
{
    uint32_t x;
    uint32_t y;
    uint32_t width;
    uint32_t height;
    CHAR16* title;
    EFI_GRAPHICS_OUTPUT_BLT_PIXEL* buffer;
    struct Window* next;
};