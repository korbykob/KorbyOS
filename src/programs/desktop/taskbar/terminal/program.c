#include "../../desktop.h"

uint64_t cores = 0;
Window* window = NULL;
uint32_t terminalWidth = 0;
uint32_t terminalHeight = 0;
uint32_t cursorX = 0;
uint32_t cursorY = 0;
BOOLEAN typing = FALSE;
uint32_t typingStart = 0;
CHAR16* typingBuffer = NULL;
uint64_t typingCurrent = 0;
BOOLEAN running = FALSE;
BOOLEAN done = TRUE;
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

void coreDrop(uint64_t id)
{
    for (uint32_t y = 0; y < window->height - 32; y++)
    {
        for (uint32_t x = 0; x < window->width / cores; x++)
        {
            uint32_t newX = x + id * (window->width / cores);
            window->buffer[y * window->width + newX] = window->buffer[(y + 32) * window->width + newX];
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
                    splitTask(coreDrop, cores);
                    drawRectangle(0, window->height - 32, window->width, 32, black);
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
                splitTask(coreDrop, cores);
                drawRectangle(0, window->height - 32, window->width, 32, black);
            }
        }
        message++;
    }
    drawRectangle(cursorX * 16, cursorY * 32, 16, 32, normal);
}

void _start()
{
    initialiseDesktopCalls();
    cores = getCores();
    registerTerminal(terminalPrint);
    window = allocateWindow(800, 480, L"Terminal", L"/programs/desktop/taskbar/terminal/program.bmp");
    initGraphics(window->buffer, window->width, readFile(L"/fonts/font.psf", NULL));
    drawRectangle(0, 0, window->width, window->height, black);
    terminalWidth = window->width / 16;
    terminalHeight = window->height / 32;
    terminalDirectory = allocate(4);
    terminalDirectory[0] = L'/';
    terminalDirectory[1] = L'\0';
    typingBuffer = allocate(128 * 2);
    typingBuffer[0] = L'\0';
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
}

void update(uint64_t ticks)
{
    Event* event = (Event*)&window->events;
    while (iterateList(&event))
    {
        switch (event->id)
        {
            case 0:
                cancelWait(&done);
                unallocate(typingBuffer);
                unallocate(terminalDirectory);
                unallocateWindow(window);
                unregisterTerminal();
                quit();
                break;
            case 1:
                keyPress(((KeyEvent*)event)->scancode, ((KeyEvent*)event)->pressed);
                break;
        }
        removeItem(&window->events, event);
        event = (Event*)&window->events;
    }
    if (!typing && !running)
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
                    done = FALSE;
                    running = TRUE;
                    waitForProgram(execute(executeString), &done);
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
                fillSize = (window->width * window->height / 2) / cores;
                splitTask(coreFill, cores);
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
            else if (StrCmp(typingBuffer, L"usage") == 0)
            {
                print(L"Using ");
                CHAR16 usedMessage[100];
                FloatToString(usedMessage, FALSE, (getUsedRam() / 10) / 100.0);
                print(usedMessage);
                print(L" KB of ram.\n");
            }
            else
            {
                print(L"Command or executable file not found.\n");
            }
            unallocate(executeString);
        }
    }
    if (!typing && done)
    {
        running = FALSE;
        print(terminalDirectory);
        typingStart = cursorX + cursorY * terminalWidth;
        typingCurrent = 0;
        typing = TRUE;
    }
}
