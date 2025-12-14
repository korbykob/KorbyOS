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
    uint8_t terminalId;
    uint8_t* memory;
    void (*start)();
    void (*update)(uint64_t ticks);
} Program;
Program* running = NULL;
Program* currentProgram = NULL;
PointerArray* keyCalls = NULL;
PointerArray* moveCalls = NULL;
PointerArray* clickCalls = NULL;
typedef struct {
    void* next;
    CHAR16* name;
    uint8_t id;
} Syscall;
Syscall* syscallIds = NULL;
uint64_t (*syscallHandlers[256])(uint64_t code, uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4);
typedef struct
{
    void* next;
    uint64_t id;
} Terminal;
Terminal* terminalIds = NULL;
void (*terminals[256])(const CHAR16* message);
EFI_GRAPHICS_OUTPUT_BLT_PIXEL* videoBuffer = NULL;
typedef struct
{
    void* next;
    uint64_t pid;
    BOOLEAN* done;
} WaitingProgram;
WaitingProgram* waitingPrograms = NULL;
typedef struct
{
    uint8_t destination[6];
    uint8_t source[6];
    uint16_t type;
} __attribute__((packed)) EthernetLayer;
typedef struct
{
    uint8_t version: 4;
    uint8_t ihl: 4;
    uint8_t type;
    uint16_t length;
    uint16_t identification;
    uint16_t flags;
    uint8_t ttl;
    uint8_t protocol;
    uint16_t checksum;
    uint8_t source[4];
    uint8_t destination[4];
} __attribute__((packed)) Ipv4Layer;
typedef struct
{
    uint16_t source;
    uint16_t destination;
    uint16_t length;
    uint16_t checksum;
} __attribute__((packed)) UdpLayer;
typedef struct
{
    uint16_t hardwareType;
    uint16_t protocolType;
    uint8_t hardwareSize;
    uint8_t protocolSize;
    uint16_t opcode;
    uint8_t sourceMac[6];
    uint8_t sourceIp[4];
    uint8_t destinationMac[6];
    uint8_t destinationIp[4];
} __attribute__((packed)) ArpLayer;
typedef struct
{
    uint8_t op;
    uint8_t htype;
    uint8_t hlen;
    uint8_t hops;
    uint32_t xid; 
    uint16_t secs;
    uint16_t flags; 
    uint8_t ciaddr[4];
    uint8_t yiaddr[4];
    uint8_t siaddr[4];
    uint8_t giaddr[4];
    uint8_t chaddr[16];
    uint8_t sname[64];
    uint8_t file[128]; 
    uint8_t options[312];
} __attribute__((packed)) DhcpLayer;
uint8_t router[6] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
uint8_t ip[4] = { 0, 0, 0, 0 };
uint8_t dns[4] = { 0, 0, 0, 0 };
typedef struct
{
    void* next;
    uint16_t port;
    Connection connection;
} ConnectionData;
ConnectionData* connections = NULL;

uint64_t execute(const CHAR16* file)
{
    debug("Executing program");
    uint64_t pid = 0;
    Program* iterator = (Program*)&running;
    while (iterateList(&iterator))
    {
        if (pid == iterator->pid)
        {
            pid++;
            iterator = (Program*)&running;
        }
    }
    debug("Allocating program");
    Program* program = addItemFirst(&running, sizeof(Program));
    program->pid = pid;
    program->terminalId = currentProgram ? currentProgram->terminalId : 0;
    uint64_t size = 0;
    uint8_t* data = readFile(file, &size);
    debug("Allocating binary");
    program->memory = allocate(size - 8);
    copyMemory(data + 8, program->memory, size - 8);
    program->start = (void*)(program->memory + *(uint32_t*)data - 8);
    program->update = (void*)(program->memory + *(uint32_t*)(data + 4) - 8);
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
    removeItem(&running, currentProgram);
    WaitingProgram* iterator = (WaitingProgram*)&waitingPrograms;
    while (iterateList(&iterator))
    {
        if (iterator->pid == currentProgram->pid)
        {
            debug("Removing waiting program");
            *iterator->done = TRUE;
            removeItem(&waitingPrograms, iterator);
            debug("Removed waiting program");
            break;
        }
    }
    unallocate(currentProgram->memory);
    debug("Exited program");
}

PointerArray* addKeyCall(void (*keyCall)(uint8_t scancode, BOOLEAN pressed))
{
    debug("Adding key press call");
    PointerArray* item = addItem(&keyCalls, sizeof(PointerArray));
    item->pointer = keyCall;
    debug("Added key press call");
    return item;
}

void removeKeyCall(PointerArray* keyCall)
{
    debug("Removing key press call");
    removeItem(&keyCalls, keyCall);
    debug("Removed key press call");
}

PointerArray* addMoveCall(void (*moveCall)(int16_t x, int16_t y))
{
    debug("Adding mouse move call");
    PointerArray* item = addItem(&moveCalls, sizeof(PointerArray));
    item->pointer = moveCall;
    debug("Added mouse move call");
    return item;
}

void removeMoveCall(PointerArray* moveCall)
{
    debug("Removing mouse move call");
    removeItem(&moveCalls, moveCall);
    debug("Removed mouse move call");
}

PointerArray* addClickCall(void (*clickCall)(BOOLEAN left, BOOLEAN pressed))
{
    debug("Adding mouse click call");
    PointerArray* item = addItem(&clickCalls, sizeof(PointerArray));
    item->pointer = clickCall;
    debug("Added mouse click call");
    return item;
}

void removeClickCall(PointerArray* clickCall)
{
    debug("Removing mouse click call");
    removeItem(&clickCalls, clickCall);
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
    Syscall* syscall = addItem(&syscallIds, sizeof(Syscall));
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
    while (iterateList(&iterator))
    {
        if (StrCmp(name, iterator->name) == 0)
        {
            debug("Got registered syscall");
            return iterator->id;
        }
    }
    return 0;
}

void unregisterSyscallHandler(uint8_t id)
{
    debug("Unregistering syscall handler");
    Syscall* iterator = (Syscall*)&syscallIds;
    while (iterateList(&iterator))
    {
        if (iterator->id == id)
        {
            debug("Found syscall handler");
            syscallHandlers[iterator->id] = NULL;
            unallocate(iterator->name);
            removeItem(&syscallIds, iterator);
            debug("Unregistered syscall handler");
        }
    }
}

void registerTerminal(void (*function)(const CHAR16* message))
{
    debug("Registering terminal");
    uint8_t id = 0;
    while (TRUE)
    {
        if (terminals[id] == NULL)
        {
            debug("Found unused terminal id");
            break;
        }
        id++;
    }
    terminals[id] = function;
    debug("Allocating terminal id");
    Terminal* terminal = addItem(&terminalIds, sizeof(Terminal));
    terminal->id = id;
    currentProgram->terminalId = id;
    debug("Registered terminal");
}

void print(const CHAR16* message)
{
    terminals[currentProgram ? currentProgram->terminalId : 0](message);
}

void unregisterTerminal()
{
    debug("Unregistering terminal");
    Terminal* iterator = (Terminal*)&terminalIds;
    while (iterateList(&iterator))
    {
        if (iterator->id == currentProgram->terminalId)
        {
            debug("Found terminal");
            terminals[iterator->id] = NULL;
            removeItem(&terminalIds, iterator);
            debug("Unregistered terminal");
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

void waitForProgram(uint64_t pid, BOOLEAN* done)
{
    debug("Allocating waiting id");
    Program* iterator = (Program*)&running;
    while (iterateList(&iterator))
    {
        if (pid == iterator->pid)
        {
            debug("Allocated waiting id");
            WaitingProgram* waiting = addItem(&waitingPrograms, sizeof(WaitingProgram));
            waiting->pid = pid;
            waiting->done = done;
            return;
        }
    }
    debug("Pid does not exist");
    *done = TRUE;
}

void cancelWait(BOOLEAN* done)
{
    debug("Removing wait");
    WaitingProgram* iterator = (WaitingProgram*)&waitingPrograms;
    while (iterateList(&iterator))
    {
        if (iterator->done == done)
        {
            debug("Found wait");
            removeItem(&waitingPrograms, iterator);
            debug("Removed wait");
        }
    }
}

uint64_t getUsedRam()
{
    uint64_t used = 0;
    Allocation* allocation = allocated;
    uint64_t count = 0;
    while (count != allocations)
    {
        if (allocation->present)
        {
            count++;
            used += allocation->end - allocation->start;
        }
        allocation--;
    }
    return used;
}

void getIpInfo(IpInfo* info)
{
    debug("Getting IP info");
    for (uint8_t i = 0; i < 4; i++)
    {
        info->ip[i] = ip[i];
        info->dns[i] = dns[i];
    }
    debug("Got IP info");
}

Connection* createConnection(uint16_t port)
{
    debug("Creating connection");
    ConnectionData* data = addItem(&connections, sizeof(ConnectionData));
    data->port = port;
    data->connection.received = NULL;
    data->connection.send = NULL;
    debug("Created connection");
    return &data->connection;
}

void closeConnection(Connection* connection)
{
    debug("Closing connection");
    ConnectionData* iterator = (ConnectionData*)&connections;
    while (iterateList(&iterator))
    {
        if (&iterator->connection == connection)
        {
            debug("Found connection");
            unallocateList(&connection->received);
            unallocateList(&connection->send);
            removeItem(&connections, iterator);
            debug("Closed connection");
            break;
        }
    }
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
            unregisterSyscallHandler((uint8_t)arg1);
            break;
        case 20:
            registerTerminal((void (*)(const CHAR16 *message))arg1);
            break;
        case 21:
            print((const CHAR16*)arg1);
            break;
        case 22:
            unregisterTerminal();
            break;
        case 23:
            getDisplayInfo((Display*)arg1);
            break;
        case 24:
            waitForProgram(arg1, (BOOLEAN*)arg2);
            break;
        case 25:
            cancelWait((BOOLEAN*)arg1);
            break;
        case 26:
            return getUsedRam();
            break;
        case 27:
            getIpInfo((IpInfo*)arg1);
            break;
        case 28:
            return (uint64_t)createConnection((uint16_t)arg1);
            break;
        case 29:
            closeConnection((Connection*)arg1);
            break;
        default:
            return syscallHandlers[code](arg1, arg2, arg3, arg4, arg5);
            break;
    }
    return 0;
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

void terminalPrint(const CHAR16* message)
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
                    typingStart -= terminalWidth;
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
                typingStart -= terminalWidth;
                splitTask(coreDrop, cpuCount + 1);
                drawRectangle(0, GOP->Mode->Info->VerticalResolution - 32, GOP->Mode->Info->HorizontalResolution, 32, black);
            }
        }
        message++;
    }
    drawRectangle(cursorX * 16, cursorY * 32, 16, 32, normal);
}

void start()
{
    debug("Filling syscall handlers");
    for (uint8_t i = 0; i < 30; i++)
    {
        syscallHandlers[i] = (void*)1;
    }
    debug("Allocating video buffer");
    videoBuffer = allocate(GOP->Mode->FrameBufferSize);
    ZeroMem(videoBuffer, GOP->Mode->FrameBufferSize);
    initGraphics(videoBuffer, GOP->Mode->Info->HorizontalResolution, readFile(L"/fonts/font.psf", NULL));
    debug("Setting up terminal");
    Terminal* newId = addItem(&terminalIds, sizeof(Terminal));
    newId->id = 0;
    terminals[0] = terminalPrint;
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
    debug("Displaying welcome information");
    print(L"Welcome to KorbyOS!\nUsable memory: ");
    CHAR16 memoryMessage[7];
    FloatToString(memoryMessage, FALSE, (memorySize / 10000000) / 100.0);
    print(memoryMessage);
    print(L" GB\nCPU cores: ");
    CHAR16 coreMessage[3];
    ValueToString(coreMessage, FALSE, cpuCount + 1);
    print(coreMessage);
    print(L"\n\n");
    Connection* dhcpConnection = createConnection(68);
    debug("Allocating DHCP request");
    Packet* dhcpPacket = addItem(&dhcpConnection->send, sizeof(Packet) + sizeof(DhcpLayer));
    for (uint8_t i = 0; i < 4; i++)
    {
        dhcpPacket->ip[i] = 255;
    }
    dhcpPacket->port = 67;
    dhcpPacket->length = sizeof(DhcpLayer);
    DhcpLayer* dhcpRequest = (DhcpLayer*)dhcpPacket->data;
    debug("Clearing out DHCP data");
    for (uint16_t i = 0; i < sizeof(DhcpLayer); i++)
    {
        ((uint8_t*)dhcpRequest)[i] = 0;
    }
    debug("Setting up DHCP request");
    dhcpRequest->op = 1;
    dhcpRequest->htype = 1;
    dhcpRequest->hlen = 6;
    dhcpRequest->xid = 0x78563412;
    for (uint8_t i = 0; i < 4; i++)
    {
        dhcpRequest->yiaddr[i] = 0;
    }
    for (uint8_t i = 0; i < 6; i++)
    {
        dhcpRequest->chaddr[i] = mac[i];
    }
    dhcpRequest->options[0] = 0x63;
    dhcpRequest->options[1] = 0x82;
    dhcpRequest->options[2] = 0x53;
    dhcpRequest->options[3] = 0x63;
    dhcpRequest->options[4] = 0x35;
    dhcpRequest->options[5] = 0x01;
    dhcpRequest->options[6] = 0x01;
    dhcpRequest->options[7] = 0x37;
    dhcpRequest->options[8] = 0x03;
    dhcpRequest->options[9] = 1;
    dhcpRequest->options[10] = 3;
    dhcpRequest->options[11] = 6;
    dhcpRequest->options[12] = 0xFF;
    debug("Starting loop");
    while (TRUE)
    {
        if (!ip[0])
        {
            Packet* packet = (Packet*)&dhcpConnection->received;
            while (iterateList(&packet))
            {
                debug("Got DHCP response");
                DhcpLayer* recieved = (DhcpLayer*)packet->data;
                debug("Updating new IP address from response");
                for (uint8_t i = 0; i < 4; i++)
                {
                    ip[i] = recieved->yiaddr[i];
                }
                uint8_t* option = packet->data + 240;
                debug("Reading DHCP options");
                while (*option != 0xFF)
                {
                    if (*option == 0x06)
                    {
                        debug("Saving DNS IP");
                        for (uint8_t i = 0; i < 4; i++)
                        {
                            dns[i] = option[2 + i];
                        }
                    }
                    option += *(option + 1) + 2;
                }
                debug("Allocating DHCP response");
                Packet* send = addItem(&dhcpConnection->send, sizeof(Packet) + sizeof(DhcpLayer));
                for (uint8_t i = 0; i < 4; i++)
                {
                    send->ip[i] = packet->ip[i];
                }
                send->port = 67;
                send->length = sizeof(DhcpLayer);
                dhcpRequest = (DhcpLayer*)send->data;
                debug("Clearing out DHCP data");
                for (uint16_t i = 0; i < sizeof(DhcpLayer); i++)
                {
                    ((uint8_t*)dhcpRequest)[i] = 0;
                }
                debug("Setting up DHCP response");
                dhcpRequest->op = 1;
                dhcpRequest->htype = 1;
                dhcpRequest->hlen = 6;
                dhcpRequest->xid = 0x78563412;
                for (uint8_t i = 0; i < 4; i++)
                {
                    dhcpRequest->yiaddr[i] = 0;
                }
                for (uint8_t i = 0; i < 6; i++)
                {
                    dhcpRequest->chaddr[i] = mac[i];
                }
                dhcpRequest->options[0] = 0x63;
                dhcpRequest->options[1] = 0x82;
                dhcpRequest->options[2] = 0x53;
                dhcpRequest->options[3] = 0x63;
                dhcpRequest->options[4] = 0x35;
                dhcpRequest->options[5] = 0x01;
                dhcpRequest->options[6] = 0x03;
                dhcpRequest->options[7] = 0x32;
                dhcpRequest->options[8] = 0x04;
                dhcpRequest->options[9] = recieved->yiaddr[0];
                dhcpRequest->options[10] = recieved->yiaddr[1];
                dhcpRequest->options[11] = recieved->yiaddr[2];
                dhcpRequest->options[12] = recieved->yiaddr[3];
                dhcpRequest->options[13] = 0x36;
                dhcpRequest->options[14] = 0x04;
                dhcpRequest->options[15] = packet->ip[0];
                dhcpRequest->options[16] = packet->ip[1];
                dhcpRequest->options[17] = packet->ip[2];
                dhcpRequest->options[18] = packet->ip[3];
                dhcpRequest->options[19] = 0xFF;
                break;
            }
        }
        for (uint16_t i = 0; i < currentKeyPress; i++)
        {
            if (typing)
            {
                switch (keyPresses[i].scancode)
                {
                    case 14:
                        if (keyPresses[i].pressed && cursorX + cursorY * terminalWidth > typingStart)
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
                        if (keyPresses[i].pressed)
                        {
                            typingBuffer[typingCurrent] = L'\0';
                            print(L"\n");
                            typing = FALSE;
                        }
                        break;
                    case 42:
                        leftShift = keyPresses[i].pressed;
                        shift = leftShift || rightShift;
                        break;
                    case 54:
                        rightShift = keyPresses[i].pressed;
                        shift = leftShift || rightShift;
                        break;
                    case 58:
                        if (keyPresses[i].pressed)
                        {
                            caps = !caps;
                        }
                        break;
                    default:
                        if (keyPresses[i].pressed)
                        {
                            char character = (caps ? !shift : shift) ? capsScancodes[keyPresses[i].scancode] : scancodes[keyPresses[i].scancode];
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
            while (iterateList(&call))
            {
                ((void (*)(uint8_t scancode, BOOLEAN pressed))call->pointer)(keyPresses[i].scancode, keyPresses[i].pressed);
            }
        }
        currentKeyPress = 0;
        for (uint16_t i = 0; i < currentMouseMove; i++)
        {
            PointerArray* call = (PointerArray*)&moveCalls;
            while (iterateList(&call))
            {
                ((void (*)(int16_t x, int16_t y))call->pointer)(mouseMoves[i].x, mouseMoves[i].y);
            }
        }
        currentMouseMove = 0;
        for (uint16_t i = 0; i < currentMouseClick; i++)
        {
            PointerArray* call = (PointerArray*)&clickCalls;
            while (iterateList(&call))
            {
                ((void (*)(BOOLEAN left, BOOLEAN pressed))call->pointer)(mouseClicks[i].left, mouseClicks[i].pressed);
            }
        }
        currentMouseClick = 0;
        for (uint16_t i = 0; i < currentEthernetPacket; i++)
        {
            EthernetLayer* ethernet = (EthernetLayer*)ethernetPackets[i].data;
            BOOLEAN found = TRUE;
            for (uint8_t i2 = 0; i2 < 6; i2++)
            {
                if (ethernet->destination[i2] != mac[i2] && ethernet->destination[i2] != 0xFF)
                {
                    found = FALSE;
                }
            }
            if (found)
            {
                if (ethernet->type == 0x608)
                {
                    ArpLayer* arp = (ArpLayer*)((uint64_t)ethernet + sizeof(EthernetLayer));
                    found = TRUE;
                    for (uint8_t i2 = 0; i2 < 6; i2++)
                    {
                        if (arp->destinationMac[i2] != mac[i])
                        {
                            found = FALSE;
                        }
                    }
                    if (!found)
                    {
                        found = TRUE;
                        for (uint8_t i2 = 0; i2 < 4; i2++)
                        {
                            if (arp->destinationIp[i2] != ip[i2])
                            {
                                found = FALSE;
                            }
                        }
                    }
                    if (found && arp->opcode == 0x0100)
                    {
                        uint8_t* data = allocate(sizeof(EthernetLayer) + sizeof(ArpLayer));
                        EthernetLayer* responseEthernet = (EthernetLayer*)data;
                        for (uint8_t i2 = 0; i2 < 6; i2++)
                        {
                            responseEthernet->source[i2] = mac[i2];
                            responseEthernet->destination[i2] = ethernet->source[i2];
                        }
                        responseEthernet->type = 0x608;
                        ArpLayer* responseArp = (ArpLayer*)((uint64_t)responseEthernet + sizeof(EthernetLayer));
                        responseArp->hardwareType = 0x0100;
                        responseArp->protocolType = 0x08;
                        responseArp->hardwareSize = 6;
                        responseArp->protocolSize = 4;
                        responseArp->opcode = 0x0200;
                        for (uint8_t i2 = 0; i2 < 6; i2++)
                        {
                            responseArp->sourceMac[i2] = mac[i2];
                            responseArp->destinationMac[i2] = ethernet->source[i2];
                        }
                        for (uint8_t i2 = 0; i2 < 4; i2++)
                        {
                            responseArp->sourceIp[i2] = ip[i2];
                            responseArp->destinationIp[i2] = arp->sourceIp[i2];
                        }
                        sendPacket(data, sizeof(EthernetLayer) + sizeof(ArpLayer));
                        unallocate(data);
                    }
                }
                else if (ethernet->type == 0x8)
                {
                    Ipv4Layer* ipv4 = (Ipv4Layer*)((uint64_t)ethernet + sizeof(EthernetLayer));
                    if (ipv4->protocol == 0x11)
                    {
                        UdpLayer* udp = (UdpLayer*)((uint64_t)ipv4 + sizeof(Ipv4Layer));
                        ConnectionData* connection = (ConnectionData*)&connections;
                        while (iterateList(&connection))
                        {
                            if (connection->port == (((udp->destination & 0xFF) << 8) | ((udp->destination >> 8) & 0xFF)))
                            {
                                if (router[0] == 0xFF)
                                {
                                    for (uint8_t i = 0; i < 6; i++)
                                    {
                                        router[i] = ethernet->source[i];
                                    }
                                }
                                uint64_t length = (((udp->length & 0xFF) << 8) | ((udp->length >> 8) & 0xFF)) - sizeof(UdpLayer);
                                Packet* packet = addItem(&connection->connection.received, sizeof(Packet) + length);
                                for (uint8_t i = 0; i < 4; i++)
                                {
                                    packet->ip[i] = ipv4->source[i];
                                }
                                packet->port = connection->port;
                                packet->length = length;
                                copyMemory((uint8_t*)((uint64_t)udp + sizeof(UdpLayer)), packet->data, packet->length);
                            }
                        }
                    }
                }
            }
        }
        currentEthernetPacket = 0;
        ConnectionData* connection = (ConnectionData*)&connections;
        while (iterateList(&connection))
        {
            Packet* packet = (Packet*)&connection->connection.send;
            while (iterateList(&packet))
            {
                uint8_t* data = allocate(sizeof(EthernetLayer) + sizeof(Ipv4Layer) + sizeof(UdpLayer) + packet->length);
                EthernetLayer* ethernet = (EthernetLayer*)data;
                for (uint8_t i = 0; i < 6; i++)
                {
                    ethernet->destination[i] = router[i];
                    ethernet->source[i] = mac[i];
                }
                ethernet->type = 0x8;
                Ipv4Layer* ipv4 = (Ipv4Layer*)((uint64_t)ethernet + sizeof(EthernetLayer));
                ipv4->version = 5;
                ipv4->ihl = 4;
                ipv4->type = 0;
                uint16_t size = sizeof(Ipv4Layer) + sizeof(UdpLayer) + packet->length;
                ipv4->length = ((size & 0xFF) << 8) | ((size >> 8) & 0xFF);
                ipv4->identification = 0;
                ipv4->flags = 0;
                ipv4->ttl = 0x80;
                ipv4->protocol = 0x11;
                ipv4->checksum = 0;
                for (uint8_t i = 0; i < 4; i++)
                {
                    ipv4->source[i] = ip[i];
                    ipv4->destination[i] = packet->ip[i];
                }
                uint32_t ipv4Checksum = 0;
                for (uint8_t i = 0; i < 10; i++)
                {
                    ipv4Checksum += ((uint16_t*)ipv4)[i];
                }
                while (ipv4Checksum >> 16)
                {
                    ipv4Checksum = ((ipv4Checksum & 0xFFFF) + (ipv4Checksum >> 16));
                }
                ipv4->checksum = ~ipv4Checksum;
                UdpLayer* udp = (UdpLayer*)((uint64_t)ipv4 + sizeof(Ipv4Layer));
                udp->source = ((connection->port & 0xFF) << 8) | ((connection->port >> 8) & 0xFF);
                udp->destination = ((packet->port & 0xFF) << 8) | ((packet->port >> 8) & 0xFF);
                uint16_t udpSize = sizeof(UdpLayer) + packet->length;
                udp->length = ((udpSize & 0xFF) << 8) | ((udpSize >> 8) & 0xFF);
                udp->checksum = 0;
                copyMemory(packet->data, (uint8_t*)((uint64_t)udp + sizeof(UdpLayer)), packet->length);
                uint64_t udpAddress = (uint64_t)udp;
                uint16_t* udpMemory = (uint16_t*)udpAddress;
                uint32_t checksum = 0;
                for (uint16_t i = 0; i < ((sizeof(UdpLayer) + packet->length) / 2); i++)
                {
                    checksum += *udpMemory;
                    udpMemory++;
                }
                if (((sizeof(UdpLayer) + packet->length) & 0x1) == 0x1)
                {
                    checksum += (*udpMemory & 0xFF);
                }
                checksum += (udp->length + (0x0011 << 8) + (ipv4->source[0] | ipv4->source[1] << 8) + (ipv4->source[2] | ipv4->source[3] << 8) + (ipv4->destination[0] | ipv4->destination[1] << 8) + (ipv4->destination[2] | ipv4->destination[3] << 8));
                while ((checksum >> 16) > 0)
                {
                    checksum = ((checksum & 0xFFFF) + (checksum >> 16));
                } 
                udp->checksum = ~checksum;
                sendPacket(data, sizeof(EthernetLayer) + sizeof(Ipv4Layer) + sizeof(UdpLayer) + packet->length);
                unallocate(data);
            }
            unallocateList(&connection->connection.send);
            connection->connection.send = NULL;
        }
        if (ip[0] && dhcpConnection)
        {
            closeConnection(dhcpConnection);
            dhcpConnection = NULL;
        }
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
                    if (StrCmp(executeString + executeLength - 4, L".bin") == 0)
                    {
                        terminalPid = execute(executeString);
                    }
                    else
                    {
                        print(L"File is not executable.\n");
                    }
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
                        uint64_t length = StrLen(terminalDirectory);
                        CHAR16* current = terminalDirectory + length - 2;
                        uint64_t count = 0;
                        while (*current-- != L'/')
                        {
                            count++;
                        }
                        CHAR16* old = terminalDirectory;
                        uint64_t newLength = (length - count) * 2;
                        terminalDirectory = allocate(newLength);
                        copyMemory((uint8_t*)old, (uint8_t*)terminalDirectory, newLength);
                        unallocate(old);
                        terminalDirectory[length - count - 1] = L'\0';
                    }
                }
                else if (StrCmp(typingBuffer, L"usage") == 0)
                {
                    print(L"Using ");
                    CHAR16 usedMessage[100];
                    uint64_t total = getUsedRam();
                    uint64_t kb = total / 1000;
                    ValueToString(usedMessage, FALSE, kb);
                    print(usedMessage);
                    print(L" KB and ");
                    ValueToString(usedMessage, FALSE, total - kb * 1000);
                    print(usedMessage);
                    print(L" bytes of ram.\n");
                }
                else if (StrCmp(typingBuffer, L"ip") == 0)
                {
                    IpInfo info;
                    getIpInfo(&info);
                    print(L"IP address: ");
                    CHAR16 characters[4];
                    for (uint8_t i = 0; i < 4; i++)
                    {
                        ValueToString(characters, FALSE, info.ip[i]);
                        print(characters);
                        if (i != 3)
                        {
                            print(L".");
                        }
                    }
                    print(L"\nDNS address: ");
                    for (uint8_t i = 0; i < 4; i++)
                    {
                        ValueToString(characters, FALSE, info.dns[i]);
                        print(characters);
                        if (i != 3)
                        {
                            print(L".");
                        }
                    }
                    print(L"\n");
                }
                else
                {
                    print(L"Command or executable file not found.\n");
                }
                unallocate(executeString);
            }
        }
        BOOLEAN found = FALSE;
        Program* program = (Program*)&running;
        while (iterateList(&program))
        {
            if (program->pid == terminalPid)
            {
                found = TRUE;
            }
            currentProgram = program;
            program->update(ticks);
        }
        currentProgram = NULL;
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
