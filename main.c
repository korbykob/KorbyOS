#include "main.h"

extern void interrupts();
extern void pitHandler();
extern void cascadeHandler();

struct {
    uint16_t lower;
    uint16_t selector;
    uint8_t ist;
    uint8_t attributes;
    uint16_t middle;
    uint32_t higher;
    uint32_t zero;
} idt[256];
BOOLEAN fired = FALSE;

static inline void outb(uint16_t port, uint8_t val)
{
    __asm__ volatile ( "outb %b0, %w1" : : "a"(val), "Nd"(port) : "memory");
    /* There's an outb %al, $imm8 encoding, for compile-time constant port numbers that fit in 8b. (N constraint).
     * Wider immediate constants would be truncated at assemble-time (e.g. "i" constraint).
     * The  outb  %al, %dx  encoding is the only option for all other cases.
     * %1 expands to %dx because  port  is a uint16_t.  %w1 could be used if we had the port number a wider C type */
}

static inline uint8_t inb(uint16_t port)
{
    uint8_t ret;
    __asm__ volatile ( "inb %w1, %b0"
                   : "=a"(ret)
                   : "Nd"(port)
                   : "memory");
    return ret;
}

static inline void lidt(void* base, uint16_t size)
{
    // This function works in 32 and 64bit mode
    struct {
        uint16_t length;
        void*    base;
    } __attribute__((packed)) IDTR = { size, base };

    __asm__ ( "lidt %0" : : "m"(IDTR) );  // let the compiler choose an addressing mode
}

void pit()
{
    fired = TRUE;
    outb(0x20, 0x20);
}

void installInterrupt(uint8_t interrupt, void* handler)
{
    uint16_t index = 0x20 + interrupt;
    idt[index].lower = (uint64_t)handler & 0x000000000000FFFF;
    idt[index].selector = 0x08;
    idt[index].ist = 0;
    idt[index].attributes = 0x8E;
    idt[index].middle = ((uint64_t)handler & 0x00000000FFFF0000) >> 16;
    idt[index].higher = ((uint64_t)handler & 0xFFFFFFFF00000000) >> 32;
    idt[index].zero = 0;
    if (interrupt < 8)
    {
        outb(0x21, inb(0x21) & ~(1 << interrupt));
    }
    else
    {
        outb(0xA1, inb(0xA1) & ~(1 << (interrupt - 8)));
    }
}

void start()
{
    outb(0x43, 0x34);
    uint16_t divisor = 1193180 / 60;
    outb(0x40, divisor & 0xFF);
    outb(0x40, (divisor >> 8) & 0xFF);
    outb(0x20, 0x11);
    outb(0xA0, 0x11);
    outb(0x21, 0x20);
    outb(0xA1, 0x28);
    outb(0x21, 0x04);
    outb(0xA1, 0x02);
    outb(0x21, 0x01);
    outb(0xA1, 0x01);
    outb(0x21, 0xFF);
    outb(0xA1, 0xFF);
    installInterrupt(0, pitHandler);
    installInterrupt(2, cascadeHandler);
    lidt(idt, 4095);
    interrupts();
    BOOLEAN flash = FALSE;
    while (!fired)
    {
        if (flash)
        {
            for (UINTN i = 0; i < GOP->Mode->FrameBufferSize; i++)
            {
                ((uint8_t*)GOP->Mode->FrameBufferBase)[i] = 0;
            }
        }
        else
        {
            for (UINTN i = 0; i < GOP->Mode->FrameBufferSize; i++)
            {
                ((uint8_t*)GOP->Mode->FrameBufferBase)[i] = 255;
            }
        }
        flash = !flash;
    }
    EFI_GRAPHICS_OUTPUT_BLT_PIXEL normal;
    normal.Red = 152;
    normal.Green = 152;
    normal.Blue = 152;
    drawString(L"IT FIRED YAYAYAYAY!!!", 0, 0, normal);
    blit();
}

/*void start()
{
    EFI_GRAPHICS_OUTPUT_BLT_PIXEL normal;
    normal.Red = 152;
    normal.Green = 152;
    normal.Blue = 152;
    drawString(L"Welcome to ", GOP->Mode->Info->HorizontalResolution / 2 - 152, GOP->Mode->Info->VerticalResolution / 2 - 32, normal);
    EFI_GRAPHICS_OUTPUT_BLT_PIXEL white;
    white.Red = 255;
    white.Green = 255;
    white.Blue = 255;
    drawString(L"KorbyOS", GOP->Mode->Info->HorizontalResolution / 2 + 24, GOP->Mode->Info->VerticalResolution / 2 - 32, white);
    drawString(L"!", GOP->Mode->Info->HorizontalResolution / 2 + 136, GOP->Mode->Info->VerticalResolution / 2 - 32, normal);
    drawString(L"Press any key to continue...", GOP->Mode->Info->HorizontalResolution / 2 - 224, GOP->Mode->Info->VerticalResolution / 2, normal);
    blit();
    while (1);
    ZeroMem(videoBuffer, GOP->Mode->FrameBufferSize);
    blit();
}*/