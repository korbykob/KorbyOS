#include "main.h"

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
int64_t mouseX = 0;
int64_t mouseY = 0;
typedef struct
{
    void* next;
    EFI_GRAPHICS_OUTPUT_BLT_PIXEL icon[24*24];
    uint64_t size;
    uint8_t* data;
} ProgramData;
ProgramData* programs = NULL;
uint8_t programCount = 0;
typedef struct {
    void* next;
    uint64_t pid;
    void (*start)();
    void (*update)(uint64_t ticks);
} Program;
Program* running = NULL;
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
Program* currentProgram = 0;

void quit()
{
    serial("Exiting program\n");
    unallocate(currentProgram->start);
    removeItem((void**)&running, currentProgram);
    serial("\n");
}

Window* allocateWindow(uint32_t width, uint32_t height, const CHAR16* title, const CHAR16* icon)
{
    serial("Allocating window\n");
    Window* window = addItem((void**)&windows, sizeof(Window));
    window->x = GOP->Mode->Info->HorizontalResolution / 2 - (width + 20) / 2;
    window->y = GOP->Mode->Info->VerticalResolution / 2 - (height + 57) / 2;
    window->width = width;
    window->height = height;
    serial("Reallocating title\n");
    window->title = allocate((StrLen(title) + 1) * 2);
    StrCpy(window->title, title);
    serial("Allocating icon\n");
    window->icon = allocate(24 * 24 * sizeof(EFI_GRAPHICS_OUTPUT_BLT_PIXEL));
    readBitmap(readFile(icon, NULL), window->icon);
    serial("Allocating video buffer\n");
    window->buffer = allocate(width * height * sizeof(EFI_GRAPHICS_OUTPUT_BLT_PIXEL));
    window->mouseMode = 0;
    window->fullscreen = FALSE;
    window->minimised = FALSE;
    window->events = NULL;
    focus = window;
    serial("Adding to taskbar\n");
    Taskbar* item = addItem((void**)&taskbar, sizeof(Taskbar));
    item->window = window;
    serial("\n");
    return window;
}

Window* allocateFullscreenWindow(const CHAR16* icon)
{
    serial("Allocating fullscreen window\n");
    Window* window = addItem((void**)&windows, sizeof(Window));
    window->x = 0;
    window->y = 0;
    window->width = GOP->Mode->Info->HorizontalResolution;
    window->height = GOP->Mode->Info->VerticalResolution;
    window->title = NULL;
    serial("Allocating icon\n");
    window->icon = allocate(24 * 24 * sizeof(EFI_GRAPHICS_OUTPUT_BLT_PIXEL));
    readBitmap(readFile(icon, NULL), window->icon);
    serial("Allocating video buffer\n");
    window->buffer = allocate(window->width * window->height * sizeof(EFI_GRAPHICS_OUTPUT_BLT_PIXEL));
    window->mouseMode = 0;
    window->fullscreen = TRUE;
    window->minimised = FALSE;
    window->events = NULL;
    focus = window;
    serial("Adding to taskbar\n");
    Taskbar* item = addItem((void**)&taskbar, sizeof(Taskbar));
    item->window = window;
    serial("\n");
    return window;
}

void unallocateWindow(Window* window)
{
    serial("Removing window\n");
    Taskbar* item = (Taskbar*)&taskbar;
    while (iterateList((void**)&item))
    {
        if (item->window == window)
        {
            serial("Removing from taskbar\n");
            removeItem((void**)&taskbar, item);
            break;
        }
    }
    if (focus == window)
    {
        focus = NULL;
    }
    if (window->title)
    {
        serial("Unallocating title\n");
        unallocate(window->title);
    }
    serial("Unallocating icon\n");
    unallocate(window->icon);
    serial("Unallocating video buffer\n");
    unallocate(window->buffer);
    serial("Unallocating window\n");
    removeItem((void**)&windows, window);
    serial("\n");
}

uint64_t syscallHandle(uint64_t code, uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4)
{
    switch (code)
    {
        case 0:
            quit();
            break;
        case 1:
            return (uint64_t)allocate(arg1);
            break;
        case 2:
            unallocate((void*)arg1);
            return 0;
            break;
        case 3:
            return (uint64_t)allocateWindow(arg1, arg2, (const CHAR16*)arg3, (const CHAR16*)arg4);
            break;
        case 4:
            return (uint64_t)allocateFullscreenWindow((const CHAR16*)arg1);
            break;
        case 5:
            unallocateWindow((Window*)arg1);
            return 0;
            break;
        case 6:
            return (uint64_t)writeFile((const CHAR16*)arg1, arg2);
            break;
        case 7:
            return (uint64_t)readFile((const CHAR16*)arg1, (uint64_t*)arg2);
            break;
        case 8:
            deleteFile((const CHAR16*)arg1);
            return 0;
            break;
        case 9:
            return cpuCount + 1;
            break;
    }
    return 0;
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
        KeyEvent* event = addItem((void**)&focus->events, sizeof(KeyEvent));
        event->header.id = 1;
        event->scancode = scancode;
        event->pressed = pressed;
    }
}

void mouseMove(int16_t x, int16_t y)
{
    if (focus && focus->mouseMode == 2 && (focus->fullscreen || (mouseX >= focus->x + 10 && mouseX < focus->x + focus->width + 10 && mouseY >= focus->y + 47 && mouseY < focus->y + 47 + focus->height)))
    {
        MouseEvent* event = addItem((void**)&focus->events, sizeof(MouseEvent));
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
            else if (mouseX > GOP->Mode->Info->HorizontalResolution - 1)
            {
                mouseX = GOP->Mode->Info->HorizontalResolution - 1;
            }
            mouseY -= y;
            if (mouseY < 0)
            {
                mouseY = 0;
            }
            else if (mouseY > GOP->Mode->Info->VerticalResolution - 1)
            {
                mouseY = GOP->Mode->Info->VerticalResolution - 1;
            }
        }
        else
        {
            dragging->x += x;
            if (dragging->x < 0)
            {
                dragging->x = 0;
            }
            else if (dragging->x > GOP->Mode->Info->HorizontalResolution - dragging->width - 20)
            {
                dragging->x = GOP->Mode->Info->HorizontalResolution - dragging->width - 20;
            }
            dragging->y -= y;
            if (dragging->y < 0)
            {
                dragging->y = 0;
            }
            else if (dragging->y > GOP->Mode->Info->VerticalResolution - dragging->height - 89)
            {
                dragging->y = GOP->Mode->Info->VerticalResolution - dragging->height - 89;
            }
            mouseX = dragging->x - draggingX;
            mouseY = dragging->y - draggingY;
        }
        if (focus && focus->mouseMode != 2)
        {
            if (focus->fullscreen)
            {
                MouseEvent* event = addItem((void**)&focus->events, sizeof(MouseEvent));
                event->header.id = 3;
                event->x = mouseX;
                event->y = mouseY;
            }
            else if (mouseX >= focus->x + 10 && mouseX < focus->x + focus->width + 10 && mouseY >= focus->y + 47 && mouseY < focus->y + 47 + focus->height)
            {
                MouseEvent* event = addItem((void**)&focus->events, sizeof(MouseEvent));
                event->header.id = 3;
                event->x = mouseX - focus->x - 10;
                event->y = mouseY - focus->y - 47;
            }
        }
    }
}

void mouseClick(BOOLEAN left, BOOLEAN pressed)
{
    if ((!focus || !focus->fullscreen) && mouseY >= GOP->Mode->Info->VerticalResolution - 28 && mouseY < GOP->Mode->Info->VerticalResolution - 4)
    {
        if (pressed)
        {
            if (mouseX >= 5 + 32 * programCount)
            {
                Taskbar* item = (Taskbar*)&taskbar;
                uint64_t i = programCount;
                while (iterateList((void**)&item))
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
                                moveItemEnd((void**)&windows, item->window);
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
                                    moveItemEnd((void**)&windows, item->window);
                                }
                            }
                        }
                        else
                        {
                            Event* event = addItem((void**)&item->window->events, sizeof(Event));
                            event->id = 0;
                        }
                        break;
                    }
                    i++;
                }
            }
            else if (left)
            {
                ProgramData* item = (ProgramData*)&programs;
                uint64_t i = 0;
                while (iterateList((void**)&item))
                {
                    uint64_t x = 4 + i * 32;
                    if (mouseX >= x && mouseX < x + 24)
                    {
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
                        serial("Allocating program\n");
                        Program* program = addItem((void**)&running, sizeof(Program));
                        program->pid = pid;
                        serial("Allocating binary\n");
                        program->start = allocate(item->size);
                        copyMemory(item->data, (uint8_t*)program->start, item->size);
                        program->update = (void*)program->start + 5;
                        Program* oldProgram = currentProgram;
                        currentProgram = program;
                        serial("Starting program\n\n");
                        program->start();
                        currentProgram = oldProgram;
                        serial("Program started\n\n");
                    }
                    i++;
                }
            }
        }
    }
    else
    {
        uint64_t length = listLength((void**)&windows);
        Window** newWindows = allocate(length * 8);
        Window* window = (Window*)&windows;
        uint64_t reverse = length - 1;
        while (iterateList((void**)&window))
        {
            newWindows[reverse--] = window;
        }
        for (uint64_t i = 0; i < length; i++)
        {
            if (!newWindows[i]->minimised)
            {
                if (newWindows[i]->fullscreen)
                {
                    if ((focus && focus->fullscreen) || mouseY < GOP->Mode->Info->VerticalResolution - 32)
                    {
                        if (focus == newWindows[i])
                        {
                            ClickEvent* event = addItem((void**)&newWindows[i]->events, sizeof(ClickEvent));
                            event->header.id = 2;
                            event->left = left;
                            event->pressed = pressed;
                        }
                        else
                        {
                            focus = newWindows[i];
                            moveItemEnd((void**)&windows, newWindows[i]);
                        }
                        break;
                    }
                }
                else if (mouseX >= newWindows[i]->x + 10 && mouseX < newWindows[i]->x + newWindows[i]->width + 10 && mouseY >= newWindows[i]->y + 47 && mouseY < newWindows[i]->y + 47 + newWindows[i]->height)
                {
                    if (focus == newWindows[i])
                    {
                        ClickEvent* event = addItem((void**)&newWindows[i]->events, sizeof(ClickEvent));
                        event->header.id = 2;
                        event->left = left;
                        event->pressed = pressed;
                    }
                    else if (left && pressed)
                    {
                        focus = newWindows[i];
                        moveItemEnd((void**)&windows, newWindows[i]);
                    }
                    break;
                }
                else if (left && pressed)
                {
                    if (mouseX >= newWindows[i]->x + newWindows[i]->width - 22 && mouseX < newWindows[i]->x + newWindows[i]->width + 10 && mouseY >= newWindows[i]->y + 10 && mouseY < newWindows[i]->y + 42)
                    {
                        serial("Sending quit event to window\n");
                        Event* event = addItem((void**)&newWindows[i]->events, sizeof(Event));
                        event->id = 0;
                        serial("\n");
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
                            moveItemEnd((void**)&windows, newWindows[i]);
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

void start()
{
    File* file = (File*)&files;
    ProgramData* current = NULL;
    serial("Loading programs\n");
    while (iterateList((void**)&file))
    {
        if (StrnCmp(file->name, L"programs/", 9) == 0)
        {
            if (StrCmp(file->name + StrLen(file->name) - 4, L".bin") == 0)
            {
                serial("Located binary\n");
                current = addItem((void**)&programs, sizeof(ProgramData));
                current->size = file->size;
                current->data = file->data;
                programCount++;
            }
            else if (current && StrCmp(file->name + StrLen(file->name) - 4, L".bmp") == 0)
            {
                serial("Located icon\n");
                readBitmap(file->data, current->icon);
                current = NULL;
            }
        }
    }
    serial("Allocating video buffer\n");
    EFI_GRAPHICS_OUTPUT_BLT_PIXEL* videoBuffer = allocate(GOP->Mode->FrameBufferSize);
    initGraphics(videoBuffer, GOP->Mode->Info->HorizontalResolution, readFile(L"fonts/font.psf", NULL));
    uint64_t ticks = 0;
    mouseX = GOP->Mode->Info->HorizontalResolution / 2;
    mouseY = GOP->Mode->Info->VerticalResolution / 2;
    serial("Starting loop\n\n");
    while (TRUE)
    {
        if (!focus || !focus->fullscreen)
        {
            blit(wallpaper, videoBuffer, GOP->Mode->Info->HorizontalResolution, GOP->Mode->Info->VerticalResolution);
        }
        Program* program = (Program*)&running;
        while (iterateList((void**)&program))
        {
            currentProgram = program;
            program->update(ticks);
        }
        Window* window = (Window*)&windows;
        while (iterateList((void**)&window))
        {
            if (!window->minimised)
            {
                if (window->fullscreen)
                {
                    blit(window->buffer, videoBuffer, GOP->Mode->Info->HorizontalResolution, GOP->Mode->Info->VerticalResolution);
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
            drawRectangle(0, GOP->Mode->Info->VerticalResolution - 32, GOP->Mode->Info->HorizontalResolution, 32, grey);
            ProgramData* programData = (ProgramData*)&programs;
            uint64_t i = 0;
            while (iterateList((void**)&programData))
            {
                drawImage(4 + i * 32, GOP->Mode->Info->VerticalResolution - 28, 24, 24, programData->icon);
            }
            drawRectangle(programCount * 32 - 1, GOP->Mode->Info->VerticalResolution - 28, 2, 24, black);
            Taskbar* item = (Taskbar*)&taskbar;
            i = programCount;
            while (iterateList((void**)&item))
            {
                if (focus == item->window)
                {
                    drawRectangle(3 + i * 32, GOP->Mode->Info->VerticalResolution - 30, 28, 28, black);
                }
                if (!item->window->minimised)
                {
                    drawRectangle(5 + i * 32, GOP->Mode->Info->VerticalResolution - 2, 24, 2, black);
                }
                drawImage(5 + i * 32, GOP->Mode->Info->VerticalResolution - 28, 24, 24, item->window->icon);
                i++;
            }
            BOOLEAN half = FALSE;
            uint8_t hour = 0;
            uint8_t minute = 0;
            getTime(&hour, &minute);
            if (hour >= 12)
            {
                half = TRUE;
                hour -= 12;
            }
            drawString(half ? L"PM" : L"AM", GOP->Mode->Info->HorizontalResolution - 37, GOP->Mode->Info->VerticalResolution - 32, white);
            CHAR16 characters[3];
            ValueToString(characters, FALSE, minute);
            drawString(characters, GOP->Mode->Info->HorizontalResolution - ((minute < 10) ? 69 : 85), GOP->Mode->Info->VerticalResolution - 32, white);
            if (minute < 10)
            {
                drawCharacter(L'0', GOP->Mode->Info->HorizontalResolution - 85, GOP->Mode->Info->VerticalResolution - 32, white);
            }
            drawCharacter(L':', GOP->Mode->Info->HorizontalResolution - 101, GOP->Mode->Info->VerticalResolution - 32, white);
            ValueToString(characters, FALSE, hour);
            drawString(characters, GOP->Mode->Info->HorizontalResolution - ((hour < 10) ? 117 : 133), GOP->Mode->Info->VerticalResolution - 32, white);
            if (hour < 10)
            {
                drawCharacter(L'0', GOP->Mode->Info->HorizontalResolution - 133, GOP->Mode->Info->VerticalResolution - 32, white);
            }
        }
        if (!focus || focus->mouseMode == 0 || (!focus->fullscreen && (mouseX < focus->x + 10 || mouseX >= focus->x + focus->width + 10 || mouseY < focus->y + 47 || mouseY >= focus->y + 47 + focus->height)))
        {
            EFI_GRAPHICS_OUTPUT_BLT_PIXEL* address = videoBuffer + mouseY * GOP->Mode->Info->HorizontalResolution + mouseX;
            uint8_t* buffer = mouseIcon;
            for (uint32_t y = 0; y < 16; y++)
            {
                for (uint32_t x = 0; x < 16; x++)
                {
                    if (*buffer != 2 && mouseX + x < GOP->Mode->Info->HorizontalResolution && mouseY + y < GOP->Mode->Info->VerticalResolution)
                    {
                        *address = *buffer ? white : black;
                    }
                    buffer++;
                    address++;
                }
                address += GOP->Mode->Info->HorizontalResolution - 16;
            }
        }
        ticks = hpetCounter;
        hpetCounter = 0;
        blit(videoBuffer, (EFI_GRAPHICS_OUTPUT_BLT_PIXEL*)GOP->Mode->FrameBufferBase, GOP->Mode->Info->HorizontalResolution, GOP->Mode->Info->VerticalResolution);
    }
}
