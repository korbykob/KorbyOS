#include <efi.h>
#include <efilib.h>

extern void syscallHandler();

EFI_MEMORY_DESCRIPTOR* descriptor;
uint64_t allocated = 0;
EFI_GRAPHICS_OUTPUT_PROTOCOL* GOP = NULL;
EFI_GRAPHICS_OUTPUT_BLT_PIXEL* videoBuffer = NULL;
uint8_t* font = NULL;
void (*test)() = NULL;
EFI_GRAPHICS_OUTPUT_BLT_PIXEL* wallpaper = NULL;
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
struct interruptFrame
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
int64_t mouseX = 0;
int64_t mouseY = 0;
BOOLEAN leftClickDebounce = FALSE;
struct Button
{
    uint32_t x;
    uint32_t y;
    uint32_t width;
    uint32_t height;
    void* action;
};
struct Button buttonsBuffer[100];
uint8_t buttonCountBuffer = 0;
struct Button buttons[100];
uint8_t buttonCount = 0;

void* alloc(uint64_t amount)
{
    void* value = (void*)(descriptor->PhysicalStart + allocated);
    allocated += amount;
    return value;
}

void blit(void* source, void* destination)
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

void startButtons()
{
    buttonCountBuffer = 0;
}

void registerButton(struct Button* button)
{
    buttonsBuffer[buttonCountBuffer] = *button;
    buttonCountBuffer++;
}

void endButtons()
{
    for (uint8_t i = 0; i < buttonCountBuffer; i++)
    {
        buttons[i] = buttonsBuffer[i];
    }
    buttonCount = buttonCountBuffer;
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
    videoBuffer = AllocateZeroPool(GOP->Mode->FrameBufferSize);
    blit(videoBuffer, (void*)GOP->Mode->FrameBufferBase);
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
    uefi_call_wrapper(fs->Open, 5, fs, &file, L"test.bin", EFI_FILE_MODE_READ, EFI_FILE_READ_ONLY | EFI_FILE_HIDDEN | EFI_FILE_SYSTEM);
    info = LibFileInfo(file);
    uint64_t testSize = info->FileSize;
    FreePool(info);
    test = AllocatePool(testSize);
    uefi_call_wrapper(file->Read, 3, file, &testSize, test);
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
            descriptor = iterator;
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
    uint8_t value;
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

__attribute__((interrupt)) void pit(struct interruptFrame* frame)
{
    waitPit = FALSE;
    outb(0x20, 0x20);
}

void keyPress(uint8_t scancode, BOOLEAN pressed);

__attribute__((interrupt)) void keyboard(struct interruptFrame* frame)
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

__attribute__((interrupt)) void mouse(struct interruptFrame* frame)
{
    mouseBytes[mouseCycle] = inb(0x60);
    mouseCycle++;
    if (mouseCycle == 3)
    {
        mouseCycle = 0;
        BOOLEAN leftClick = mouseBytes[0] & 0b00000001;
        mouseX += mouseBytes[1];
        if (mouseX < 0)
        {
            mouseX = 0;
        }
        if (mouseX > GOP->Mode->Info->HorizontalResolution - 16)
        {
            mouseX = GOP->Mode->Info->HorizontalResolution - 16;
        }
        mouseY -= mouseBytes[2];
        if (mouseY < 0)
        {
            mouseY = 0;
        }
        if (mouseY > GOP->Mode->Info->VerticalResolution - 16)
        {
            mouseY = GOP->Mode->Info->VerticalResolution - 16;
        }
        if (leftClick && !leftClickDebounce)
        {
            for (uint8_t i = 0; i < buttonCount; i++)
            {
                if (mouseX >= buttons[i].x && mouseX < buttons[i].x + buttons[i].width && mouseY >= buttons[i].y && mouseY < buttons[i].y + buttons[i].height)
                {
                    ((void (*)())buttons[i].action)();
                }
            }
        }
        leftClickDebounce = leftClick;
    }
    outb(0xA0, 0x20);
    outb(0x20, 0x20);
}

uint64_t syscallHandle(uint64_t code, uint64_t arg);

void start();

void completed()
{
    __asm__ ("movw $0x10, %ax; movw %ax, %ds; movw %ax, %es; movw %ax, %fs; movw %ax, %gs; movw %ax, %ss");
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
