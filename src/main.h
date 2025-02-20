#include "shared.h"

extern void syscallHandler();

uint64_t memory = 0;
typedef struct
{
    BOOLEAN present;
    uint64_t start;
    uint64_t end;
} Allocation;
Allocation* allocated = NULL;
uint64_t allocations = 0;
EFI_GRAPHICS_OUTPUT_PROTOCOL* GOP = NULL;
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
typedef struct
{
    uint64_t ip;
    uint64_t cs;
    uint64_t flags;
    uint64_t sp;
    uint64_t ss;
} __attribute__((packed)) InterruptFrame;
uint64_t hpetCounter = 0;
uint8_t mouseCycle = 2;
uint8_t mouseBytes[3];
BOOLEAN lastLeftClick = FALSE;
BOOLEAN lastRightClick = FALSE;
typedef struct {
    void* next;
    CHAR16* name;
    uint64_t size;
    uint8_t* data;
} File;
File* files = NULL;
typedef struct
{
    char signature[4];
    uint32_t length;
    uint8_t revision;
    uint8_t checksum;
    char oemId[6];
    char oemTableID[8];
    uint32_t oemRevision;
    uint32_t creatorID;
    uint32_t creatorRevision;
} __attribute__ ((packed)) AcpiSdtHeader;
typedef struct
{
    AcpiSdtHeader header;
    AcpiSdtHeader* entries[];
} __attribute__ ((packed)) Xsdt;
typedef struct 
{
    AcpiSdtHeader header;
    uint32_t blockId;
    uint32_t gas;
    uint64_t address;
    uint8_t number;
    uint16_t minimum;
    uint8_t protection;
} __attribute__((packed)) Hpet;
uint64_t hpetAddress = 0;

void* allocate(uint64_t amount)
{
    uint64_t value = memory;
    Allocation* allocation = allocated;
    uint64_t count = 0;
    while (count != allocations)
    {
        if (allocation->present)
        {
            count++;
            if (value >= allocation->end)
            {
                Allocation* test = allocation - 1;
                while (!test->present && count != allocations)
                {
                    count++;
                    test--;
                }
                if (count == allocations || value + amount - 1 < test->start)
                {
                    break;
                }
                allocation = test;
            }
            else
            {
                value = allocation->end;
                allocation--;
            }
        }
        else
        {
            allocation--;
        }
    }
    allocation->present = TRUE;
    allocation->start = value;
    allocation->end = value + amount;
    allocations++;
    return (void*)value;
}

void unallocate(void* pointer)
{
    Allocation* test = allocated;
    while (!test->present || test->start != (uint64_t)pointer)
    {
        test--;
    }
    test->present = FALSE;
    allocations--;
}

EFI_STATUS efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE* SystemTable)
{
    InitializeLib(ImageHandle, SystemTable);
    for (uint64_t i = 0; i < ST->NumberOfTableEntries; i++)
    {
        EFI_GUID guid = ACPI_20_TABLE_GUID;
        if (CompareGuid(&ST->ConfigurationTable[i].VendorGuid, &guid) == 0)
        {
            Xsdt* xsdt = *(Xsdt**)(ST->ConfigurationTable[i].VendorTable + 24);
            for (uint32_t table = 0; table < (xsdt->header.length - sizeof(AcpiSdtHeader)) / sizeof(AcpiSdtHeader*); table++)
            {
                if (strncmpa(xsdt->entries[table]->signature, "HPET", 4) == 0)
                {
                    hpetAddress = ((Hpet*)xsdt->entries[table])->address;
                    break;
                }
            }
            break;
        }
    }
    LibLocateProtocol(&GraphicsOutputProtocol, (void**)&GOP);
    uefi_call_wrapper(GOP->SetMode, 2, GOP, 0);
    EFI_LOADED_IMAGE* image = NULL;
    uefi_call_wrapper(BS->HandleProtocol, 3, ImageHandle, &LoadedImageProtocol, (void**)&image);
    EFI_FILE_HANDLE fs = LibOpenRoot(image->DeviceHandle);
    EFI_FILE_HANDLE file = NULL;
    uefi_call_wrapper(fs->Open, 5, fs, &file, L"wallpapers\\wallpaper.bmp", EFI_FILE_MODE_READ, EFI_FILE_READ_ONLY | EFI_FILE_HIDDEN | EFI_FILE_SYSTEM);
    EFI_FILE_INFO* info = LibFileInfo(file);
    uint64_t wallpaperSize = info->FileSize;
    FreePool(info);
    uint8_t* wallpaperFile = AllocatePool(wallpaperSize);
    uefi_call_wrapper(file->Read, 3, file, &wallpaperSize, wallpaperFile);
    uefi_call_wrapper(file->Close, 1, file);
    wallpaper = AllocatePool(GOP->Mode->FrameBufferSize);
    EFI_GRAPHICS_OUTPUT_BLT_PIXEL* buffer = wallpaper;
    uint8_t* fileBuffer = wallpaperFile + 0x36;
    int32_t width = *(int32_t*)(wallpaperFile + 0x12);
    int32_t height = *(int32_t*)(wallpaperFile + 0x16);
    for (uint32_t y = 0; y < GOP->Mode->Info->VerticalResolution; y++)
    {
        for (uint32_t x = 0; x < GOP->Mode->Info->HorizontalResolution; x++)
        {
            uint32_t newX = width * (x / (float)GOP->Mode->Info->HorizontalResolution);
            uint32_t newY = height * (y / (float)GOP->Mode->Info->VerticalResolution);
            uint64_t index = ((height - newY - 1) * width + newX) * 3;
            buffer->Blue = fileBuffer[index];
            buffer->Green = fileBuffer[index + 1];
            buffer->Red = fileBuffer[index + 2];
            buffer++;
        }
    }
    FreePool(wallpaperFile);
    uefi_call_wrapper(fs->Open, 5, fs, &file, L"fonts\\font.psf", EFI_FILE_MODE_READ, EFI_FILE_READ_ONLY | EFI_FILE_HIDDEN | EFI_FILE_SYSTEM);
    info = LibFileInfo(file);
    uint64_t fontSize = info->FileSize;
    FreePool(info);
    uint8_t* font = AllocatePool(fontSize);
    uefi_call_wrapper(file->Read, 3, file, &fontSize, font);
    uefi_call_wrapper(file->Close, 1, file);
    uefi_call_wrapper(fs->Open, 5, fs, &file, L"programs\\test\\test.bin", EFI_FILE_MODE_READ, EFI_FILE_READ_ONLY | EFI_FILE_HIDDEN | EFI_FILE_SYSTEM);
    info = LibFileInfo(file);
    uint64_t testSize = info->FileSize;
    FreePool(info);
    uint8_t* test = AllocatePool(testSize);
    uefi_call_wrapper(file->Read, 3, file, &testSize, test);
    uefi_call_wrapper(file->Close, 1, file);
    uefi_call_wrapper(fs->Open, 5, fs, &file, L"programs\\test\\test.bmp", EFI_FILE_MODE_READ, EFI_FILE_READ_ONLY | EFI_FILE_HIDDEN | EFI_FILE_SYSTEM);
    info = LibFileInfo(file);
    uint64_t bmpSize = info->FileSize;
    FreePool(info);
    uint8_t* bmp = AllocatePool(bmpSize);
    uefi_call_wrapper(file->Read, 3, file, &bmpSize, bmp);
    uefi_call_wrapper(file->Close, 1, file);
    uefi_call_wrapper(fs->Open, 5, fs, &file, L"programs\\test\\wall.bmp", EFI_FILE_MODE_READ, EFI_FILE_READ_ONLY | EFI_FILE_HIDDEN | EFI_FILE_SYSTEM);
    info = LibFileInfo(file);
    uint64_t wallSize = info->FileSize;
    FreePool(info);
    uint8_t* wall = AllocatePool(wallSize);
    uefi_call_wrapper(file->Read, 3, file, &wallSize, wall);
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
            memory = iterator->PhysicalStart;
            allocated = (Allocation*)((iterator->PhysicalStart + iterator->NumberOfPages * EFI_PAGE_SIZE) - sizeof(Allocation));
            allocated->present = FALSE;
        }
    }
    uefi_call_wrapper(BS->ExitBootServices, 2, ImageHandle, key);
    File* newFile = addItem((void**)&files, sizeof(File));
    newFile->name = L"fonts/font.psf";
    newFile->size = fontSize;
    newFile->data = font;
    newFile = addItem((void**)&files, sizeof(File));
    newFile->name = L"programs/test/test.bin";
    newFile->size = testSize;
    newFile->data = test;
    newFile = addItem((void**)&files, sizeof(File));
    newFile->name = L"programs/test/test.bmp";
    newFile->size = bmpSize;
    newFile->data = bmp;
    newFile = addItem((void**)&files, sizeof(File));
    newFile->name = L"programs/test/wall.bmp";
    newFile->size = wallSize;
    newFile->data = wall;
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

void* writeFile(const CHAR16* name, uint64_t size)
{
    File* file = NULL;
    File* iterator = (File*)&files;
    while (iterateList((void**)&iterator))
    {
        if (StrCmp(name, iterator->name) == 0)
        {
            file = iterator;
            unallocate(file->data);
            break;
        }
    }
    if (!file)
    {
        file = addItem((void**)&files, sizeof(File));
        file->name = allocate((StrLen(name) + 1) * 2);
        StrCpy(file->name, name);
    }
    file->size = size;
    file->data = allocate(size);
    return file->data;
}

uint8_t* readFile(const CHAR16* name, uint64_t* size)
{
    File* file = (File*)&files;
    while (iterateList((void**)&file))
    {
        if (StrCmp(name, file->name) == 0)
        {
            if (size)
            {
                *size = file->size;
            }
            return file->data;
        }
    }
    return NULL;
}

void deleteFile(const CHAR16* name)
{
    File* file = (File*)&files;
    while (iterateList((void**)&file))
    {
        if (StrCmp(name, file->name) == 0)
        {
            removeItem((void**)&files, file);
            unallocate(file->name);
            unallocate(file->data);
            break;
        }
    }
}

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

__attribute__((interrupt, target("general-regs-only"))) void hpet(InterruptFrame* frame)
{
    hpetCounter++;
    outb(0x20, 0x20);
}

void keyPress(uint8_t scancode, BOOLEAN pressed);

__attribute__((interrupt, target("general-regs-only"))) void keyboard(InterruptFrame* frame)
{
    uint8_t scancode = inb(0x60);
    BOOLEAN unpressed = scancode & 0b10000000;
    if (unpressed)
    {
        scancode = scancode & 0b01111111;
    }
    keyPress(scancode, !unpressed);
    outb(0x20, 0x20);
}

void mouseMove(int16_t x, int16_t y);

void mouseClick(BOOLEAN left, BOOLEAN pressed);

__attribute__((interrupt, target("general-regs-only"))) void mouse(InterruptFrame* frame)
{
    mouseBytes[mouseCycle] = inb(0x60);
    mouseCycle++;
    if (mouseCycle == 3)
    {
        mouseCycle = 0;
        int16_t x = mouseBytes[1] - ((mouseBytes[0] << 4) & 0x100);
        int16_t y = mouseBytes[2] - ((mouseBytes[0] << 3) & 0x100);
        if (x != 0 || y != 0)
        {
            mouseMove(x, y);
        }
        BOOLEAN leftClick = mouseBytes[0] & 0b00000001;
        if (leftClick != lastLeftClick)
        {
            mouseClick(TRUE, !lastLeftClick);
            lastLeftClick = leftClick;
        }
        BOOLEAN rightClick = mouseBytes[0] & 0b00000010;
        if (rightClick != lastRightClick)
        {
            mouseClick(FALSE, !lastRightClick);
            lastRightClick = rightClick;
        }
    }
    outb(0xA0, 0x20);
    outb(0x20, 0x20);
}

uint64_t syscallHandle(uint64_t code, uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4);

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
    *(uint32_t*)(hpetAddress + 0x10) |= 0b11;
    *(uint32_t*)(hpetAddress + 0x100) |= 0b1100;
    *(uint64_t*)(hpetAddress + 0x108) = (1000000000000000ULL / ((*(uint64_t*)hpetAddress >> 32) & 0xFFFFFFFF)) / 60;
    *(uint64_t*)(hpetAddress + 0xF0) = 0;
    installInterrupt(0, hpet, TRUE);
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
