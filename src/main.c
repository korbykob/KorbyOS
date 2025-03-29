#include "main.h"

uint32_t terminalWidth = 0;
uint32_t terminalHeight = 0;
uint32_t cursorX = 0;
uint32_t cursorY = 0;
BOOLEAN typing = FALSE;
uint32_t typingStart = 0;
CHAR16* typingBuffer = NULL;
uint64_t typingCurrent = 0;
uint64_t terminalPid = UINT64_MAX;
CHAR16* terminalDirectory = NULL;
BOOLEAN leftShift = FALSE;
BOOLEAN rightShift = FALSE;
BOOLEAN shift = FALSE;
BOOLEAN caps = FALSE;
char scancodes[] = {
    0, 0, '1', '2', '3', '4', '5', '6', '7', '8', /* 9 */
    '9', '0', '-', '=', 0, /* Backspace */
    0, /* Tab */
    'q', 'w', 'e', 'r', /* 19 */
    't', 'y', 'u', 'i', 'o', 'p', '[', ']', 0, /* Enter key */
    0, /* 29 - Control */
    'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', /* 39 */
    '\'', '`',  0, /* Left shift */
    '\\', 'z', 'x', 'c', 'v', 'b', 'n', /* 49 */
    'm', ',', '.', '/', 0, /* Right shift */
    '*', 0, /* Alt */
    ' ', /* Space bar */
    0, /* Caps lock */
    0, /* 59 - F1 key ... > */
    0, 0, 0, 0, 0, 0, 0, 0, 0, /* < ... F10 */
    0, /* 69 - Num lock */
    0, /* Scroll Lock */
    0, /* Home key */
    0, /* Up Arrow */
    0, /* Page Up */
    '-', 0, /* Left Arrow */
    0, 0, /* Right Arrow */
    '+', 0, /* 79 - End key */
    0, /* Down Arrow */
    0, /* Page Down */
    0, /* Insert Key */
    0, /* Delete Key */
    0, 0, 0, 0, /* F11 Key */
    0, /* F12 Key */
    0 /* All other keys are undefined */
};
char capsScancodes[] = {
    0, 0, '!', '@', '#', '$', '%', '^', '&', '*', /* 9 */
    '(', ')', '_', '+', 0, /* Backspace */
    0, /* Tab */
    'Q', 'W', 'E', 'R', /* 19 */
    'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', 0, /* Enter key */
    0, /* 29 - Control */
    'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', /* 39 */
    '"', '~',  0, /* Left shift */
    '|', 'Z', 'X', 'C', 'V', 'B', 'N', /* 49 */
    'M', '<', '>', '?', 0, /* Right shift */
    '*', 0, /* Alt */
    ' ', /* Space bar */
    0, /* Caps lock */
    0, /* 59 - F1 key ... > */
    0, 0, 0, 0, 0, 0, 0, 0, 0, /* < ... F10 */
    0, /* 69 - Num lock */
    0, /* Scroll Lock */
    0, /* Home key */
    0, /* Up Arrow */
    0, /* Page Up */
    '-', 0, /* Left Arrow */
    0, 0, /* Right Arrow */
    '+', 0, /* 79 - End key */
    0, /* Down Arrow */
    0, /* Page Down */
    0, /* Insert Key */
    0, /* Delete Key */
    0, 0, 0, 0, /* F11 Key */
    0, /* F12 Key */
    0 /* All other keys are undefined */
};
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

uint64_t execute(const CHAR16* file)
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
    return pid;
}

void quit()
{
    debug("Exiting program");
    unallocate(currentProgram->start);
    removeItem((void**)&running, currentProgram);
    debug("Exited program");
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

void coreDrop(uint64_t id)
{
    for (uint32_t y = 0; y < GOP->Mode->Info->VerticalResolution - 32; y++)
    {
        for (uint32_t x = 0; x < GOP->Mode->Info->HorizontalResolution / (cpuCount + 1); x++)
        {
            uint32_t newX = x + id * (GOP->Mode->Info->HorizontalResolution / (cpuCount + 1));
            videoBuffer[y * GOP->Mode->Info->HorizontalResolution + newX] = videoBuffer[(y + 32) * GOP->Mode->Info->HorizontalResolution + newX];
        }
    }
}

void print(const CHAR16* message)
{
    drawRectangle(cursorX * 16, cursorY * 32, 16, 32, black);
    uint64_t length = StrLen(message);
    for (uint64_t i = 0; i < length; i++)
    {
        if (*message != L'\n')
        {
            drawCharacter(*message, cursorX * 16, cursorY * 32, normal);
            cursorX++;
            if (cursorX == terminalWidth)
            {
                cursorX = 0;
                if (cursorY + 1 != terminalHeight)
                {
                    cursorY++;
                }
                else
                {
                    splitTask(coreDrop, cpuCount + 1);
                    drawRectangle(0, GOP->Mode->Info->VerticalResolution - 32, GOP->Mode->Info->HorizontalResolution, 32, black);
                }
            }
        }
        else
        {
            cursorX = 0;
            if (cursorY + 1 != terminalHeight)
            {
                cursorY++;
            }
            else
            {
                splitTask(coreDrop, cpuCount + 1);
                drawRectangle(0, GOP->Mode->Info->VerticalResolution - 32, GOP->Mode->Info->HorizontalResolution, 32, black);
            }
        }
        message++;
    }
    drawRectangle(cursorX * 16, cursorY * 32, 16, 32, normal);
}

uint64_t syscallHandle(uint64_t code, uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4, uint64_t arg5)
{
    switch (code)
    {
        case 0:
            return execute((const CHAR16*)arg1);
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
            return (uint64_t)checkFile((const CHAR16*)arg1);
            break;
        case 7:
            return (uint64_t)checkFolder((const CHAR16*)arg1);
            break;
        case 8:
            deleteFile((const CHAR16*)arg1);
            break;
        case 9:
            return (uint64_t)getFiles((const CHAR16*)arg1, (uint64_t*)arg2, (BOOLEAN)arg3);
            break;
        case 10:
            return cpuCount + 1;
            break;
        case 11:
            return (uint64_t)addKeyCall((void (*)(uint8_t scancode, BOOLEAN pressed))arg1);
            break;
        case 12:
            removeKeyCall((PointerArray*)arg1);
            break;
        case 13:
            return (uint64_t)addMoveCall((void (*)(int16_t x, int16_t y))arg1);
            break;
        case 14:
            removeMoveCall((PointerArray*)arg1);
            break;
        case 15:
            return (uint64_t)addClickCall((void (*)(BOOLEAN left, BOOLEAN pressed))arg1);
            break;
        case 16:
            removeClickCall((PointerArray*)arg1);
            break;
        case 17:
            return (uint64_t)registerSyscallHandler((const CHAR16*)arg1, (uint64_t (*)(uint64_t code, uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4))arg2);
            break;
        case 18:
            return (uint64_t)getRegisteredSyscall((const CHAR16*)arg1);
            break;
        case 19:
            unregisterSyscallHandler((const CHAR16*)arg1);
            break;
        case 20:
            getDisplayInfo((Display*)arg1);
            break;
        case 21:
            print((const CHAR16*)arg1);
            break;
        default:
            return syscallHandlers[code](arg1, arg2, arg3, arg4, arg5);
            break;
    }
    return 0;
}

void keyPress(uint8_t scancode, BOOLEAN pressed)
{
    if (typing)
    {
        switch (scancode)
        {
            case 14:
                if (pressed && cursorX + cursorY * terminalWidth > typingStart)
                {
                    typingCurrent--;
                    drawRectangle(cursorX * 16, cursorY * 32, 16, 32, black);
                    if (cursorX > 0)
                    {
                        cursorX--;
                    }
                    else
                    {
                        cursorX = terminalWidth - 1;
                        cursorY--;
                    }
                    drawRectangle(cursorX * 16, cursorY * 32, 16, 32, normal);
                }
                break;
            case 28:
                if (pressed)
                {
                    typingBuffer[typingCurrent] = L'\0';
                    print(L"\n");
                    typing = FALSE;
                }
                break;
            case 42:
                leftShift = pressed;
                shift = leftShift || rightShift;
                break;
            case 54:
                rightShift = pressed;
                shift = leftShift || rightShift;
                break;
            case 58:
                if (pressed)
                {
                    caps = !caps;
                }
                break;
            default:
                if (pressed)
                {
                    char character = (caps ? !shift : shift) ? capsScancodes[scancode] : scancodes[scancode];
                    if (character)
                    {
                        typingBuffer[typingCurrent] = character;
                        typingCurrent++;
                        CHAR16 message[2];
                        message[0] = character;
                        message[1] = L'\0';
                        print(message);
                    }
                }
                break;
        }
    }
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
    for (uint8_t i = 0; i < 22; i++)
    {
        syscallHandlers[i] = (void*)1;
    }
    debug("Allocating video buffer");
    videoBuffer = allocate(GOP->Mode->FrameBufferSize);
    ZeroMem(videoBuffer, GOP->Mode->FrameBufferSize);
    initGraphics(videoBuffer, GOP->Mode->Info->HorizontalResolution, readFile(L"/fonts/font.psf", NULL));
    debug("Setting up terminal");
    terminalWidth = GOP->Mode->Info->HorizontalResolution / 16;
    terminalHeight = GOP->Mode->Info->VerticalResolution / 32;
    debug("Allocating execution buffer");
    terminalDirectory = allocate(4);
    terminalDirectory[0] = L'/';
    terminalDirectory[1] = L'\0';
    debug("Allocating input buffer");
    typingBuffer = allocate(128 * 2);
    typingBuffer[0] = L'\0';
    uint64_t ticks = 0;
    debug("Starting loop");
    while (TRUE)
    {
        if (!typing && terminalPid == UINT64_MAX)
        {
            if (StrLen(typingBuffer) > 0)
            {
                uint64_t directoryLength = StrLen(terminalDirectory);
                uint64_t executeLength = directoryLength + StrLen(typingBuffer);
                CHAR16* executeString = allocate((executeLength + 1) * 2);
                StrCpy(executeString, terminalDirectory);
                StrCpy(executeString + directoryLength, typingBuffer);
                if (checkFile(executeString))
                {
                    terminalPid = execute(executeString);
                }
                else if (checkFolder(executeString))
                {
                    unallocate(terminalDirectory);
                    uint64_t directoryLength = StrLen(executeString);
                    terminalDirectory = allocate((directoryLength + 1) * 2);
                    StrCpy(terminalDirectory, executeString);
                }
                else if (StrCmp(typingBuffer, L"clear") == 0)
                {
                    fillColour = 0x0;
                    fillSize = (GOP->Mode->Info->HorizontalResolution * GOP->Mode->Info->VerticalResolution / 2) / (cpuCount + 1);
                    splitTask(coreFill, cpuCount + 1);
                    cursorX = 0;
                    cursorY = 0;
                }
                else if (StrCmp(typingBuffer, L"dir") == 0)
                {
                    uint64_t count = 0;
                    File** files = getFiles(terminalDirectory, &count, FALSE);
                    uint64_t length = StrLen(terminalDirectory);
                    for (uint64_t i = 0; i < count; i++)
                    {
                        print(files[i]->name + length);
                        print(L"\n");
                    }
                    unallocate(files);
                }
                else if (StrCmp(typingBuffer, L"..") == 0)
                {
                    if (terminalDirectory[1] != L'\0')
                    {
                        CHAR16* current = terminalDirectory;
                        uint64_t last = 0;
                        uint64_t end = 0;
                        while (*current)
                        {
                            end++;
                            if (*current == L'/' && *(current + 1) != L'\0')
                            {
                                last = end;
                            }
                            current++;
                        }
                        terminalDirectory[last] = L'\0';
                    }
                }
                else
                {
                    print(L"Command or executable file not found\n");
                }
                unallocate(executeString);
            }
        }
        BOOLEAN found = FALSE;
        Program* program = (Program*)&running;
        while (iterateList((void**)&program))
        {
            if (program->pid == terminalPid)
            {
                found = TRUE;
            }
            currentProgram = program;
            program->update(ticks);
        }
        if (!typing && !found)
        {
            terminalPid = UINT64_MAX;
            print(terminalDirectory);
            typingStart = cursorX + cursorY * terminalWidth;
            typingCurrent = 0;
            typing = TRUE;
        }
        blitTo = (uint64_t*)GOP->Mode->FrameBufferBase;
        blitFrom = (uint64_t*)videoBuffer;
        blitSize = (GOP->Mode->Info->HorizontalResolution * GOP->Mode->Info->VerticalResolution / 2) / (cpuCount + 1);
        splitTask(coreBlit, cpuCount + 1);
        ticks = hpetCounter;
        hpetCounter = 0;
    }
}
