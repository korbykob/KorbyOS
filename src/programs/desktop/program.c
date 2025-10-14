#include "../../program.h"

Display display = {};
EFI_GRAPHICS_OUTPUT_BLT_PIXEL* wallpaper = NULL;
typedef struct
{
    void* next;
    const CHAR16* name;
    EFI_GRAPHICS_OUTPUT_BLT_PIXEL icon[24 * 24];
    uint64_t size;
    uint8_t* data;
} ProgramData;
ProgramData* programs = NULL;
uint8_t programCount = 1;
uint8_t mouseIcon[] = {
    00,00,00,02,02,02,02,02,02,02,02,02,02,02,02,02,
    00,01,00,00,00,02,02,02,02,02,02,02,02,02,02,02,
    00,01,01,01,01,00,00,00,02,02,02,02,02,02,02,02,
    00,01,01,01,01,01,01,00,00,02,02,02,02,02,02,02,
    00,00,01,01,01,01,01,01,01,00,00,00,02,02,02,02,
    00,00,01,01,01,01,01,01,01,01,00,00,02,02,02,02,
    02,00,01,01,01,01,01,01,01,01,00,02,02,02,02,02,
    02,00,01,01,01,01,01,01,01,00,02,02,02,02,02,02,
    02,00,00,01,01,01,01,00,00,00,00,02,02,02,02,02,
    02,02,00,01,00,00,00,00,01,01,00,00,02,02,02,02,
    02,02,00,00,00,02,00,00,01,01,01,00,00,00,00,02,
    02,02,02,00,02,02,02,00,00,00,01,01,01,01,00,02,
    02,02,02,02,02,02,02,02,02,02,00,00,00,01,00,02,
    02,02,02,02,02,02,02,02,02,02,02,02,00,00,00,02,
    02,02,02,02,02,02,02,02,02,02,02,02,02,00,02,02,
    02,02,02,02,02,02,02,02,02,02,02,02,02,02,02,02,
};
uint8_t exitIcon[] = {
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,
    1,1,1,1,1,0,0,0,1,1,1,0,0,0,0,0,0,0,0,1,1,1,1,1,
    1,1,1,1,1,0,0,0,1,1,1,0,0,0,0,0,0,0,0,1,1,1,1,1,
    1,1,1,1,1,0,0,0,1,1,1,0,0,0,0,0,0,0,0,1,1,1,1,1,
    1,1,1,1,1,0,0,0,0,0,1,1,1,1,1,1,0,0,0,1,1,1,1,1,
    1,1,1,1,1,0,0,0,0,1,1,1,1,1,1,1,1,0,0,1,1,1,1,1,
    1,1,1,1,1,0,0,0,1,1,1,1,1,1,0,1,1,0,0,1,1,1,1,1,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,1,1,0,1,1,1,1,1,
    1,1,1,1,1,1,1,1,1,0,1,1,1,1,0,0,1,1,0,1,1,1,1,1,
    1,1,1,1,1,0,0,0,0,0,0,1,1,1,1,0,0,0,0,1,1,1,1,1,
    1,1,1,1,1,0,0,0,0,0,0,1,1,1,1,0,0,0,0,1,1,1,1,1,
    1,1,1,1,1,0,0,0,0,0,1,1,1,1,1,0,0,0,0,1,1,1,1,1,
    1,1,1,1,1,0,0,0,0,0,1,1,0,1,1,0,0,0,0,1,1,1,1,1,
    1,1,1,1,1,0,0,0,0,1,1,1,0,1,1,0,0,0,0,1,1,1,1,1,
    1,1,1,1,1,0,0,0,0,1,1,0,0,1,1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,1,0,0,0,1,1,1,0,0,0,1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,1,0,0,0,1,1,0,0,0,0,0,0,0,0,0,1,1,1,1,1,
    1,1,1,1,1,0,0,1,1,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,
    1,1,1,1,1,0,0,1,1,1,0,0,0,0,0,0,0,0,0,1,1,1,1,1,
    1,1,1,1,1,1,0,0,1,1,1,0,0,0,0,0,0,0,0,1,1,1,1,1,
    1,1,1,1,1,1,1,0,0,1,1,1,0,0,0,0,0,0,0,0,1,1,1,1,
    1,1,1,1,1,1,1,1,0,0,1,1,1,0,0,0,0,0,0,0,0,1,1,1,
};
int64_t mouseX = 0;
int64_t mouseY = 0;
Window* windows = NULL;
Window* dragging = NULL;
int64_t draggingX = 0;
int64_t draggingY = 0;
Window* focus = NULL;
typedef struct
{
    void* next;
    Window* window;
} Taskbar;
Taskbar* taskbar = NULL;
uint64_t cores = 0;
PointerArray* key = NULL;
PointerArray* move = NULL;
PointerArray* click = NULL;
uint8_t syscall = 0;
BOOLEAN exiting = FALSE;

Window* allocateWindow(uint32_t width, uint32_t height, const CHAR16* title, const CHAR16* icon)
{
    debug("Allocating window");
    Window* window = addItem(&windows, sizeof(Window));
    window->x = display.width / 2 - (width + 20) / 2;
    window->y = display.height / 2 - (height + 57) / 2;
    window->width = width;
    window->height = height;
    debug("Reallocating title");
    window->title = allocate((StrLen(title) + 1) * 2);
    StrCpy(window->title, title);
    debug("Allocating icon");
    window->icon = allocate(24 * 24 * sizeof(EFI_GRAPHICS_OUTPUT_BLT_PIXEL));
    readBitmap(readFile(icon, NULL), window->icon);
    debug("Allocating video buffer");
    window->buffer = allocate(width * height * sizeof(EFI_GRAPHICS_OUTPUT_BLT_PIXEL));
    window->mouseMode = 0;
    window->fullscreen = FALSE;
    window->minimised = FALSE;
    window->events = NULL;
    focus = window;
    debug("Adding to taskbar");
    Taskbar* item = addItem(&taskbar, sizeof(Taskbar));
    item->window = window;
    debug("Allocated window");
    return window;
}

Window* allocateFullscreenWindow(const CHAR16* icon)
{
    debug("Allocating fullscreen window");
    Window* window = addItem(&windows, sizeof(Window));
    window->x = 0;
    window->y = 0;
    window->width = display.width;
    window->height = display.height;
    window->title = NULL;
    debug("Allocating icon");
    window->icon = allocate(24 * 24 * sizeof(EFI_GRAPHICS_OUTPUT_BLT_PIXEL));
    readBitmap(readFile(icon, NULL), window->icon);
    debug("Allocating video buffer");
    window->buffer = allocate(window->width * window->height * sizeof(EFI_GRAPHICS_OUTPUT_BLT_PIXEL));
    window->mouseMode = 0;
    window->fullscreen = TRUE;
    window->minimised = FALSE;
    window->events = NULL;
    focus = window;
    debug("Adding to taskbar");
    Taskbar* item = addItem(&taskbar, sizeof(Taskbar));
    item->window = window;
    debug("Allocated fullscreen window");
    return window;
}

void unallocateWindow(Window* window)
{
    debug("Removing window");
    Taskbar* item = (Taskbar*)&taskbar;
    while (iterateList(&item))
    {
        if (item->window == window)
        {
            debug("Removing from taskbar");
            removeItem(&taskbar, item);
            break;
        }
    }
    if (focus == window)
    {
        focus = NULL;
    }
    if (window->title)
    {
        debug("Unallocating title");
        unallocate(window->title);
    }
    debug("Unallocating icon");
    unallocate(window->icon);
    debug("Unallocating video buffer");
    unallocate(window->buffer);
    debug("Unallocating window");
    removeItem(&windows, window);
    debug("Removed window");
}

void keyPress(uint8_t scancode, BOOLEAN pressed)
{
    if (pressed && scancode == 91)
    {
        dragging = NULL;
        focus = NULL;
    }
    if (focus)
    {
        KeyEvent* event = addItem(&focus->events, sizeof(KeyEvent));
        event->header.id = 1;
        event->scancode = scancode;
        event->pressed = pressed;
    }
}

void mouseMove(int16_t x, int16_t y)
{
    if (focus && focus->mouseMode == 2 && (focus->fullscreen || (mouseX >= focus->x + 10 && mouseX < focus->x + focus->width + 10 && mouseY >= focus->y + 47 && mouseY < focus->y + 47 + focus->height)))
    {
        MouseEvent* event = addItem(&focus->events, sizeof(MouseEvent));
        event->header.id = 3;
        event->x = x;
        event->y = y;
    }
    else
    {
        if (!dragging)
        {
            mouseX += x;
            if (mouseX < 0)
            {
                mouseX = 0;
            }
            else if (mouseX > display.width - 1)
            {
                mouseX = display.width - 1;
            }
            mouseY -= y;
            if (mouseY < 0)
            {
                mouseY = 0;
            }
            else if (mouseY > display.height - 1)
            {
                mouseY = display.height - 1;
            }
        }
        else
        {
            dragging->x += x;
            if (dragging->x < 0)
            {
                dragging->x = 0;
            }
            else if (dragging->x > display.width - dragging->width - 20)
            {
                dragging->x = display.width - dragging->width - 20;
            }
            dragging->y -= y;
            if (dragging->y < 0)
            {
                dragging->y = 0;
            }
            else if (dragging->y > display.height - dragging->height - 89)
            {
                dragging->y = display.height - dragging->height - 89;
            }
            mouseX = dragging->x - draggingX;
            mouseY = dragging->y - draggingY;
        }
        if (focus && focus->mouseMode != 2)
        {
            if (focus->fullscreen)
            {
                MouseEvent* event = addItem(&focus->events, sizeof(MouseEvent));
                event->header.id = 3;
                event->x = mouseX;
                event->y = mouseY;
            }
            else if (mouseX >= focus->x + 10 && mouseX < focus->x + focus->width + 10 && mouseY >= focus->y + 47 && mouseY < focus->y + 47 + focus->height)
            {
                MouseEvent* event = addItem(&focus->events, sizeof(MouseEvent));
                event->header.id = 3;
                event->x = mouseX - focus->x - 10;
                event->y = mouseY - focus->y - 47;
            }
        }
    }
}

void mouseClick(BOOLEAN left, BOOLEAN pressed)
{
    if ((!focus || !focus->fullscreen) && mouseY >= display.height - 28 && mouseY < display.height - 4)
    {
        if (pressed)
        {
            if (mouseX >= 5 + 32 * programCount)
            {
                Taskbar* item = (Taskbar*)&taskbar;
                uint64_t i = programCount;
                while (iterateList(&item))
                {
                    uint64_t x = 5 + i * 32;
                    if (mouseX >= x && mouseX < x + 24)
                    {
                        if (left)
                        {
                            if (item->window->minimised)
                            {
                                item->window->minimised = FALSE;
                                focus = item->window;
                                moveItemEnd(&windows, item->window);
                            }
                            else
                            {
                                if (focus == item->window)
                                {
                                    focus = NULL;
                                    item->window->minimised = TRUE;
                                }
                                else
                                {
                                    focus = item->window;
                                    moveItemEnd(&windows, item->window);
                                }
                            }
                        }
                        else
                        {
                            debug("Sending quit event to window");
                            ((Event*)addItem(&item->window->events, sizeof(Event)))->id = 0;
                            debug("Sent quit event to window");
                        }
                        break;
                    }
                    i++;
                }
            }
            else if (left)
            {
                if (mouseX >= 4 && mouseX < 28)
                {
                    exiting = TRUE;
                    Window* window = (Window*)&windows;
                    while (iterateList(&window))
                    {
                        ((Event*)addItem(&window->events, sizeof(Event)))->id = 0;
                    }
                }
                else
                {
                    ProgramData* item = (ProgramData*)&programs;
                    uint64_t i = 1;
                    while (iterateList(&item))
                    {
                        uint64_t x = 4 + i * 32;
                        if (mouseX >= x && mouseX < x + 24)
                        {
                            execute(item->name);
                        }
                        i++;
                    }
                }
            }
        }
    }
    else
    {
        uint64_t length = listLength(&windows);
        Window** newWindows = allocate(length * 8);
        Window* window = (Window*)&windows;
        uint64_t reverse = length - 1;
        while (iterateList(&window))
        {
            newWindows[reverse--] = window;
        }
        for (uint64_t i = 0; i < length; i++)
        {
            if (!newWindows[i]->minimised)
            {
                if (newWindows[i]->fullscreen)
                {
                    if ((focus && focus->fullscreen) || mouseY < display.height - 32)
                    {
                        if (focus == newWindows[i])
                        {
                            ClickEvent* event = addItem(&newWindows[i]->events, sizeof(ClickEvent));
                            event->header.id = 2;
                            event->left = left;
                            event->pressed = pressed;
                        }
                        else
                        {
                            focus = newWindows[i];
                            moveItemEnd(&windows, newWindows[i]);
                        }
                        break;
                    }
                }
                else if (mouseX >= newWindows[i]->x + 10 && mouseX < newWindows[i]->x + newWindows[i]->width + 10 && mouseY >= newWindows[i]->y + 47 && mouseY < newWindows[i]->y + 47 + newWindows[i]->height)
                {
                    if (focus == newWindows[i])
                    {
                        ClickEvent* event = addItem(&newWindows[i]->events, sizeof(ClickEvent));
                        event->header.id = 2;
                        event->left = left;
                        event->pressed = pressed;
                    }
                    else if (left && pressed)
                    {
                        focus = newWindows[i];
                        moveItemEnd(&windows, newWindows[i]);
                    }
                    break;
                }
                else if (left && pressed)
                {
                    if (mouseX >= newWindows[i]->x + newWindows[i]->width - 22 && mouseX < newWindows[i]->x + newWindows[i]->width + 10 && mouseY >= newWindows[i]->y + 10 && mouseY < newWindows[i]->y + 42)
                    {
                        debug("Sending quit event to window");
                        ((Event*)addItem(&newWindows[i]->events, sizeof(Event)))->id = 0;
                        debug("Sent quit event to window");
                        break;
                    }
                    else if (mouseX >= newWindows[i]->x + newWindows[i]->width - 59 && mouseX < newWindows[i]->x + newWindows[i]->width - 27 && mouseY >= newWindows[i]->y + 10 && mouseY < newWindows[i]->y + 42)
                    {
                        if (focus == newWindows[i])
                        {
                            focus = NULL;
                        }
                        newWindows[i]->minimised = TRUE;
                        break;
                    }
                    else if (mouseX >= newWindows[i]->x && mouseX < newWindows[i]->x + newWindows[i]->width + 20 && mouseY >= newWindows[i]->y && mouseY < newWindows[i]->y + 47)
                    {
                        draggingX = newWindows[i]->x - mouseX;
                        draggingY = newWindows[i]->y - mouseY;
                        dragging = newWindows[i];
                        if (focus != newWindows[i])
                        {
                            focus = newWindows[i];
                            moveItemEnd(&windows, newWindows[i]);
                        }
                        break;
                    }
                }
            }
        }
        unallocate(newWindows);
    }
    if (left && !pressed)
    {
        dragging = NULL;
    }
}

uint64_t syscallHandle(uint64_t code, uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4)
{
    switch (code)
    {
        case 0:
            return (uint64_t)allocateWindow(arg1, arg2, (const CHAR16*)arg3, (const CHAR16*)arg4);
            break;
        case 1:
            return (uint64_t)allocateFullscreenWindow((const CHAR16*)arg1);
            break;
        case 2:
            unallocateWindow((Window*)arg1);
            break;
    }
    return 0;
}

void _start()
{
    uint64_t count = 0;
    debug("Loading programs");
    File** files = getFiles(L"/programs/desktop/taskbar/", &count, TRUE);
    ProgramData* current = NULL;
    for (uint64_t i = 0; i < count; i++)
    {
        if (StrCmp(files[i]->name + StrLen(files[i]->name) - 4, L".bin") == 0)
        {
            debug("Located binary");
            current = addItem(&programs, sizeof(ProgramData));
            current->name = files[i]->name;
            current->size = files[i]->size;
            current->data = files[i]->data;
            programCount++;
        }
        else if (current && StrCmp(files[i]->name + StrLen(files[i]->name) - 4, L".bmp") == 0)
        {
            debug("Located icon");
            readBitmap(files[i]->data, current->icon);
            current = NULL;
        }
    }
    unallocate(files);
    getDisplayInfo(&display);
    initGraphics(display.buffer, display.width, readFile(L"/fonts/font.psf", NULL));
    wallpaper = allocate(display.width * display.height * sizeof(EFI_GRAPHICS_OUTPUT_BLT_PIXEL));
    EFI_GRAPHICS_OUTPUT_BLT_PIXEL* buffer = wallpaper;
    uint8_t* wallpaperFile = readFile(L"/programs/desktop/wallpaper.bmp", NULL);
    uint8_t* fileBuffer = wallpaperFile + 0x36;
    int32_t width = *(int32_t*)(wallpaperFile + 0x12);
    int32_t height = *(int32_t*)(wallpaperFile + 0x16);
    debug("Fitting wallpaper to screen");
    for (uint32_t y = 0; y < display.height; y++)
    {
        for (uint32_t x = 0; x < display.width; x++)
        {
            uint32_t newX = width * (x / (double)display.width);
            uint32_t newY = height * (y / (double)display.height);
            uint64_t index = ((height - newY - 1) * width + newX) * 3;
            buffer->Blue = fileBuffer[index];
            buffer->Green = fileBuffer[index + 1];
            buffer->Red = fileBuffer[index + 2];
            buffer++;
        }
    }
    debug("Centering mouse to screen");
    mouseX = display.width / 2;
    mouseY = display.height / 2;
    debug("Adding calls");
    key = addKeyCall(keyPress);
    move = addMoveCall(mouseMove);
    click = addClickCall(mouseClick);
    syscall = registerSyscallHandler(L"desktop", syscallHandle);
    debug("Getting core count");
    cores = getCores();
    debug("Desktop started");
}

void update(uint64_t ticks)
{
    if (!focus || !focus->fullscreen)
    {
        blitTo = (uint64_t*)display.buffer;
        blitFrom = (uint64_t*)wallpaper;
        blitSize = (display.width * display.height / 2) / cores;
        splitTask(coreBlit, cores);
    }
    Window* window = (Window*)&windows;
    while (iterateList(&window))
    {
        if (!window->minimised)
        {
            if (window->fullscreen)
            {
                blitTo = (uint64_t*)display.buffer;
                blitFrom = (uint64_t*)window->buffer;
                blitSize = (display.width * display.height / 2) / cores;
                splitTask(coreBlit, cores);
            }
            else
            {
                drawRectangle(window->x, window->y, window->width + 20, window->height + 57, focus == window ? white : black);
                drawRectangle(window->x + 5, window->y + 5, window->width + 10, window->height + 47, grey);
                drawImage(window->x + 14, window->y + 14, 24, 24, window->icon);
                drawString(window->title, (window->x + 10 + window->width / 2) - (StrLen(window->title) * 8), window->y + 10, black);
                drawRectangle(window->x + window->width - 22, window->y + 10, 32, 32, red);
                drawCharacter(L'X', window->x + window->width - 14, window->y + 10, black);
                drawRectangle(window->x + window->width - 59, window->y + 10, 32, 32, black);
                drawCharacter(L'-', window->x + window->width - 51, window->y + 10, white);
                drawImage(window->x + 10, window->y + 47, window->width, window->height, window->buffer);
            }
        }
    }
    if (!focus || !focus->fullscreen)
    {
        drawRectangle(0, display.height - 32, display.width, 32, grey);
        EFI_GRAPHICS_OUTPUT_BLT_PIXEL* address = display.buffer + (display.height - 28) * display.width + 4;
        uint8_t* buffer = exitIcon;
        for (uint8_t y = 0; y < 24; y++)
        {
            for (uint8_t x = 0; x < 24; x++)
            {
                *address++ = *buffer++ ? red : white;
            }
            address += display.width - 24;
        }
        ProgramData* programData = (ProgramData*)&programs;
        uint64_t i = 1;
        while (iterateList(&programData))
        {
            drawImage(4 + i * 32, display.height - 28, 24, 24, programData->icon);
            i++;
        }
        drawRectangle(programCount * 32 - 1, display.height - 28, 2, 24, black);
        Taskbar* item = (Taskbar*)&taskbar;
        i = programCount;
        while (iterateList(&item))
        {
            if (focus == item->window)
            {
                drawRectangle(3 + i * 32, display.height - 30, 28, 28, black);
            }
            if (!item->window->minimised)
            {
                drawRectangle(5 + i * 32, display.height - 2, 24, 2, black);
            }
            drawImage(5 + i * 32, display.height - 28, 24, 24, item->window->icon);
            i++;
        }
        BOOLEAN half = FALSE;
        uint8_t hour = 0;
        uint8_t minute = 0;
        getTime(&hour, &minute);
        if (hour >= 12)
        {
            half = TRUE;
            if (hour != 12)
            {
                hour -= 12;
            }
        }
        else if (hour == 0)
        {
            hour = 12;
        }
        drawString(half ? L"PM" : L"AM", display.width - 37, display.height - 32, white);
        CHAR16 characters[3];
        ValueToString(characters, FALSE, minute);
        drawString(characters, display.width - ((minute < 10) ? 69 : 85), display.height - 32, white);
        if (minute < 10)
        {
            drawCharacter(L'0', display.width - 85, display.height - 32, white);
        }
        drawCharacter(L':', display.width - 101, display.height - 32, white);
        ValueToString(characters, FALSE, hour);
        drawString(characters, display.width - ((hour < 10) ? 117 : 133), display.height - 32, white);
        if (hour < 10)
        {
            drawCharacter(L'0', display.width - 133, display.height - 32, white);
        }
    }
    if (!focus || focus->mouseMode == 0 || (!focus->fullscreen && (mouseX < focus->x + 10 || mouseX >= focus->x + focus->width + 10 || mouseY < focus->y + 47 || mouseY >= focus->y + 47 + focus->height)))
    {
        EFI_GRAPHICS_OUTPUT_BLT_PIXEL* address = display.buffer + mouseY * display.width + mouseX;
        uint8_t* buffer = mouseIcon;
        for (uint32_t y = 0; y < 16; y++)
        {
            for (uint32_t x = 0; x < 16; x++)
            {
                if (*buffer != 2 && mouseX + x < display.width && mouseY + y < display.height)
                {
                    *address = *buffer ? white : black;
                }
                buffer++;
                address++;
            }
            address += display.width - 16;
        }
    }
    if (exiting && listLength(&windows) == 0)
    {
        unallocateList(&programs);
        unallocate(wallpaper);
        removeKeyCall(key);
        removeMoveCall(move);
        removeClickCall(click);
        unregisterSyscallHandler(syscall);
        quit();
    }
}
