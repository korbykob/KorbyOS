#include "shared.h"

uint64_t memorySize = 0;
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
uint8_t mouseCycle = 1;
uint8_t mouseBytes[3];
BOOLEAN lastLeftClick = FALSE;
BOOLEAN lastRightClick = FALSE;
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
typedef struct 
{
    AcpiSdtHeader header;
    uint32_t address;
    uint32_t flags;
} __attribute__((packed)) Madt;
uint64_t apicAddress = 0;
uint64_t cpuCount = 0;
uint8_t* cpus = NULL;
uint64_t soundDuration = 0;

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
    debug("Locating GOP protocol");
    LibLocateProtocol(&GraphicsOutputProtocol, (void**)&GOP);
    debug("Resetting GOP");
    uefi_call_wrapper(GOP->SetMode, 2, GOP, 0);
    uefi_call_wrapper(ST->ConOut->SetCursorPosition, 3, ST->ConOut, 0, 0);
    debug("Displaying resolution prompt");
    Print(L"Use the up and down arrow keys to move.\nPress enter to select and boot using the selected resolution.\n\nPlease select a resolution:\n");
    for (uint32_t i = 0; i < GOP->Mode->MaxMode; i++)
    {
        uint64_t size = 0;
        EFI_GRAPHICS_OUTPUT_MODE_INFORMATION* info = NULL;
        uefi_call_wrapper(GOP->QueryMode, 4, GOP, i, &size, &info);
        Print(L"%dx%d\n", info->HorizontalResolution, info->VerticalResolution);
    }
    debug("Disabling watchdog timer");
    uefi_call_wrapper(BS->SetWatchdogTimer, 4, 0, 0, 0, NULL);
    debug("Waiting for user to choose resolution");
    uint32_t selected = 0;
    EFI_INPUT_KEY pressed;
    pressed.ScanCode = 1;
    pressed.UnicodeChar = '\0';
    while (pressed.UnicodeChar != '\r')
    {
        if (pressed.ScanCode == 1 || pressed.ScanCode == 2)
        {
            uefi_call_wrapper(ST->ConOut->SetCursorPosition, 3, ST->ConOut, 10, selected + 4);
            Print(L"   ");
            if (pressed.ScanCode == 1 && selected != 0)
            {
                selected--;
            }
            else if (pressed.ScanCode == 2 && selected != GOP->Mode->MaxMode - 1)
            {
                selected++;
            }
            uefi_call_wrapper(ST->ConOut->SetCursorPosition, 3, ST->ConOut, 10, selected + 4);
            Print(L"<--");
        }
        WaitForSingleEvent(ST->ConIn->WaitForKey, 0);
        uefi_call_wrapper(ST->ConIn->ReadKeyStroke, 2, ST->ConIn, &pressed);
    }
    debug("Switching GOP mode");
    uefi_call_wrapper(GOP->SetMode, 2, GOP, selected);
    debug("Searching tables");
    EFI_GUID guid = ACPI_20_TABLE_GUID;
    for (uint64_t i = 0; i < ST->NumberOfTableEntries; i++)
    {
        if (CompareGuid(&ST->ConfigurationTable[i].VendorGuid, &guid) == 0)
        {
            debug("Found ACPI 2.0 table");
            Xsdt* xsdt = *(Xsdt**)(ST->ConfigurationTable[i].VendorTable + 24);
            debug("Searching ACPI tables");
            for (uint32_t table = 0; table < (xsdt->header.length - sizeof(AcpiSdtHeader)) / sizeof(AcpiSdtHeader*); table++)
            {
                if (strncmpa(xsdt->entries[table]->signature, "HPET", 4) == 0)
                {
                    debug("Found HPET");
                    hpetAddress = ((Hpet*)xsdt->entries[table])->address;
                }
                else if (strncmpa(xsdt->entries[table]->signature, "APIC", 4) == 0)
                {
                    debug("Found APIC table");
                    apicAddress = ((Madt*)xsdt->entries[table])->address;
                    uint8_t bsp = *(uint32_t*)(apicAddress + 0x20) >> 24;
                    uint8_t* record = ((uint8_t*)xsdt->entries[table]) + 44;
                    debug("Searching for cpu cores");
                    while ((uint64_t)record - (uint64_t)xsdt->entries[table] != xsdt->entries[table]->length)
                    {
                        if (*record == 0 && *(record + 3) != bsp)
                        {
                            debug("Found core");
                            cpuCount++;
                        }
                        record += *(record + 1);
                    }
                    cpus = AllocatePool(cpuCount);
                    record = ((uint8_t*)xsdt->entries[table]) + 44;
                    uint64_t cpu = 0;
                    debug("Saving core IDs");
                    while ((uint64_t)record - (uint64_t)xsdt->entries[table] != xsdt->entries[table]->length)
                    {
                        if (*record == 0 && *(record + 3) != bsp)
                        {
                            cpus[cpu] = *(record + 3);
                            cpu++;
                        }
                        record += *(record + 1);
                    }
                }
            }
            break;
        }
    }
    debug("Locating LIP protocol");
    EFI_LOADED_IMAGE* image = NULL;
    uefi_call_wrapper(BS->HandleProtocol, 3, ImageHandle, &LoadedImageProtocol, (void**)&image);
    debug("Opening root file system");
    EFI_FILE_HANDLE fs = LibOpenRoot(image->DeviceHandle);
    EFI_FILE_HANDLE file = NULL;
    debug("Loading /system/smp.bin");
    uefi_call_wrapper(fs->Open, 5, fs, &file, L"system\\smp.bin", EFI_FILE_MODE_READ, EFI_FILE_READ_ONLY | EFI_FILE_HIDDEN | EFI_FILE_SYSTEM);
    EFI_FILE_INFO* info = LibFileInfo(file);
    uint64_t smpSize = info->FileSize;
    FreePool(info);
    uint8_t* smp = AllocatePool(smpSize);
    uefi_call_wrapper(file->Read, 3, file, &smpSize, smp);
    uefi_call_wrapper(file->Close, 1, file);
    debug("Loading /fonts/font.psf");
    uefi_call_wrapper(fs->Open, 5, fs, &file, L"fonts\\font.psf", EFI_FILE_MODE_READ, EFI_FILE_READ_ONLY | EFI_FILE_HIDDEN | EFI_FILE_SYSTEM);
    info = LibFileInfo(file);
    uint64_t fontSize = info->FileSize;
    FreePool(info);
    uint8_t* font = AllocatePool(fontSize);
    uefi_call_wrapper(file->Read, 3, file, &fontSize, font);
    uefi_call_wrapper(file->Close, 1, file);
    debug("Loading /programs/test/program.bin");
    uefi_call_wrapper(fs->Open, 5, fs, &file, L"programs\\test\\program.bin", EFI_FILE_MODE_READ, EFI_FILE_READ_ONLY | EFI_FILE_HIDDEN | EFI_FILE_SYSTEM);
    info = LibFileInfo(file);
    uint64_t testSize = info->FileSize;
    FreePool(info);
    uint8_t* test = AllocatePool(testSize);
    uefi_call_wrapper(file->Read, 3, file, &testSize, test);
    uefi_call_wrapper(file->Close, 1, file);
    debug("Loading /programs/desktop/program.bin");
    uefi_call_wrapper(fs->Open, 5, fs, &file, L"programs\\desktop\\program.bin", EFI_FILE_MODE_READ, EFI_FILE_READ_ONLY | EFI_FILE_HIDDEN | EFI_FILE_SYSTEM);
    info = LibFileInfo(file);
    uint64_t desktopSize = info->FileSize;
    FreePool(info);
    uint8_t* desktop = AllocatePool(desktopSize);
    uefi_call_wrapper(file->Read, 3, file, &desktopSize, desktop);
    uefi_call_wrapper(file->Close, 1, file);
    debug("Loading /programs/desktop/wallpaper.bmp");
    uefi_call_wrapper(fs->Open, 5, fs, &file, L"programs\\desktop\\wallpaper.bmp", EFI_FILE_MODE_READ, EFI_FILE_READ_ONLY | EFI_FILE_HIDDEN | EFI_FILE_SYSTEM);
    info = LibFileInfo(file);
    uint64_t wallpaperSize = info->FileSize;
    FreePool(info);
    uint8_t* wallpaper = AllocatePool(wallpaperSize);
    uefi_call_wrapper(file->Read, 3, file, &wallpaperSize, wallpaper);
    uefi_call_wrapper(file->Close, 1, file);
    debug("Loading /programs/desktop/taskbar/rendering/program.bin");
    uefi_call_wrapper(fs->Open, 5, fs, &file, L"programs\\desktop\\taskbar\\rendering\\program.bin", EFI_FILE_MODE_READ, EFI_FILE_READ_ONLY | EFI_FILE_HIDDEN | EFI_FILE_SYSTEM);
    info = LibFileInfo(file);
    uint64_t renderingSize = info->FileSize;
    FreePool(info);
    uint8_t* rendering = AllocatePool(renderingSize);
    uefi_call_wrapper(file->Read, 3, file, &renderingSize, rendering);
    uefi_call_wrapper(file->Close, 1, file);
    debug("Loading /programs/desktop/taskbar/rendering/program.bmp");
    uefi_call_wrapper(fs->Open, 5, fs, &file, L"programs\\desktop\\taskbar\\rendering\\program.bmp", EFI_FILE_MODE_READ, EFI_FILE_READ_ONLY | EFI_FILE_HIDDEN | EFI_FILE_SYSTEM);
    info = LibFileInfo(file);
    uint64_t renderingBmpSize = info->FileSize;
    FreePool(info);
    uint8_t* renderingBmp = AllocatePool(renderingBmpSize);
    uefi_call_wrapper(file->Read, 3, file, &renderingBmpSize, renderingBmp);
    uefi_call_wrapper(file->Close, 1, file);
    debug("Loading /programs/desktop/taskbar/rendering/wall.bmp");
    uefi_call_wrapper(fs->Open, 5, fs, &file, L"programs\\desktop\\taskbar\\rendering\\wall.bmp", EFI_FILE_MODE_READ, EFI_FILE_READ_ONLY | EFI_FILE_HIDDEN | EFI_FILE_SYSTEM);
    info = LibFileInfo(file);
    uint64_t wallSize = info->FileSize;
    FreePool(info);
    uint8_t* wall = AllocatePool(wallSize);
    uefi_call_wrapper(file->Read, 3, file, &wallSize, wall);
    uefi_call_wrapper(file->Close, 1, file);
    debug("Loading /programs/desktop/taskbar/rendering/sprite.bmp");
    uefi_call_wrapper(fs->Open, 5, fs, &file, L"programs\\desktop\\taskbar\\rendering\\sprite.bmp", EFI_FILE_MODE_READ, EFI_FILE_READ_ONLY | EFI_FILE_HIDDEN | EFI_FILE_SYSTEM);
    info = LibFileInfo(file);
    uint64_t spriteSize = info->FileSize;
    FreePool(info);
    uint8_t* sprite = AllocatePool(spriteSize);
    uefi_call_wrapper(file->Read, 3, file, &spriteSize, sprite);
    uefi_call_wrapper(file->Close, 1, file);
    UINTN entries;
    UINTN key;
    UINTN size;
    UINT32 version;
    debug("Reading memory map");
    uint8_t* map = (uint8_t*)LibMemoryMap(&entries, &key, &size, &version);
    for (UINTN i = 0; i < entries; i++)
    {
        EFI_MEMORY_DESCRIPTOR* iterator = (EFI_MEMORY_DESCRIPTOR*)(map + i * size);
        if (iterator->Type == EfiConventionalMemory || iterator->Type == EfiBootServicesCode || iterator->Type == EfiBootServicesData)
        {
            uint64_t keySize = iterator->NumberOfPages * EFI_PAGE_SIZE;
            if (keySize > memorySize)
            {
                memorySize = keySize;
                memory = iterator->PhysicalStart;
                allocated = (Allocation*)((iterator->PhysicalStart + keySize) - sizeof(Allocation));
            }
        }
    }
    debug("Exiting boot services");
    uefi_call_wrapper(BS->ExitBootServices, 2, ImageHandle, key);
    debug("Adding /system");
    File* newFile = addItem((void**)&files, sizeof(File));
    newFile->name = L"/system";
    newFile->size = 0;
    newFile->data = NULL;
    debug("Adding /system/smp.bin");
    newFile = addItem((void**)&files, sizeof(File));
    newFile->name = L"/system/smp.bin";
    newFile->size = smpSize;
    newFile->data = smp;
    debug("Adding /fonts");
    newFile = addItem((void**)&files, sizeof(File));
    newFile->name = L"/fonts";
    newFile->size = 0;
    newFile->data = NULL;
    debug("Adding /fonts/font.psf");
    newFile = addItem((void**)&files, sizeof(File));
    newFile->name = L"/fonts/font.psf";
    newFile->size = fontSize;
    newFile->data = font;
    debug("Adding /programs");
    newFile = addItem((void**)&files, sizeof(File));
    newFile->name = L"/programs";
    newFile->size = 0;
    newFile->data = NULL;
    debug("Adding /programs/test");
    newFile = addItem((void**)&files, sizeof(File));
    newFile->name = L"/programs/test";
    newFile->size = 0;
    newFile->data = NULL;
    debug("Adding /programs/test/program.bin");
    newFile = addItem((void**)&files, sizeof(File));
    newFile->name = L"/programs/test/program.bin";
    newFile->size = testSize;
    newFile->data = test;
    debug("Adding /programs/desktop");
    newFile = addItem((void**)&files, sizeof(File));
    newFile->name = L"/programs/desktop";
    newFile->size = 0;
    newFile->data = NULL;
    debug("Adding /programs/desktop/program.bin");
    newFile = addItem((void**)&files, sizeof(File));
    newFile->name = L"/programs/desktop/program.bin";
    newFile->size = desktopSize;
    newFile->data = desktop;
    debug("Adding /programs/desktop/wallpaper.bmp");
    newFile = addItem((void**)&files, sizeof(File));
    newFile->name = L"/programs/desktop/wallpaper.bmp";
    newFile->size = wallpaperSize;
    newFile->data = wallpaper;
    debug("Adding /programs/desktop/taskbar");
    newFile = addItem((void**)&files, sizeof(File));
    newFile->name = L"/programs/desktop/taskbar";
    newFile->size = 0;
    newFile->data = NULL;
    debug("Adding /programs/desktop/taskbar/rendering");
    newFile = addItem((void**)&files, sizeof(File));
    newFile->name = L"/programs/desktop/taskbar/rendering";
    newFile->size = 0;
    newFile->data = NULL;
    debug("Adding /programs/desktop/taskbar/rendering/program.bin");
    newFile = addItem((void**)&files, sizeof(File));
    newFile->name = L"/programs/desktop/taskbar/rendering/program.bin";
    newFile->size = renderingSize;
    newFile->data = rendering;
    debug("Adding /programs/desktop/taskbar/rendering/program.bmp");
    newFile = addItem((void**)&files, sizeof(File));
    newFile->name = L"/programs/desktop/taskbar/rendering/program.bmp";
    newFile->size = renderingBmpSize;
    newFile->data = renderingBmp;
    debug("Adding /programs/desktop/taskbar/rendering/wall.bmp");
    newFile = addItem((void**)&files, sizeof(File));
    newFile->name = L"/programs/desktop/taskbar/rendering/wall.bmp";
    newFile->size = wallSize;
    newFile->data = wall;
    debug("Adding /programs/desktop/taskbar/rendering/sprite.bmp");
    newFile = addItem((void**)&files, sizeof(File));
    newFile->name = L"/programs/desktop/taskbar/rendering/sprite.bmp";
    newFile->size = spriteSize;
    newFile->data = sprite;
    debug("Setting up GDT");
    uint64_t gdt[3];
    gdt[0] = 0x0000000000000000;
    gdt[1] = 0x00209A0000000000;
    gdt[2] = 0x0000920000000000;
    debug("Setting up GDTR");
    struct {
        uint16_t length;
        uint64_t base;
    } __attribute__((packed)) gdtr;
    gdtr.length = 23;
    gdtr.base = (uint64_t)gdt;
    debug("Loading GDT");
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

BOOLEAN checkFile(const CHAR16* name)
{
    File* file = (File*)&files;
    while (iterateList((void**)&file))
    {
        if (StrCmp(name, file->name) == 0 && file->size != 0)
        {
            return TRUE;
        }
    }
    return FALSE;
}

BOOLEAN checkFolder(const CHAR16* name)
{
    uint64_t nameLength = StrLen(name);
    if (name[nameLength - 1] != L'/')
    {
        return FALSE;
    }
    File* file = (File*)&files;
    while (iterateList((void**)&file))
    {
        if (StrnCmp(name, file->name, nameLength) == 0)
        {
            return TRUE;
        }
    }
    return FALSE;
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

File** getFiles(const CHAR16* root, uint64_t* count, BOOLEAN recursive)
{
    uint64_t length = StrLen(root);
    File* file = (File*)&files;
    while (iterateList((void**)&file))
    {
        if (StrnCmp(file->name, root, length) == 0 && (recursive || !inString(file->name + length, L'/')))
        {
            *count = *count + 1;
        }
    }
    File** items = allocate(*count * sizeof(File*));
    uint64_t i = 0;
    file = (File*)&files;
    while (iterateList((void**)&file))
    {
        if (StrnCmp(file->name, root, length) == 0 && (recursive || !inString(file->name + length, L'/')))
        {
            items[i] = file;
            i++;
        }
    }
    return items;
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

__attribute__((target("general-regs-only"))) void panic(uint8_t isr)
{
    __asm__ volatile ("cli");
    outb(0x61, inb(0x61) & 0b11111101);
    initGraphics((EFI_GRAPHICS_OUTPUT_BLT_PIXEL*)GOP->Mode->FrameBufferBase, GOP->Mode->Info->HorizontalResolution, readFile(L"fonts/font.psf", NULL));
    drawRectangle(0, 0, GOP->Mode->Info->HorizontalResolution, GOP->Mode->Info->VerticalResolution, black);
    CHAR16 characters[100];
    ValueToString(characters, FALSE, isr);
    uint32_t x = GOP->Mode->Info->HorizontalResolution / 2 - (StrLen(characters) + 23) * 8;
    drawString(L"ISR exception occured:", x, GOP->Mode->Info->VerticalResolution / 2 - 32, white);
    drawString(characters, x + 368, GOP->Mode->Info->VerticalResolution / 2 - 32, white);
    for (uint64_t i = 0; i < strlena(lastDebug) + 1; i++)
    {
        characters[i] = lastDebug[i];
    }
    x = GOP->Mode->Info->HorizontalResolution / 2 - (StrLen(characters) + 20) * 8;
    drawString(L"Last debug message:", x, GOP->Mode->Info->VerticalResolution / 2, white);
    drawString(characters, x + 320, GOP->Mode->Info->VerticalResolution / 2, white);
    __asm__ volatile ("hlt");
}

__attribute__((target("general-regs-only"))) void panicCode(uint8_t isr, uint64_t code)
{
    __asm__ volatile ("cli");
    outb(0x61, inb(0x61) & 0b11111101);
    initGraphics((EFI_GRAPHICS_OUTPUT_BLT_PIXEL*)GOP->Mode->FrameBufferBase, GOP->Mode->Info->HorizontalResolution, readFile(L"fonts/font.psf", NULL));
    drawRectangle(0, 0, GOP->Mode->Info->HorizontalResolution, GOP->Mode->Info->VerticalResolution, black);
    CHAR16 characters[100];
    ValueToString(characters, FALSE, isr);
    uint32_t x = GOP->Mode->Info->HorizontalResolution / 2 - (StrLen(characters) + 23) * 8;
    drawString(L"ISR exception occured:", x, GOP->Mode->Info->VerticalResolution / 2 - 48, white);
    drawString(characters, x + 368, GOP->Mode->Info->VerticalResolution / 2 - 48, white);
    ValueToHex(characters, code);
    x = GOP->Mode->Info->HorizontalResolution / 2 - (StrLen(characters) + 14) * 8;
    drawString(L"Error code: 0x", x, GOP->Mode->Info->VerticalResolution / 2 - 16, white);
    drawString(characters, x + 224, GOP->Mode->Info->VerticalResolution / 2 - 16, white);
    for (uint64_t i = 0; i < strlena(lastDebug) + 1; i++)
    {
        characters[i] = lastDebug[i];
    }
    x = GOP->Mode->Info->HorizontalResolution / 2 - (StrLen(characters) + 20) * 8;
    drawString(L"Last debug message:", x, GOP->Mode->Info->VerticalResolution / 2 + 16, white);
    drawString(characters, x + 320, GOP->Mode->Info->VerticalResolution / 2 + 16, white);
    __asm__ volatile ("hlt");
}

__attribute__((interrupt, target("general-regs-only"))) void isr0(InterruptFrame* frame)
{
    panic(0);
}

__attribute__((interrupt, target("general-regs-only"))) void isr1(InterruptFrame* frame)
{
    panic(1);
}

__attribute__((interrupt, target("general-regs-only"))) void isr2(InterruptFrame* frame)
{
    panic(2);
}

__attribute__((interrupt, target("general-regs-only"))) void isr3(InterruptFrame* frame)
{
    panic(3);
}

__attribute__((interrupt, target("general-regs-only"))) void isr4(InterruptFrame* frame)
{
    panic(4);
}

__attribute__((interrupt, target("general-regs-only"))) void isr5(InterruptFrame* frame)
{
    panic(5);
}

__attribute__((interrupt, target("general-regs-only"))) void isr6(InterruptFrame* frame)
{
    panic(6);
}

__attribute__((interrupt, target("general-regs-only"))) void isr7(InterruptFrame* frame)
{
    panic(7);
}

__attribute__((interrupt, target("general-regs-only"))) void isr8(InterruptFrame* frame, uint64_t code)
{
    panicCode(8, code);
}

__attribute__((interrupt, target("general-regs-only"))) void isr9(InterruptFrame* frame)
{
    panic(9);
}

__attribute__((interrupt, target("general-regs-only"))) void isr10(InterruptFrame* frame, uint64_t code)
{
    panicCode(10, code);
}

__attribute__((interrupt, target("general-regs-only"))) void isr11(InterruptFrame* frame, uint64_t code)
{
    panicCode(11, code);
}

__attribute__((interrupt, target("general-regs-only"))) void isr12(InterruptFrame* frame, uint64_t code)
{
    panicCode(12, code);
}

__attribute__((interrupt, target("general-regs-only"))) void isr13(InterruptFrame* frame, uint64_t code)
{
    panicCode(13, code);
}

__attribute__((interrupt, target("general-regs-only"))) void isr14(InterruptFrame* frame, uint64_t code)
{
    panicCode(14, code);
}

__attribute__((interrupt, target("general-regs-only"))) void isr15(InterruptFrame* frame)
{
    panic(15);
}

__attribute__((interrupt, target("general-regs-only"))) void isr16(InterruptFrame* frame)
{
    panic(16);
}

__attribute__((interrupt, target("general-regs-only"))) void isr17(InterruptFrame* frame, uint64_t code)
{
    panicCode(17, code);
}

__attribute__((interrupt, target("general-regs-only"))) void isr18(InterruptFrame* frame)
{
    panic(18);
}

__attribute__((interrupt, target("general-regs-only"))) void isr19(InterruptFrame* frame)
{
    panic(19);
}

__attribute__((interrupt, target("general-regs-only"))) void isr20(InterruptFrame* frame)
{
    panic(20);
}

__attribute__((interrupt, target("general-regs-only"))) void isr21(InterruptFrame* frame, uint64_t code)
{
    panicCode(21, code);
}
__attribute__((interrupt, target("general-regs-only"))) void isr22(InterruptFrame* frame)
{
    panic(22);
}

__attribute__((interrupt, target("general-regs-only"))) void isr23(InterruptFrame* frame)
{
    panic(23);
}

__attribute__((interrupt, target("general-regs-only"))) void isr24(InterruptFrame* frame)
{
    panic(24);
}

__attribute__((interrupt, target("general-regs-only"))) void isr25(InterruptFrame* frame)
{
    panic(25);
}

__attribute__((interrupt, target("general-regs-only"))) void isr26(InterruptFrame* frame)
{
    panic(26);
}

__attribute__((interrupt, target("general-regs-only"))) void isr27(InterruptFrame* frame)
{
    panic(27);
}

__attribute__((interrupt, target("general-regs-only"))) void isr28(InterruptFrame* frame)
{
    panic(28);
}

__attribute__((interrupt, target("general-regs-only"))) void isr29(InterruptFrame* frame, uint64_t code)
{
    panicCode(29, code);
}

__attribute__((interrupt, target("general-regs-only"))) void isr30(InterruptFrame* frame, uint64_t code)
{
    panicCode(30, code);
}

__attribute__((interrupt, target("general-regs-only"))) void isr31(InterruptFrame* frame)
{
    panic(31);
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

__attribute__((naked)) void syscallHandler()
{
    __asm__ volatile ("cld; call syscallHandle; iretq");
}

uint64_t syscallHandle(uint64_t code, uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4, uint64_t arg5);

void start();

void completed()
{
    __asm__ volatile ("movw $0x10, %ax; movw %ax, %ds; movw %ax, %es; movw %ax, %fs; movw %ax, %gs; movw %ax, %ss");
    debug("Initialising PICs");
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
    debug("Setting up exception handlers");
    installInterrupt(0, isr0, FALSE);
    installInterrupt(1, isr1, FALSE);
    installInterrupt(2, isr2, FALSE);
    installInterrupt(3, isr3, FALSE);
    installInterrupt(4, isr4, FALSE);
    installInterrupt(5, isr5, FALSE);
    installInterrupt(6, isr6, FALSE);
    installInterrupt(7, isr7, FALSE);
    installInterrupt(8, isr8, FALSE);
    installInterrupt(9, isr9, FALSE);
    installInterrupt(10, isr10, FALSE);
    installInterrupt(11, isr11, FALSE);
    installInterrupt(12, isr12, FALSE);
    installInterrupt(13, isr13, FALSE);
    installInterrupt(14, isr14, FALSE);
    installInterrupt(15, isr15, FALSE);
    installInterrupt(16, isr16, FALSE);
    installInterrupt(17, isr17, FALSE);
    installInterrupt(18, isr18, FALSE);
    installInterrupt(19, isr19, FALSE);
    installInterrupt(20, isr20, FALSE);
    installInterrupt(21, isr21, FALSE);
    installInterrupt(22, isr22, FALSE);
    installInterrupt(23, isr23, FALSE);
    installInterrupt(24, isr24, FALSE);
    installInterrupt(25, isr25, FALSE);
    installInterrupt(26, isr26, FALSE);
    installInterrupt(27, isr27, FALSE);
    installInterrupt(28, isr28, FALSE);
    installInterrupt(29, isr29, FALSE);
    installInterrupt(30, isr30, FALSE);
    installInterrupt(31, isr31, FALSE);
    debug("Adding HPET to IDT");
    installInterrupt(0, hpet, TRUE);
    debug("Adding PS/2 keyboard to IDT");
    installInterrupt(1, keyboard, TRUE);
    debug("Unmasking second PIC");
    unmaskInterrupt(2);
    debug("Adding PS/2 mouse to IDT");
    installInterrupt(12, mouse, TRUE);
    debug("Adding syscall to IDT");
    installInterrupt(0x80, syscallHandler, FALSE);
    debug("Setting up IDTR");
    struct {
        uint16_t length;
        uint64_t base;
    } __attribute__((packed)) idtr;
    idtr.length = 4095;
    idtr.base = (uint64_t)idt;
    debug("Loading IDT");
    __asm__ volatile ("lidt %0" : : "m"(idtr));
    debug("Enabling interrupts");
    __asm__ volatile ("sti");
    debug("Initialising HPET");
    *(uint32_t*)(hpetAddress + 0x10) |= 0b11;
    *(uint32_t*)(hpetAddress + 0x100) |= 0b1100;
    *(uint64_t*)(hpetAddress + 0x108) = (1000000000000000ULL / ((*(uint64_t*)hpetAddress >> 32) & 0xFFFFFFFF)) / 1000;
    *(uint64_t*)(hpetAddress + 0xF0) = 0;
    debug("Initialising PS/2 mouse");
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
    debug("Identity mapping other cores");
    uint64_t* PML4T = (uint64_t*)0x1000;
    for (uint16_t i = 0; i < 512; i++)
    {
        PML4T[i] = 0;
    }
    uint64_t* PDPT = (uint64_t*)0x2000;
    for (uint16_t i = 0; i < 512; i++)
    {
        PDPT[i] = 0;
    }
    PML4T[0] = (uint64_t)PDPT | 0b11;
    uint64_t* PDT = (uint64_t*)0x3000;
    for (uint16_t i = 0; i < 512; i++)
    {
        PDT[i] = 0;
    }
    PDPT[0] = (uint64_t)PDT | 0b11;
    uint64_t* PT = (uint64_t*)0x4000;
    uint64_t address = 0;
    for (uint16_t i = 0; i < 512; i++)
    {
        PT[i] = address | 0b11;
        address += 0x1000;
    }
    PDT[0] = (uint64_t)PT | 0b11;
    uint64_t cr3 = 0;
    debug("Saving default paging");
    __asm__ volatile ("mov %%cr3, %0" : "=g"(cr3));
    *(uint64_t*)0x5000 = cr3;
    uint64_t smpSize = 0;
    uint8_t* data = readFile(L"/system/smp.bin", &smpSize);
    debug("Loading core binary");
    copyMemory(data, (uint8_t*)0xF000, smpSize);
    *(uint8_t*)0xEFFF = 0;
    debug("Initialising cores");
    for (uint64_t i = 0; i < cpuCount; i++)
    {
        debug("Initialising core");
        *(uint64_t*)(0x5008 + i * 8) = (uint64_t)allocate(0x8000) + 0x8000;
        *(uint32_t*)(apicAddress + 0x310) = cpus[i] << 24;
        *(uint32_t*)(apicAddress + 0x300) = 0x4500;
        while (*(uint32_t*)(apicAddress + 0x300) & 0x1000);
    }
    debug("Starting cores");
    for (uint64_t i = 0; i < cpuCount; i++)
    {
        debug("Starting core");
        *(uint32_t*)(apicAddress + 0x310) = cpus[i] << 24;
        *(uint32_t*)(apicAddress + 0x300) = 0x460F;
        while (*(uint32_t*)(apicAddress + 0x300) & 0x1000);
        hpetCounter = 0;
        while (hpetCounter < 1);
    }
    debug("Waiting for cores");
    while (*(uint8_t*)0xEFFF != cpuCount);
    debug("Starting OS");
    start();
    __asm__ volatile ("cli; hlt");
}
