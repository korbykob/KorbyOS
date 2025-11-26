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
uint8_t mouseCycle = 2;
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
    void* next;
    uint64_t id;
    void* stack;
    uint64_t sp;
} Thread;
Thread* threads = NULL;
Thread* currentThread = NULL;
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
struct
{
    uint8_t scancode;
    BOOLEAN pressed;
} keyPresses[255];
uint8_t currentKeyPress = 0;
struct
{
    int16_t x;
    int16_t y;
} mouseMoves[255];
uint8_t currentMouseMove = 0;
struct
{
    BOOLEAN left;
    BOOLEAN pressed;
} mouseClicks[255];
uint8_t currentMouseClick = 0;
typedef struct
{
    uint32_t lower;
    uint32_t upper;
    uint16_t size;
    uint16_t fragmentChecksum;
    uint8_t done: 1;
    uint8_t end: 1;
    uint8_t reserved1: 1;
    uint8_t vp: 1;
    uint8_t udpChecksum: 1;
    uint8_t l4Checksum: 1;
    uint8_t ipv4Checksum: 1;
    uint8_t filterExact: 1;
    uint8_t reserved2: 5;
    uint8_t tcpUdpError: 1;
    uint8_t ipv4Error: 1;
    uint8_t error: 1;
    uint16_t vlan;
} __attribute__((packed)) E1000RxDescriptor;
typedef struct
{
    uint32_t lower;
    uint32_t upper;
    uint16_t size;
    uint8_t checksumOffset;
    uint8_t end: 1;
    uint8_t insertFcs: 1;
    uint8_t insertChecksum: 1;
    uint8_t reportStatus: 1;
    uint8_t reserved1: 1;
    uint8_t descriptorExtension: 1;
    uint8_t vlanEnable: 1;
    uint8_t reserved2: 1;
    uint8_t done: 1;
    uint8_t reserved: 7;
    uint8_t checksumStart;
    uint16_t vlan;
} __attribute__((packed)) E1000TxDescriptor;
typedef struct
{
    uint16_t size;
    uint16_t reserved: 14;
    uint16_t end: 1;
    uint16_t done: 1;
    uint32_t vlan;
    uint32_t lower;
    uint32_t upper;
} __attribute__((packed)) Rtl8169RxDescriptor;
typedef struct
{
    uint16_t size;
    uint16_t reserved: 12;
    uint16_t last_segment_descriptor: 1;
    uint16_t first_segment_descriptor: 1;
    uint16_t end: 1;
    uint16_t done: 1;
    uint32_t vlan;
    uint32_t lower;
    uint32_t upper;
} __attribute__((packed)) Rtl8169TxDescriptor;
uint64_t bar = 0;
uint8_t mac[6];
void* rxDescriptors = NULL;
void* txDescriptors = NULL;
void (*sendPacket)(uint8_t* data, uint16_t size) = NULL;
uint16_t txCurrent = 0;
struct
{
    uint8_t data[2048];
    uint16_t size;
} ethernetPackets[255];
uint8_t currentEthernetPacket = 0;

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
    uefi_call_wrapper(BS->HandleProtocol, 3, ImageHandle, &LoadedImageProtocol, &image);
    debug("Opening root file system");
    EFI_FILE_HANDLE fs = LibOpenRoot(image->DeviceHandle);
    EFI_FILE_HANDLE file = NULL;
    debug("Loading /system/smp.boot");
    uefi_call_wrapper(fs->Open, 5, fs, &file, L"system\\smp.boot", EFI_FILE_MODE_READ, EFI_FILE_READ_ONLY | EFI_FILE_HIDDEN | EFI_FILE_SYSTEM);
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
    debug("Loading /programs/desktop/taskbar/terminal/program.bin");
    uefi_call_wrapper(fs->Open, 5, fs, &file, L"programs\\desktop\\taskbar\\terminal\\program.bin", EFI_FILE_MODE_READ, EFI_FILE_READ_ONLY | EFI_FILE_HIDDEN | EFI_FILE_SYSTEM);
    info = LibFileInfo(file);
    uint64_t terminalSize = info->FileSize;
    FreePool(info);
    uint8_t* terminal = AllocatePool(terminalSize);
    uefi_call_wrapper(file->Read, 3, file, &terminalSize, terminal);
    uefi_call_wrapper(file->Close, 1, file);
    debug("Loading /programs/desktop/taskbar/terminal/program.bmp");
    uefi_call_wrapper(fs->Open, 5, fs, &file, L"programs\\desktop\\taskbar\\terminal\\program.bmp", EFI_FILE_MODE_READ, EFI_FILE_READ_ONLY | EFI_FILE_HIDDEN | EFI_FILE_SYSTEM);
    info = LibFileInfo(file);
    uint64_t terminalBmpSize = info->FileSize;
    FreePool(info);
    uint8_t* terminalBmp = AllocatePool(terminalBmpSize);
    uefi_call_wrapper(file->Read, 3, file, &terminalBmpSize, terminalBmp);
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
    File* newFile = addItem(&files, sizeof(File));
    newFile->name = L"/system";
    newFile->size = 0;
    newFile->data = NULL;
    debug("Adding /system/smp.boot");
    newFile = addItem(&files, sizeof(File));
    newFile->name = L"/system/smp.boot";
    newFile->size = smpSize;
    newFile->data = smp;
    debug("Adding /fonts");
    newFile = addItem(&files, sizeof(File));
    newFile->name = L"/fonts";
    newFile->size = 0;
    newFile->data = NULL;
    debug("Adding /fonts/font.psf");
    newFile = addItem(&files, sizeof(File));
    newFile->name = L"/fonts/font.psf";
    newFile->size = fontSize;
    newFile->data = font;
    debug("Adding /programs");
    newFile = addItem(&files, sizeof(File));
    newFile->name = L"/programs";
    newFile->size = 0;
    newFile->data = NULL;
    debug("Adding /programs/test");
    newFile = addItem(&files, sizeof(File));
    newFile->name = L"/programs/test";
    newFile->size = 0;
    newFile->data = NULL;
    debug("Adding /programs/test/program.bin");
    newFile = addItem(&files, sizeof(File));
    newFile->name = L"/programs/test/program.bin";
    newFile->size = testSize;
    newFile->data = test;
    debug("Adding /programs/desktop");
    newFile = addItem(&files, sizeof(File));
    newFile->name = L"/programs/desktop";
    newFile->size = 0;
    newFile->data = NULL;
    debug("Adding /programs/desktop/program.bin");
    newFile = addItem(&files, sizeof(File));
    newFile->name = L"/programs/desktop/program.bin";
    newFile->size = desktopSize;
    newFile->data = desktop;
    debug("Adding /programs/desktop/wallpaper.bmp");
    newFile = addItem(&files, sizeof(File));
    newFile->name = L"/programs/desktop/wallpaper.bmp";
    newFile->size = wallpaperSize;
    newFile->data = wallpaper;
    debug("Adding /programs/desktop/taskbar");
    newFile = addItem(&files, sizeof(File));
    newFile->name = L"/programs/desktop/taskbar";
    newFile->size = 0;
    newFile->data = NULL;
    debug("Adding /programs/desktop/taskbar/terminal");
    newFile = addItem(&files, sizeof(File));
    newFile->name = L"/programs/desktop/taskbar/terminal";
    newFile->size = 0;
    newFile->data = NULL;
    debug("Adding /programs/desktop/taskbar/terminal/program.bin");
    newFile = addItem(&files, sizeof(File));
    newFile->name = L"/programs/desktop/taskbar/terminal/program.bin";
    newFile->size = terminalSize;
    newFile->data = terminal;
    debug("Adding /programs/desktop/taskbar/terminal/program.bmp");
    newFile = addItem(&files, sizeof(File));
    newFile->name = L"/programs/desktop/taskbar/terminal/program.bmp";
    newFile->size = terminalBmpSize;
    newFile->data = terminalBmp;
    debug("Adding /programs/desktop/taskbar/rendering");
    newFile = addItem(&files, sizeof(File));
    newFile->name = L"/programs/desktop/taskbar/rendering";
    newFile->size = 0;
    newFile->data = NULL;
    debug("Adding /programs/desktop/taskbar/rendering/program.bin");
    newFile = addItem(&files, sizeof(File));
    newFile->name = L"/programs/desktop/taskbar/rendering/program.bin";
    newFile->size = renderingSize;
    newFile->data = rendering;
    debug("Adding /programs/desktop/taskbar/rendering/program.bmp");
    newFile = addItem(&files, sizeof(File));
    newFile->name = L"/programs/desktop/taskbar/rendering/program.bmp";
    newFile->size = renderingBmpSize;
    newFile->data = renderingBmp;
    debug("Adding /programs/desktop/taskbar/rendering/wall.bmp");
    newFile = addItem(&files, sizeof(File));
    newFile->name = L"/programs/desktop/taskbar/rendering/wall.bmp";
    newFile->size = wallSize;
    newFile->data = wall;
    debug("Adding /programs/desktop/taskbar/rendering/sprite.bmp");
    newFile = addItem(&files, sizeof(File));
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
    while (iterateList(&iterator))
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
        file = addItem(&files, sizeof(File));
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
    while (iterateList(&file))
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
    while (iterateList(&file))
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
    while (iterateList(&file))
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
    while (iterateList(&file))
    {
        if (StrCmp(name, file->name) == 0)
        {
            removeItem(&files, file);
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
    while (iterateList(&file))
    {
        if (StrnCmp(file->name, root, length) == 0 && (recursive || !inString(file->name + length, L'/')))
        {
            *count = *count + 1;
        }
    }
    File** items = allocate(*count * sizeof(File*));
    uint64_t i = 0;
    file = (File*)&files;
    while (iterateList(&file))
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
    initGraphics((EFI_GRAPHICS_OUTPUT_BLT_PIXEL*)GOP->Mode->FrameBufferBase, GOP->Mode->Info->HorizontalResolution, readFile(L"/fonts/font.psf", NULL));
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
    initGraphics((EFI_GRAPHICS_OUTPUT_BLT_PIXEL*)GOP->Mode->FrameBufferBase, GOP->Mode->Info->HorizontalResolution, readFile(L"/fonts/font.psf", NULL));
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

__attribute__((naked)) void hpet()
{
    __asm__ volatile ("pushq %rax; pushq %rbx; pushq %rcx; pushq %rdx; pushq %rsi; pushq %rdi; pushq %rbp; pushq %r8; pushq %r9; pushq %r10; pushq %r11; pushq %r12; pushq %r13; pushq %r14; pushq %r15");
    __asm__ volatile ("subq $256, %rsp; movdqu %xmm0, 240(%rsp); movdqu %xmm1, 224(%rsp); movdqu %xmm2, 208(%rsp); movdqu %xmm3, 192(%rsp); movdqu %xmm4, 176(%rsp); movdqu %xmm5, 160(%rsp); movdqu %xmm6, 144(%rsp); movdqu %xmm7, 128(%rsp); movdqu %xmm8, 112(%rsp); movdqu %xmm9, 96(%rsp); movdqu %xmm10, 80(%rsp); movdqu %xmm11, 64(%rsp); movdqu %xmm12, 48(%rsp); movdqu %xmm13, 32(%rsp); movdqu %xmm14, 16(%rsp); movdqu %xmm15, (%rsp)");
    __asm__ volatile ("movq %%rsp, %0" : "=g"(currentThread->sp));
    __asm__ volatile ("movq %1, %0" : "=g"(currentThread) : "g"(currentThread->next));
    __asm__ volatile ("incq hpetCounter(%rip)");
    __asm__ volatile ("movb $0x20, %al; outb %al, $0x20");
    __asm__ volatile ("nextThread:");
    __asm__ volatile ("movq %0, %%rsp" : : "g"(currentThread->sp));
    __asm__ volatile ("movdqu (%rsp), %xmm15; movdqu 16(%rsp), %xmm14; movdqu 32(%rsp), %xmm13; movdqu 48(%rsp), %xmm12; movdqu 64(%rsp), %xmm11; movdqu 80(%rsp), %xmm10; movdqu 96(%rsp), %xmm9; movdqu 112(%rsp), %xmm8; movdqu 128(%rsp), %xmm7; movdqu 144(%rsp), %xmm6; movdqu 160(%rsp), %xmm5; movdqu 176(%rsp), %xmm4; movdqu 192(%rsp), %xmm3; movdqu 208(%rsp), %xmm2; movdqu 224(%rsp), %xmm1; movdqu 240(%rsp), %xmm0; addq $256, %rsp");
    __asm__ volatile ("popq %r15; popq %r14; popq %r13; popq %r12; popq %r11; popq %r10; popq %r9; popq %r8; popq %rbp; popq %rdi; popq %rsi; popq %rdx; popq %rcx; popq %rbx; popq %rax");
    __asm__ volatile ("iretq");
}

void destroyThread(uint64_t id)
{
    Thread* last = threads;
    Thread* current = threads;
    while (current->id != id)
    {
        last = current;
        current = current->next;
    }
    last->next = current->next;
    unallocate(current->stack);
    unallocate(current);
}

void exitThread()
{
    Thread* current = threads;
    while (current->next != currentThread)
    {
        current = current->next;
    }
    current->next = currentThread->next;
    unallocate(currentThread->stack);
    unallocate(currentThread);
    currentThread = current->next;
    __asm__ volatile ("jmp nextThread");
}

uint64_t createThread(void (*function)())
{
    uint64_t flags = 0;
    uint64_t cs = 0;
    uint64_t ss = 0;
    __asm__ volatile ("pushfq; pop %0; movq %%cs, %1; movq %%ss, %2" : "=g"(flags), "=r"(cs), "=r"(ss));
    Thread* thread = allocate(sizeof(Thread));
    thread->next = currentThread;
    uint64_t id = 0;
    Thread* current = threads;
    while (TRUE)
    {
        if (current->id == id)
        {
            id++;
            current = threads;
        }
        else
        {
            current = current->next;
            if (current == threads)
            {
                break;
            }
        }
    }
    thread->id = id;
    thread->stack = allocate(0x80000);
    thread->sp = (uint64_t)thread->stack + 0x80000 - sizeof(InterruptFrame) - 8;
    InterruptFrame* frame = (InterruptFrame*)thread->sp;
    frame->ip = (uint64_t)function;
    frame->cs = cs;
    frame->flags = flags;
    frame->sp = thread->sp + sizeof(InterruptFrame);
    frame->ss = ss;
    *(uint64_t*)(thread->sp + sizeof(InterruptFrame)) = (uint64_t)exitThread;
    __asm__ volatile ("movq %%rsp, %0" : "=g"(currentThread->sp));
    currentThread = thread;
    __asm__ volatile ("movq %0, %%rsp" : : "g"(currentThread->sp));
    __asm__ volatile ("pushq %rax; pushq %rbx; pushq %rcx; pushq %rdx; pushq %rsi; pushq %rdi; pushq %rbp; pushq %r8; pushq %r9; pushq %r10; pushq %r11; pushq %r12; pushq %r13; pushq %r14; pushq %r15");
    __asm__ volatile ("subq $256, %rsp; movdqu %xmm0, 240(%rsp); movdqu %xmm1, 224(%rsp); movdqu %xmm2, 208(%rsp); movdqu %xmm3, 192(%rsp); movdqu %xmm4, 176(%rsp); movdqu %xmm5, 160(%rsp); movdqu %xmm6, 144(%rsp); movdqu %xmm7, 128(%rsp); movdqu %xmm8, 112(%rsp); movdqu %xmm9, 96(%rsp); movdqu %xmm10, 80(%rsp); movdqu %xmm11, 64(%rsp); movdqu %xmm12, 48(%rsp); movdqu %xmm13, 32(%rsp); movdqu %xmm14, 16(%rsp); movdqu %xmm15, (%rsp)");
    __asm__ volatile ("movq %%rsp, %0" : "=g"(currentThread->sp));
    __asm__ volatile ("movq %1, %0" : "=g"(currentThread) : "g"(currentThread->next));
    __asm__ volatile ("movq %0, %%rsp" : : "g"(currentThread->sp));
    thread->next = threads;
    current = threads;
    while (current->next != threads)
    {
        current = current->next;
    }
    current->next = thread;
    return id;
}

__attribute__((interrupt, target("general-regs-only"))) void keyboard(InterruptFrame* frame)
{
    uint8_t scancode = inb(0x60);
    BOOLEAN unpressed = scancode & 0b10000000;
    if (unpressed)
    {
        scancode = scancode & 0b01111111;
    }
    keyPresses[currentKeyPress].scancode = scancode;
    keyPresses[currentKeyPress].pressed = !unpressed;
    currentKeyPress++;
    outb(0x20, 0x20);
}

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
            mouseMoves[currentMouseMove].x = x;
            mouseMoves[currentMouseMove].y = y;
            currentMouseMove++;
        }
        BOOLEAN leftClick = mouseBytes[0] & 0b00000001;
        if (leftClick != lastLeftClick)
        {
            mouseClicks[currentMouseClick].left = TRUE;
            mouseClicks[currentMouseClick].pressed = !lastLeftClick;
            currentMouseClick++;
            lastLeftClick = leftClick;
        }
        BOOLEAN rightClick = mouseBytes[0] & 0b00000010;
        if (rightClick != lastRightClick)
        {
            mouseClicks[currentMouseClick].left = FALSE;
            mouseClicks[currentMouseClick].pressed = !lastRightClick;
            currentMouseClick++;
            lastRightClick = rightClick;
        }
    }
    outb(0xA0, 0x20);
    outb(0x20, 0x20);
}

uint8_t pciInb(uint32_t bus, uint32_t device, uint32_t function, uint32_t offset)
{
    outd(0xCF8, (0x80000000 | (bus << 16) | (device << 11) | (function << 8) | (offset & ~0x3)));
    return (uint8_t)(ind(0xCFC) >> ((offset & 0x3) * 8));
}

uint16_t pciInw(uint32_t bus, uint32_t device, uint32_t function, uint32_t offset)
{
    outd(0xCF8, (0x80000000 | (bus << 16) | (device << 11) | (function << 8) | (offset & ~0x3)));
    return (uint16_t)(ind(0xCFC) >> ((offset & 0x3) * 8));
}

uint32_t pciInd(uint32_t bus, uint32_t device, uint32_t function, uint32_t offset)
{
    outd(0xCF8, (0x80000000 | (bus << 16) | (device << 11) | (function << 8) | offset));
    return ind(0xCFC);
}

void pciOutd(uint32_t bus, uint32_t device, uint32_t function, uint32_t offset, uint32_t value)
{
    outd(0xCF8, (0x80000000 | (bus << 16) | (device << 11) | (function << 8) | offset));
    outd(0xCFC, value);
}

__attribute__((interrupt, target("general-regs-only"))) void e1000(InterruptFrame* frame)
{
    uint32_t status = (*(uint32_t*)(bar + 0xC0) & 0xFFFFF);
    if (status)
    {
        if (status & 0x80)
        {
            uint32_t tail = (*(uint32_t*)(bar + 0x2818) + 1);
            if (tail >= 256)
            {
                tail = 0;
            }
            if ((((E1000RxDescriptor*)rxDescriptors)[tail].error | ((E1000RxDescriptor*)rxDescriptors)[tail].ipv4Error | ((E1000RxDescriptor*)rxDescriptors)[tail].tcpUdpError) == 0)
            {
                copyMemory((uint8_t*)((uint64_t)((E1000RxDescriptor*)rxDescriptors)[tail].lower | ((uint64_t)((E1000RxDescriptor*)rxDescriptors)[tail].upper << 32)), ethernetPackets[currentEthernetPacket].data, ((E1000RxDescriptor*)rxDescriptors)[tail].size);
                ethernetPackets[currentEthernetPacket].size = ((E1000RxDescriptor*)rxDescriptors)[tail].size;
                currentEthernetPacket++;
            }
            ((E1000RxDescriptor*)rxDescriptors)[tail].done = 0;
            *(uint32_t*)(bar + 0x2818) = tail;
        }
        *(uint32_t*)(bar + 0xC0) = status;
    }
    outb(0xA0, 0x20);
    outb(0x20, 0x20);
}

void e1000Send(uint8_t* data, uint16_t size)
{
    uint32_t current = *(uint32_t*)(bar + 0x3818);
    uint32_t next = (current + 1);
    if (next >= 256)
    {
        next = 0;
    }
    copyMemory(data, (uint8_t*)((uint64_t)((E1000TxDescriptor*)txDescriptors)[current].lower | ((uint64_t)((E1000TxDescriptor*)txDescriptors)[current].upper << 32)), size);
    ((E1000TxDescriptor*)txDescriptors)[current].size = size;
    ((E1000TxDescriptor*)txDescriptors)[current].end = 1;
    ((E1000TxDescriptor*)txDescriptors)[current].done = 0;
    *(uint32_t*)(bar + 0x3818) = next;
}

__attribute__((interrupt, target("general-regs-only"))) void rtl8169(InterruptFrame* frame)
{
    uint16_t status = inw(bar + 0x3E);
    if (status)
    {
        if ((status & 0x11) != 0x00)
        {
            for (uint32_t i = 0; i < 256; i++)
            {
                if (((Rtl8169RxDescriptor*)rxDescriptors)[i].done != 1)
                {
                    copyMemory((uint8_t*)((uint64_t)((Rtl8169RxDescriptor*)rxDescriptors)[i].lower | ((uint64_t)((Rtl8169RxDescriptor*)rxDescriptors)[i].upper << 32)), ethernetPackets[currentEthernetPacket].data, ((Rtl8169RxDescriptor*)rxDescriptors)[i].size);
                    ethernetPackets[currentEthernetPacket].size = ((Rtl8169RxDescriptor*)rxDescriptors)[i].size;
                    currentEthernetPacket++;
                    ((Rtl8169RxDescriptor*)rxDescriptors)[i].size = 2048;
                    ((Rtl8169RxDescriptor*)rxDescriptors)[i].vlan = 0;
                    ((Rtl8169RxDescriptor*)rxDescriptors)[i].done = 1;
                }
            }
        }
        outw(bar + 0x3E, status);
    }
    outb(0xA0, 0x20);
    outb(0x20, 0x20);
}

void rtl8169Send(uint8_t* data, uint16_t size)
{
    copyMemory(data, (uint8_t*)((uint64_t)((Rtl8169TxDescriptor*)txDescriptors)[txCurrent].lower | ((uint64_t)((Rtl8169TxDescriptor*)txDescriptors)[txCurrent].upper << 32)), size);
    ((Rtl8169TxDescriptor*)txDescriptors)[txCurrent].size = size;
    ((Rtl8169TxDescriptor*)txDescriptors)[txCurrent].first_segment_descriptor = 1;
    ((Rtl8169TxDescriptor*)txDescriptors)[txCurrent].last_segment_descriptor = 1;
    ((Rtl8169TxDescriptor*)txDescriptors)[txCurrent].done = 1;
    outb(bar + 0x38, (1 << 6));
    txCurrent++;
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
    debug("Initialising HPET");
    *(uint32_t*)(hpetAddress + 0x10) |= 0b11;
    *(uint32_t*)(hpetAddress + 0x100) |= 0b1100;
    *(uint64_t*)(hpetAddress + 0x108) = (1000000000000000ULL / ((*(uint64_t*)hpetAddress >> 32) & 0xFFFFFFFF)) / 1000;
    *(uint64_t*)(hpetAddress + 0xF0) = 0;
    debug("Setting up main thread");
    threads = allocate(sizeof(Thread));
    threads->next = threads;
    threads->id = 0;
    __asm__ volatile ("movq %%rsp, %0" : "=g"(threads->sp));
    currentThread = threads;
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
    debug("Initialising PCI devices");
    BOOLEAN found = FALSE;
    for (uint16_t bus = 0; bus < 256; bus++)
    {
        for (uint8_t device = 0; device < 32; device++)
        {
            for (uint8_t function = 0; function < 8; function++)
            {
                uint32_t id = pciInd(bus, device, function, 0);
                if (id == 0x10D38086)
                {
                    debug("Found e1000e");
                    found = TRUE;
                    debug("Getting first bar address");
                    bar = pciInd(bus, device, function, 0x10) & 0xFFFFFFF0;
                    debug("Enabling bus mastering");
                    pciOutd(bus, device, function, 0x04, pciInd(bus, device, function, 0x04) | (1 << 2));
                    debug("Reading MAC address");
                    *(uint32_t*)(bar + 0x00) = (1 << 26);
                    while (*(uint32_t*)(bar + 0x00) & (1 << 26));
                    for (uint8_t i = 0; i < 3; i++)
                    {
                        *(uint32_t*)(bar + 0x14) = ((uint32_t)i << 2) | 1;
                        uint32_t tmp = 0;
                        while (!((tmp = *(uint32_t*)(bar + 0x14)) & (1 << 1)));
                        mac[i * 2] = tmp >> 16;
                        mac[i * 2 + 1] = tmp >> 24;
                    }
                    debug("Enabling card");
                    *(uint32_t*)(bar + 0x00) = *(uint32_t*)(bar + 0x00) | (1 << 6);
                    debug("Setting up RX buffers");
                    rxDescriptors = (E1000RxDescriptor*)(((uint64_t)allocate(sizeof(E1000RxDescriptor) * 256 + 0xFF) + 0xFF) & 0xFFFFFFFFFFFFFF00);
                    for (uint64_t i = 0; i < sizeof(E1000RxDescriptor) * 256; i++)
                    {
                        *((uint8_t*)rxDescriptors + i) = 0;
                    }
                    debug("Setting up RX memory");
                    uint8_t* rxMemory = (uint8_t*)allocate(2048 * 256);
                    for (uint32_t i = 0; i < 256; i++)
                    {
                        uint64_t address = (uint64_t)(rxMemory + 2048 * i);
                        ((E1000RxDescriptor*)rxDescriptors)[i].lower = address;
                        ((E1000RxDescriptor*)rxDescriptors)[i].upper = address >> 32;
                    }
                    debug("Updating RX buffers");
                    *(uint32_t*)(bar + 0x2800) = (uint64_t)rxDescriptors;
                    *(uint32_t*)(bar + 0x2804) = (uint64_t)rxDescriptors >> 32;
                    *(uint32_t*)(bar + 0x2808) = sizeof(E1000RxDescriptor) * 256;
                    *(uint32_t*)(bar + 0x2810) = 0;
                    *(uint32_t*)(bar + 0x2818) = 255;
                    debug("Setting up TX buffers");
                    txDescriptors = (E1000TxDescriptor*)(((uint64_t)allocate(sizeof(E1000TxDescriptor) * 256 + 0xFF) + 0xFF) & 0xFFFFFFFFFFFFFF00);
                    for (uint64_t i = 0; i < sizeof(E1000TxDescriptor) * 256; i++)
                    {
                        *((uint8_t*)txDescriptors + i) = 0;
                    }
                    debug("Setting up TX memory");
                    uint8_t* txMemory = (uint8_t*)allocate(2048 * 256);
                    for (uint32_t i = 0; i < 256; i++)
                    {
                        uint64_t address = (uint64_t)(txMemory + 2048 * i);
                        ((E1000TxDescriptor*)txDescriptors)[i].lower = address;
                        ((E1000TxDescriptor*)txDescriptors)[i].upper = address >> 32;
                    }
                    debug("Updating TX buffers");
                    *(uint32_t*)(bar + 0x3800) = (uint64_t)txDescriptors;
                    *(uint32_t*)(bar + 0x3804) = (uint64_t)txDescriptors >> 32;
                    *(uint32_t*)(bar + 0x3808) = sizeof(E1000TxDescriptor) * 256;
                    *(uint32_t*)(bar + 0x3810) = 0;
                    *(uint32_t*)(bar + 0x3818) = 0;
                    debug("Enabling receiving packets");
                    *(uint32_t*)(bar + 0x100) = ((1 << 1) | (1 << 3) | (0 << 5) | (1 << 15));
                    debug("Enabling sending packets");
                    *(uint32_t*)(bar + 0x400) = ((1 << 1) | (1 << 3) | (1 << 15) | (0x3F << 12) | (0x3 << 28));
                    debug("Setting up interrupts for card");
                    installInterrupt(pciInb(bus, device, function, 0x3C), e1000, TRUE);
                    *(uint32_t*)(bar + 0xD0) = (1 << 7);
                    debug("Updating sendPacket function");
                    sendPacket = e1000Send;
                    break;
                }
                else if (id == 0x816810EC)
                {
                    debug("Found rtl8169");
                    found = TRUE;
                    debug("Getting first bar address");
                    bar = pciInw(bus, device, function, 0x10) & 0xFFFC;
                    debug("Enabling bus mastering");
                    pciOutd(bus, device, function, 0x04, pciInd(bus, device, function, 0x04) | (1 << 2));
                    debug("Enabling multiple read and writes");
                    outw(bar + 0xE0, (1 << 3));
                    debug("Resetting card");
                    outb(bar + 0x37, (1 << 4));
                    while ((inb(bar + 0x37) & (1 << 4)) == (1 << 4));
                    debug("Reading MAC address");
                    for (uint8_t i = 0; i < 6; i++)
                    {
                        mac[i] = inb(bar + i);
                    }
                    debug("Unlocking registers");
                    outb(bar + 0x50, 0xC0);
                    debug("Setting up interrupts for card");
                    installInterrupt(pciInb(bus, device, function, 0x3C), rtl8169, TRUE);
                    outw(bar + 0x3C, (1 << 0));
                    debug("Setting up RX buffers");
                    rxDescriptors = (Rtl8169RxDescriptor*)(((uint64_t)allocate(sizeof(Rtl8169RxDescriptor) * 256 + 0xFF) + 0xFF) & 0xFFFFFFFFFFFFFF00);
                    for (uint64_t i = 0; i < sizeof(Rtl8169RxDescriptor) * 256; i++)
                    {
                        *((uint8_t*)rxDescriptors + i) = 0;
                    }
                    debug("Setting up RX memory");
                    uint8_t* rxMemory = (uint8_t*)allocate(2048 * 256);
                    for (uint32_t i = 0; i < 256; i++)
                    {
                        uint64_t address = (uint64_t)(rxMemory + 2048 * i);
                        ((Rtl8169RxDescriptor*)rxDescriptors)[i].lower = address;
                        ((Rtl8169RxDescriptor*)rxDescriptors)[i].upper = address >> 32;
                        ((Rtl8169RxDescriptor*)rxDescriptors)[i].size = 2048;
                        ((Rtl8169RxDescriptor*)rxDescriptors)[i].done = 1;
                    }
                    ((Rtl8169RxDescriptor*)rxDescriptors)[255].end = 1;
                    debug("Updating RX buffers");
                    outd(bar + 0x44, (0x1F | (0x7 << 8) | (0x7 << 13)));
                    outw(bar + 0xDA, 0x1FFF);
                    outd(bar + 0xE4, (uint64_t)rxDescriptors);
                    outd(bar + 0xE8, (uint64_t)rxDescriptors >> 32);
                    debug("Setting up TX buffers");
                    txDescriptors = (Rtl8169TxDescriptor*)(((uint64_t)allocate(sizeof(Rtl8169TxDescriptor) * 256 + 0xFF) + 0xFF) & 0xFFFFFFFFFFFFFF00);
                    for (uint64_t i = 0; i < sizeof(Rtl8169TxDescriptor) * 256; i++)
                    {
                        *((uint8_t*)txDescriptors + i) = 0;
                    }
                    debug("Setting up TX memory");
                    uint8_t* txMemory = (uint8_t*)allocate(2048 * 256);
                    for (uint32_t i = 0; i < 256; i++)
                    {
                        uint64_t address = (uint64_t)(txMemory + 2048 * i);
                        ((Rtl8169TxDescriptor*)txDescriptors)[i].lower = address;
                        ((Rtl8169TxDescriptor*)txDescriptors)[i].upper = address >> 32;
                    }
                    ((Rtl8169TxDescriptor*)txDescriptors)[255].end = 1;
                    debug("Updating TX buffers");
                    outb(bar + 0x37, 0x04);
                    outb(bar + 0xEC, 0x3F);
                    outd(bar + 0x40, (0x7 << 8) | (0x3 << 24));
                    outd(bar + 0x20, (uint64_t)txDescriptors);
                    outd(bar + 0x24, (uint64_t)txDescriptors >> 32);
                    debug("Disabling high priority TX buffers");
                    outd(bar + 0x28, 0);
                    outd(bar + 0x2C, 0);
                    debug("Enabling sending and receiving packets");
                    outb(bar + 0x37, 0x0C);
                    debug("Locking registers");
                    outb(bar + 0x50, 0x00);
                    debug("Updating sendPacket function");
                    sendPacket = rtl8169Send;
                    break;
                }
            }
            if (found)
            {
                break;
            }
        }
        if (found)
        {
            break;
        }
    }
    debug("Enabling interrupts");
    __asm__ volatile ("sti");
    debug("Saving paging address");
    uint64_t cr3 = 0;
    __asm__ volatile ("mov %%cr3, %0" : "=g"(cr3));
    *(uint32_t*)0x1000 = cr3;
    uint64_t smpSize = 0;
    uint8_t* data = readFile(L"/system/smp.boot", &smpSize);
    debug("Loading core binary");
    copyMemory(data, (uint8_t*)0x2000, smpSize);
    *(uint8_t*)0x1004 = 0;
    debug("Initialising cores");
    for (uint64_t i = 0; i < cpuCount; i++)
    {
        debug("Initialising core");
        *(uint64_t*)(0x1005 + i * 8) = (uint64_t)allocate(0x8000) + 0x8000;
        *(uint32_t*)(apicAddress + 0x310) = cpus[i] << 24;
        *(uint32_t*)(apicAddress + 0x300) = 0x4500;
        while (*(uint32_t*)(apicAddress + 0x300) & 0x1000);
    }
    debug("Starting cores");
    for (uint64_t i = 0; i < cpuCount; i++)
    {
        debug("Starting core");
        *(uint32_t*)(apicAddress + 0x310) = cpus[i] << 24;
        *(uint32_t*)(apicAddress + 0x300) = 0x4602;
        while (*(uint32_t*)(apicAddress + 0x300) & 0x1000);
        hpetCounter = 0;
        while (hpetCounter == 0);
    }
    debug("Waiting for cores");
    while (*(uint8_t*)0x1004 != cpuCount);
    debug("Starting OS");
    start();
    while (TRUE);
}
