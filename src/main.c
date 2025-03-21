#include "main.h"

typedef struct
{
    void* next;
    uint64_t pid;
    void (*start)();
    void (*update)(uint64_t ticks);
} Program;
Program* running = NULL;
Program* currentProgram = 0;
PointerArray* keyCalls = NULL;
PointerArray* moveCalls = NULL;
PointerArray* clickCalls = NULL;
Syscall* syscallIds = NULL;
uint64_t (*syscallHandlers[256])(uint64_t code, uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4);
EFI_GRAPHICS_OUTPUT_BLT_PIXEL* videoBuffer = NULL;

void execute(const CHAR16* file)
{
    debug("Executing program");
    uint64_t pid = 0;
    Program* iterator = (Program*)&running;
    while (iterateList((void**)&iterator))
    {
        if (pid == iterator->pid)
        {
            pid++;
            iterator = (Program*)&running;
        }
    }
    debug("Allocating program");
    Program* program = addItemFirst((void**)&running, sizeof(Program));
    program->pid = pid;
    uint64_t size = 0;
    uint8_t* data = readFile(file, &size);
    debug("Allocating binary");
    program->start = allocate(size);
    copyMemory(data, (uint8_t*)program->start, size);
    program->update = (void*)program->start + 5;
    Program* oldProgram = currentProgram;
    currentProgram = program;
    debug("Starting program");
    program->start();
    currentProgram = oldProgram;
    debug("Program executed");
}

void quit()
{
    debug("Exiting program");
    unallocate(currentProgram->start);
    removeItem((void**)&running, currentProgram);
    debug("Exited program");
}

File** getFiles(const CHAR16* root, uint64_t* count)
{
    debug("Getting files");
    File* file = (File*)&files;
    while (iterateList((void**)&file))
    {
        if (StrnCmp(file->name, root, 9) == 0)
        {
            *count = *count + 1;
        }
    }
    File** items = allocate(*count * sizeof(File*));
    uint64_t i = 0;
    file = (File*)&files;
    while (iterateList((void**)&file))
    {
        if (StrnCmp(file->name, root, 9) == 0)
        {
            items[i] = file;
            i++;
        }
    }
    debug("Got files");
    return items;
}

PointerArray* addKeyCall(void (*keyCall)(uint8_t scancode, BOOLEAN pressed))
{
    debug("Adding key press call");
    PointerArray* item = addItem((void**)&keyCalls, sizeof(PointerArray));
    item->pointer = keyCall;
    debug("Added key press call");
    return item;
}

void removeKeyCall(PointerArray* keyCall)
{
    debug("Removing key press call");
    removeItem((void**)&keyCalls, keyCall);
    debug("Removed key press call");
}

PointerArray* addMoveCall(void (*moveCall)(int16_t x, int16_t y))
{
    debug("Adding mouse move call");
    PointerArray* item = addItem((void**)&moveCalls, sizeof(PointerArray));
    item->pointer = moveCall;
    debug("Added mouse move call");
    return item;
}

void removeMoveCall(PointerArray* moveCall)
{
    debug("Removing mouse move call");
    removeItem((void**)&moveCalls, moveCall);
    debug("Removed mouse move call");
}

PointerArray* addClickCall(void (*clickCall)(BOOLEAN left, BOOLEAN pressed))
{
    debug("Adding mouse click call");
    PointerArray* item = addItem((void**)&clickCalls, sizeof(PointerArray));
    item->pointer = clickCall;
    debug("Added mouse click call");
    return item;
}

void removeClickCall(PointerArray* clickCall)
{
    debug("Removing mouse click call");
    removeItem((void**)&clickCalls, clickCall);
    debug("Removed mouse click call");
}

uint8_t registerSyscallHandler(const CHAR16* name, uint64_t (*syscallHandler)(uint64_t code, uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4))
{
    debug("Registering syscall handler");
    uint8_t id = 0;
    while (TRUE)
    {
        if (syscallHandlers[id] == NULL)
        {
            debug("Found unused syscall id");
            break;
        }
        id++;
    }
    syscallHandlers[id] = syscallHandler;
    debug("Allocating syscall id");
    Syscall* syscall = addItem((void**)&syscallIds, sizeof(Syscall));
    debug("Reallocating name");
    syscall->name = allocate((StrLen(name) + 1) * 2);
    StrCpy(syscall->name, name);
    syscall->id = id;
    debug("Registered syscall handler");
    return id;
}

uint8_t getRegisteredSyscall(const CHAR16* name)
{
    debug("Getting registered syscall");
    Syscall* iterator = (Syscall*)&syscallIds;
    while (iterateList((void**)&iterator))
    {
        if (StrCmp(name, iterator->name) == 0)
        {
            debug("Got registered syscall");
            return iterator->id;
        }
    }
    return 0;
}

void unregisterSyscallHandler(const CHAR16* name)
{
    debug("Unregistering syscall handler");
    Syscall* iterator = (Syscall*)&syscallIds;
    while (iterateList((void**)&iterator))
    {
        if (StrCmp(name, iterator->name) == 0)
        {
            debug("Found syscall handler");
            syscallHandlers[iterator->id] = NULL;
            unallocate(iterator->name);
            removeItem((void**)&syscallIds, iterator);
            debug("Unregistered syscall handler");
        }
    }
}

void getDisplayInfo(Display* display)
{
    debug("Getting display info");
    display->buffer = videoBuffer;
    display->width = GOP->Mode->Info->HorizontalResolution;
    display->height = GOP->Mode->Info->VerticalResolution;
    debug("Got display info");
}

uint64_t syscallHandle(uint64_t code, uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4, uint64_t arg5)
{
    switch (code)
    {
        case 0:
            execute((const CHAR16*)arg1);
            break;
        case 1:
            quit();
            break;
        case 2:
            return (uint64_t)allocate(arg1);
            break;
        case 3:
            unallocate((void*)arg1);
            break;
        case 4:
            return (uint64_t)writeFile((const CHAR16*)arg1, arg2);
            break;
        case 5:
            return (uint64_t)readFile((const CHAR16*)arg1, (uint64_t*)arg2);
            break;
        case 6:
            deleteFile((const CHAR16*)arg1);
            break;
        case 7:
            return (uint64_t)getFiles((const CHAR16*)arg1, (uint64_t*)arg2);
            break;
        case 8:
            return cpuCount + 1;
            break;
        case 9:
            return (uint64_t)addKeyCall((void (*)(uint8_t scancode, BOOLEAN pressed))arg1);
            break;
        case 10:
            removeKeyCall((PointerArray*)arg1);
            break;
        case 11:
            return (uint64_t)addMoveCall((void (*)(int16_t x, int16_t y))arg1);
            break;
        case 12:
            removeMoveCall((PointerArray*)arg1);
            break;
        case 13:
            return (uint64_t)addClickCall((void (*)(BOOLEAN left, BOOLEAN pressed))arg1);
            break;
        case 14:
            removeClickCall((PointerArray*)arg1);
            break;
        case 15:
            return (uint64_t)registerSyscallHandler((const CHAR16*)arg1, (uint64_t (*)(uint64_t code, uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4))arg2);
            break;
        case 16:
            return (uint64_t)getRegisteredSyscall((const CHAR16*)arg1);
            break;
        case 17:
            unregisterSyscallHandler((const CHAR16*)arg1);
            break;
        case 18:
            getDisplayInfo((Display*)arg1);
            break;
        default:
            return syscallHandlers[code](arg1, arg2, arg3, arg4, arg5);
            break;
    }
    return 0;
}

void keyPress(uint8_t scancode, BOOLEAN pressed)
{
    PointerArray* call = (PointerArray*)&keyCalls;
    while (iterateList((void**)&call))
    {
        ((void (*)(uint8_t scancode, BOOLEAN pressed))call->pointer)(scancode, pressed);
    }
}

void mouseMove(int16_t x, int16_t y)
{
    PointerArray* call = (PointerArray*)&moveCalls;
    while (iterateList((void**)&call))
    {
        ((void (*)(int16_t x, int16_t y))call->pointer)(x, y);
    }
}

void mouseClick(BOOLEAN left, BOOLEAN pressed)
{
    PointerArray* call = (PointerArray*)&clickCalls;
    while (iterateList((void**)&call))
    {
        ((void (*)(BOOLEAN left, BOOLEAN pressed))call->pointer)(left, pressed);
    }
}

void start()
{
    debug("Filling syscall handlers");
    for (uint8_t i = 0; i < 19; i++)
    {
        syscallHandlers[i] = (void*)1;
    }
    debug("Allocating video buffer");
    videoBuffer = allocate(GOP->Mode->FrameBufferSize);
    uint64_t ticks = 0;
    debug("Starting loop");
    execute(L"programs/desktop/program.bin");
    while (TRUE)
    {
        Program* program = (Program*)&running;
        while (iterateList((void**)&program))
        {
            currentProgram = program;
            program->update(ticks);
        }
        blitTo = (uint64_t*)GOP->Mode->FrameBufferBase;
        blitFrom = (uint64_t*)videoBuffer;
        blitSize = (GOP->Mode->Info->HorizontalResolution * GOP->Mode->Info->VerticalResolution / 2) / (cpuCount + 1);
        splitTask(coreBlit, cpuCount + 1);
        ticks = hpetCounter;
        hpetCounter = 0;
    }
}
