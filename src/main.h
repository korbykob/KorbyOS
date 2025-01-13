#include "shared.h"

extern void syscallHandler();

void* memory = NULL;
BOOLEAN* allocated = NULL;
uint64_t cleared = 0;
EFI_GRAPHICS_OUTPUT_PROTOCOL* GOP = NULL;
EFI_GRAPHICS_OUTPUT_BLT_PIXEL* videoBuffer = NULL;
uint8_t* font = NULL;
EFI_GRAPHICS_OUTPUT_BLT_PIXEL* wallpaper = NULL;
struct
{
    EFI_GRAPHICS_OUTPUT_BLT_PIXEL icon[24*24];
    void (*start)();
    void (*update)();
    void (*stop)();
    BOOLEAN running;
} programs[1];
struct
{
    uint16_t lower;
    uint16_t selector;
    uint8_t ist;
    uint8_t attributes;
    uint16_t middle;
    uint32_t higher;
    uint32_t zero;
} __attribute__((packed)) idt[256];
struct InterruptFrame
{
    uint64_t ip;
    uint64_t cs;
    uint64_t flags;
    uint64_t sp;
    uint64_t ss;
} __attribute__((packed));
BOOLEAN waitPit = FALSE;
BOOLEAN waitKey = FALSE;
uint8_t mouseCycle = 2;
int8_t mouseBytes[3];
BOOLEAN lastLeftClick = FALSE;
BOOLEAN lastRightClick = FALSE;

void* allocate(uint64_t amount)
{
    BOOLEAN* test = allocated;
    uint64_t value = 0;
    retry:
    while (*test)
    {
        if (value == cleared)
        {
            cleared = value + amount;
            goto done;
        }
        test--;
        value++;
    }
    uint64_t size = 0;
    while (size != amount)
    {
        if (value + size == cleared)
        {
            cleared = value + amount;
            goto done;
        }
        if (*test)
        {
            goto retry;
        }
        test--;
        size++;
    }
    done:
    test = allocated - value;
    for (uint64_t i = 0; i < amount; i++)
    {
        *test-- = TRUE;
    }
    return memory + value;
}

void unallocate(void* pointer, uint64_t amount)
{
    BOOLEAN* room = allocated - (pointer - memory);
    for (uint64_t i = 0; i < amount; i++)
    {
        *room-- = FALSE;
    }
}

void blit(EFI_GRAPHICS_OUTPUT_BLT_PIXEL* source, EFI_GRAPHICS_OUTPUT_BLT_PIXEL* destination)
{
    uint64_t* to = (uint64_t*)destination;
    uint64_t* from = (uint64_t*)source;
    for (uint64_t i = 0; i < (GOP->Mode->Info->HorizontalResolution * GOP->Mode->Info->VerticalResolution) / 2; i++)
    {
        *to++ = *from++;
    }
}

void drawCharacter(CHAR16 character, uint32_t x, uint32_t y, EFI_GRAPHICS_OUTPUT_BLT_PIXEL colour)
{
    EFI_GRAPHICS_OUTPUT_BLT_PIXEL* address = videoBuffer + y * GOP->Mode->Info->HorizontalResolution + x;
    uint8_t* glyph = font + ((uint32_t*)font)[2] + ((uint32_t*)font)[5] * character;
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
        address += GOP->Mode->Info->HorizontalResolution - 16;
    }
}

void drawString(const CHAR16* string, uint32_t x, uint32_t y, EFI_GRAPHICS_OUTPUT_BLT_PIXEL colour)
{
    EFI_GRAPHICS_OUTPUT_BLT_PIXEL* address = videoBuffer + y * GOP->Mode->Info->HorizontalResolution + x;
    while (*string != 0)
    {
        uint8_t* glyph = font + 32 + 64 * *string++;
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
            address += GOP->Mode->Info->HorizontalResolution - 16;
        }
        address -= GOP->Mode->Info->HorizontalResolution * 32 - 16;
    }
}

void fillScreen(EFI_GRAPHICS_OUTPUT_BLT_PIXEL color)
{
    EFI_GRAPHICS_OUTPUT_BLT_PIXEL colours[2];
    colours[0] = color;
    colours[1] = color;
    uint64_t* buffer = (uint64_t*)videoBuffer;
    for (uint64_t i = 0; i < (GOP->Mode->Info->HorizontalResolution * GOP->Mode->Info->VerticalResolution) / 2; i++)
    {
        *buffer++ = *(uint64_t*)colours;
    }
}

void drawRectangle(uint32_t x, uint32_t y, uint32_t width, uint32_t height, EFI_GRAPHICS_OUTPUT_BLT_PIXEL color)
{
    EFI_GRAPHICS_OUTPUT_BLT_PIXEL* address = videoBuffer + y * GOP->Mode->Info->HorizontalResolution + x;
    for (uint32_t y = 0; y < height; y++)
    {
        for (uint32_t x = 0; x < width; x++)
        {
            *address++ = color;
        }
        address += GOP->Mode->Info->HorizontalResolution - width;
    }
}

void drawImage(uint32_t x, uint32_t y, uint32_t width, uint32_t height, EFI_GRAPHICS_OUTPUT_BLT_PIXEL* buffer)
{
    EFI_GRAPHICS_OUTPUT_BLT_PIXEL* from = buffer;
    EFI_GRAPHICS_OUTPUT_BLT_PIXEL* to = videoBuffer + y * GOP->Mode->Info->HorizontalResolution + x;
    for (uint32_t y = 0; y < height; y++)
    {
        for (uint32_t x = 0; x < width; x++)
        {
            *to++ = *from++;
        }
        to += GOP->Mode->Info->HorizontalResolution - width;
    }
}

void waitForKey()
{
    waitKey = TRUE;
    while (waitKey);
}

void waitForPit()
{
    waitPit = TRUE;
    while (waitPit);
}

EFI_STATUS efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE* SystemTable)
{
    InitializeLib(ImageHandle, SystemTable);
    LibLocateProtocol(&GraphicsOutputProtocol, (void**)&GOP);
    uefi_call_wrapper(GOP->SetMode, 2, GOP, 0);
    videoBuffer = AllocateZeroPool(GOP->Mode->FrameBufferSize);
    blit(videoBuffer, (EFI_GRAPHICS_OUTPUT_BLT_PIXEL*)GOP->Mode->FrameBufferBase);
    EFI_LOADED_IMAGE* image = NULL;
    uefi_call_wrapper(BS->HandleProtocol, 3, ImageHandle, &LoadedImageProtocol, (void**)&image);
    EFI_FILE_HANDLE fs = LibOpenRoot(image->DeviceHandle);
    EFI_FILE_HANDLE file = NULL;
    uefi_call_wrapper(fs->Open, 5, fs, &file, L"font.psf", EFI_FILE_MODE_READ, EFI_FILE_READ_ONLY | EFI_FILE_HIDDEN | EFI_FILE_SYSTEM);
    EFI_FILE_INFO* info = LibFileInfo(file);
    uint64_t fontSize = info->FileSize;
    FreePool(info);
    font = AllocatePool(fontSize);
    uefi_call_wrapper(file->Read, 3, file, &fontSize, font);
    uefi_call_wrapper(file->Close, 1, file);
    uefi_call_wrapper(fs->Open, 5, fs, &file, L"wallpaper.bmp", EFI_FILE_MODE_READ, EFI_FILE_READ_ONLY | EFI_FILE_HIDDEN | EFI_FILE_SYSTEM);
    info = LibFileInfo(file);
    uint64_t wallpaperSize = info->FileSize;
    FreePool(info);
    uint8_t* wallpaperFile = AllocatePool(wallpaperSize);
    uefi_call_wrapper(file->Read, 3, file, &wallpaperSize, wallpaperFile);
    uefi_call_wrapper(file->Close, 1, file);
    wallpaper = AllocatePool(GOP->Mode->FrameBufferSize);
    EFI_GRAPHICS_OUTPUT_BLT_PIXEL* buffer = wallpaper;
    uint8_t* fileBuffer = wallpaperFile + 0x36;
    for (uint32_t y = 0; y < GOP->Mode->Info->VerticalResolution; y++)
    {
        for (uint32_t x = 0; x < GOP->Mode->Info->HorizontalResolution; x++)
        {
            EFI_GRAPHICS_OUTPUT_BLT_PIXEL pixel;
            uint32_t newX = (1280 * ((((uint64_t)x) * 100000000) / GOP->Mode->Info->HorizontalResolution)) / 100000000;
            uint32_t newY = (800 * ((((uint64_t)y) * 100000000) / GOP->Mode->Info->VerticalResolution)) / 100000000;
            uint64_t index = ((799 - newY) * 1280 + newX) * 3;
            pixel.Blue = fileBuffer[index];
            pixel.Green = fileBuffer[index + 1];
            pixel.Red = fileBuffer[index + 2];
            *buffer++ = pixel;
        }
    }
    FreePool(wallpaperFile);
    uefi_call_wrapper(fs->Open, 5, fs, &file, L"test.bmp", EFI_FILE_MODE_READ, EFI_FILE_READ_ONLY | EFI_FILE_HIDDEN | EFI_FILE_SYSTEM);
    info = LibFileInfo(file);
    uint64_t bmpSize = info->FileSize;
    FreePool(info);
    uint8_t* bmpFile = AllocatePool(bmpSize);
    uefi_call_wrapper(file->Read, 3, file, &bmpSize, bmpFile);
    uefi_call_wrapper(file->Close, 1, file);
    uint16_t pixel = 0;
    fileBuffer = bmpFile + 0x36;
    for (uint8_t y = 0; y < 24; y++)
    {
        for (uint8_t x = 0; x < 24; x++)
        {
            uint64_t index = ((23 - y) * 24 + x) * 3;
            programs[0].icon[pixel].Blue = fileBuffer[index];
            programs[0].icon[pixel].Green = fileBuffer[index + 1];
            programs[0].icon[pixel].Red = fileBuffer[index + 2];
            pixel++;
        }
    }
    FreePool(bmpFile);
    uefi_call_wrapper(fs->Open, 5, fs, &file, L"test.bin", EFI_FILE_MODE_READ, EFI_FILE_READ_ONLY | EFI_FILE_HIDDEN | EFI_FILE_SYSTEM);
    info = LibFileInfo(file);
    uint64_t testSize = info->FileSize;
    FreePool(info);
    programs[0].start = AllocatePool(testSize);
    programs[0].update = programs[0].start + 5;
    programs[0].stop = programs[0].start + 10;
    programs[0].running = FALSE;
    uefi_call_wrapper(file->Read, 3, file, &testSize, programs[0].start);
    uefi_call_wrapper(file->Close, 1, file);
    UINTN entries;
    UINTN key;
    UINTN size;
    UINT32 version;
    uint8_t* map = (uint8_t*)LibMemoryMap(&entries, &key, &size, &version);
    for (UINTN i = 0; i < entries; i++)
    {
        EFI_MEMORY_DESCRIPTOR* iterator = (EFI_MEMORY_DESCRIPTOR*)(map + i * size);
        if (iterator->Type == EfiConventionalMemory)
        {
            memory = (void*)iterator->PhysicalStart;
            allocated = (BOOLEAN*)((iterator->PhysicalStart + iterator->NumberOfPages * EFI_PAGE_SIZE) - 1);
        }
    }
    uefi_call_wrapper(BS->ExitBootServices, 2, ImageHandle, key);
    uint64_t gdt[3];
    gdt[0] = 0;
    gdt[1] = ((uint64_t)1 << 44) | ((uint64_t)1 << 47) | ((uint64_t)1 << 41) | ((uint64_t)1 << 43) | ((uint64_t)1 << 53);
    gdt[2] = ((uint64_t)1 << 44) | ((uint64_t)1 << 47) | ((uint64_t)1 << 41);
    struct {
        uint16_t length;
        uint64_t base;
    } __attribute__((packed)) gdtr;
    gdtr.length = 23;
    gdtr.base = (uint64_t)gdt;
    __asm__ volatile ("lgdt %0" : : "m"(gdtr));
    __asm__ ("pushq $0x08; leaq completed(%rip), %rax; pushq %rax; retfq");
    return EFI_SUCCESS;
}

static inline void outb(uint16_t port, uint8_t value)
{
    __asm__ volatile ("outb %b0, %w1" : : "a"(value), "Nd"(port) : "memory");
}

static inline uint8_t inb(uint16_t port)
{
    uint8_t value = 0;
    __asm__ volatile ("inb %w1, %b0" : "=a"(value) : "Nd"(port) : "memory");
    return value;
}

void unmaskInterrupt(uint8_t interrupt)
{
    if (interrupt < 8)
    {
        outb(0x21, inb(0x21) & ~(1 << interrupt));
    }
    else
    {
        outb(0xA1, inb(0xA1) & ~(1 << (interrupt - 8)));
    }
}

void installInterrupt(uint8_t interrupt, void* handler, BOOLEAN hardware)
{
    uint16_t index = (hardware ? 0x20 : 0) + interrupt;
    idt[index].lower = (uint64_t)handler;
    idt[index].selector = 0x08;
    idt[index].ist = 0;
    idt[index].attributes = 0x8E;
    idt[index].middle = (uint64_t)handler >> 16;
    idt[index].higher = (uint64_t)handler >> 32;
    idt[index].zero = 0;
    if (hardware)
    {
        unmaskInterrupt(interrupt);
    }
}

__attribute__((interrupt)) void pit(struct InterruptFrame* frame)
{
    waitPit = FALSE;
    outb(0x20, 0x20);
}

void keyPress(uint8_t scancode, BOOLEAN pressed);

__attribute__((interrupt)) void keyboard(struct InterruptFrame* frame)
{
    uint8_t scancode = inb(0x60);
    BOOLEAN unpressed = scancode & 0b10000000;
    if (unpressed)
    {
        scancode = scancode & 0b01111111;
    }
    keyPress(scancode, unpressed);
    outb(0x20, 0x20);
}

void mouseMove(int8_t x, int8_t y);

void mouseClick(BOOLEAN left, BOOLEAN unpressed);

__attribute__((interrupt)) void mouse(struct InterruptFrame* frame)
{
    mouseBytes[mouseCycle] = inb(0x60);
    mouseCycle++;
    if (mouseCycle == 3)
    {
        mouseCycle = 0;
        mouseMove(mouseBytes[1], mouseBytes[2]);
        BOOLEAN leftClick = mouseBytes[0] & 0b00000001;
        if (leftClick != lastLeftClick)
        {
            mouseClick(TRUE, lastLeftClick);
            lastLeftClick = leftClick;
        }
        BOOLEAN rightClick = mouseBytes[0] & 0b00000010;
        if (rightClick != lastRightClick)
        {
            mouseClick(FALSE, lastRightClick);
            lastRightClick = rightClick;
        }
    }
    outb(0xA0, 0x20);
    outb(0x20, 0x20);
}

uint64_t syscallHandle(uint64_t code, uint64_t arg1, uint64_t arg2, uint64_t arg3);

void start();

void completed()
{
    __asm__ volatile ("movw $0x10, %ax; movw %ax, %ds; movw %ax, %es; movw %ax, %fs; movw %ax, %gs; movw %ax, %ss");
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
    unmaskInterrupt(2);
    outb(0x43, 0x34);
    uint16_t divisor = 1193180 / 60;
    outb(0x40, divisor & 0xFF);
    outb(0x40, (divisor >> 8) & 0xFF);
    installInterrupt(0, pit, TRUE);
    installInterrupt(1, keyboard, TRUE);
    outb(0x64, 0xA8);
    outb(0x64, 0x20);
    uint8_t status = inb(0x60) | 2;
    outb(0x64, 0x60);
    outb(0x60, status);
    outb(0x64, 0xD4);
    outb(0x60, 0xF6);
    inb(0x60);
    outb(0x64, 0xD4);
    outb(0x60, 0xF4);
    inb(0x60);
    installInterrupt(12, mouse, TRUE);
    installInterrupt(0x80, syscallHandler, FALSE);
    struct {
        uint16_t length;
        uint64_t base;
    } __attribute__((packed)) idtr;
    idtr.length = 4095;
    idtr.base = (uint64_t)idt;
    __asm__ volatile ("lidt %0" : : "m"(idtr));
    __asm__ volatile ("sti");
    start();
    while (1);
}
