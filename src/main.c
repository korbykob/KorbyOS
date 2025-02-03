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
EFI_GRAPHICS_OUTPUT_BLT_PIXEL white = { 255, 255, 255 };
EFI_GRAPHICS_OUTPUT_BLT_PIXEL black = { 0, 0, 0 };
EFI_GRAPHICS_OUTPUT_BLT_PIXEL grey = { 128, 128, 128 };
EFI_GRAPHICS_OUTPUT_BLT_PIXEL red = { 0, 0, 255 };
int64_t mouseX = 0;
int64_t mouseY = 0;
struct
{
    EFI_GRAPHICS_OUTPUT_BLT_PIXEL icon[24*24];
    uint64_t size;
    uint8_t* data;
} programs[1];
typedef struct {
    void* next;
    uint64_t size;
    uint64_t id;
    void (*start)(uint64_t id);
    void (*update)();
} Program;
Program* running = NULL;
Window* windows = NULL;
Window* dragging = NULL;
Window* focus = NULL;
typedef struct
{
    void* next;
    Window* window;
} Taskbar;
Taskbar* taskbar = NULL;

void quit(uint64_t id)
{
    Program* program = (Program*)&running;
    while (iterateList((void**)&program))
    {
        if (program->id == id)
        {
            unallocate(program->start, program->size);
            removeItem((void**)&running, program, sizeof(Program));
            break;
        }
    }
}

Window* allocateWindow(uint32_t width, uint32_t height, const CHAR16* title, const CHAR16* icon)
{
    Window* window = addItem((void**)&windows, sizeof(Window));
    window->x = GOP->Mode->Info->HorizontalResolution / 2 - (width + 20) / 2;
    window->y = GOP->Mode->Info->VerticalResolution / 2 - (height + 57) / 2;
    window->width = width;
    window->height = height;
    window->title = allocate((StrLen(title) + 1) * 2);
    StrCpy(window->title, title);
    window->icon = allocate(24 * 24 * sizeof(EFI_GRAPHICS_OUTPUT_BLT_PIXEL));
    readBitmap(readFile(icon, NULL), window->icon);
    window->buffer = allocate(width * height * sizeof(EFI_GRAPHICS_OUTPUT_BLT_PIXEL));
    window->hideMouse = FALSE;
    window->fullscreen = FALSE;
    window->minimised = FALSE;
    window->events = NULL;
    focus = window;
    Taskbar* item = addItem((void**)&taskbar, sizeof(Taskbar));
    item->window = window;
    return window;
}

Window* allocateFullscreenWindow(const CHAR16* icon)
{
    Window* window = addItem((void**)&windows, sizeof(Window));
    window->x = 0;
    window->y = 0;
    window->width = GOP->Mode->Info->HorizontalResolution;
    window->height = GOP->Mode->Info->VerticalResolution;
    window->title = NULL;
    window->icon = allocate(24 * 24 * sizeof(EFI_GRAPHICS_OUTPUT_BLT_PIXEL));
    readBitmap(readFile(icon, NULL), window->icon);
    window->buffer = allocate(window->width * window->height * sizeof(EFI_GRAPHICS_OUTPUT_BLT_PIXEL));
    window->hideMouse = FALSE;
    window->fullscreen = TRUE;
    window->minimised = FALSE;
    window->events = NULL;
    focus = window;
    Taskbar* item = addItem((void**)&taskbar, sizeof(Taskbar));
    item->window = window;
    return window;
}

void unallocateWindow(Window* window)
{
    Taskbar* item = (Taskbar*)&taskbar;
    while (iterateList((void**)&item))
    {
        if (item->window == window)
        {
            removeItem((void**)&taskbar, item, sizeof(Taskbar));
            break;
        }
    }
    if (focus == window)
    {
        focus = NULL;
    }
    if (window->title)
    {
        unallocate(window->title, (StrLen(window->title) + 1) * 2);
    }
    unallocate(window->icon, 24 * 24 * sizeof(EFI_GRAPHICS_OUTPUT_BLT_PIXEL));
    unallocate(window->buffer, window->width * window->height * sizeof(EFI_GRAPHICS_OUTPUT_BLT_PIXEL));
    removeItem((void**)&windows, window, sizeof(Window));
}

uint64_t syscallHandle(uint64_t code, uint64_t arg1, uint64_t arg2, uint64_t arg3, uint64_t arg4)
{
    switch (code)
    {
        case 0:
            quit(arg1);
            break;
        case 1:
            return (uint64_t)allocate(arg1);
            break;
        case 2:
            unallocate((void*)arg1, arg2);
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
            return (uint64_t)createFile((const CHAR16*)arg1, arg2);
            break;
        case 7:
            return (uint64_t)readFile((const CHAR16*)arg1, (uint64_t*)arg2);
            break;
        case 8:
            deleteFile((const CHAR16*)arg1);
            return 0;
            break;
    }
    return 0;
}

void keyPress(uint8_t scancode, BOOLEAN pressed)
{
    if (pressed && scancode == 91)
    {
        focus = NULL;
    }
    if (focus)
    {
        KeyEvent* event = addItem((void**)&focus->events, sizeof(KeyEvent));
        event->id = 1;
        event->size = sizeof(KeyEvent);
        event->scancode = scancode;
        event->pressed = pressed;
    }
}

void mouseMove(int16_t x, int16_t y)
{
    mouseX += x;
    if (mouseX < 0)
    {
        mouseX = 0;
    }
    if (mouseX > GOP->Mode->Info->HorizontalResolution - 16)
    {
        mouseX = GOP->Mode->Info->HorizontalResolution - 16;
    }
    mouseY -= y;
    if (mouseY < 0)
    {
        mouseY = 0;
    }
    if (mouseY > GOP->Mode->Info->VerticalResolution - 16)
    {
        mouseY = GOP->Mode->Info->VerticalResolution - 16;
    }
    if (dragging)
    {
        dragging->x += x;
        if (dragging->x < 0)
        {
            dragging->x = 0;
        }
        if (dragging->x > GOP->Mode->Info->HorizontalResolution - dragging->width - 20)
        {
            dragging->x = GOP->Mode->Info->HorizontalResolution - dragging->width - 20;
        }
        dragging->y -= y;
        if (dragging->y < 0)
        {
            dragging->y = 0;
        }
        if (dragging->y > GOP->Mode->Info->VerticalResolution - dragging->height - 89)
        {
            dragging->y = GOP->Mode->Info->VerticalResolution - dragging->height - 89;
        }
    }
}

void mouseClick(BOOLEAN left, BOOLEAN pressed)
{
    if ((!focus || !focus->fullscreen) && mouseY >= GOP->Mode->Info->VerticalResolution - 28 && mouseY < GOP->Mode->Info->VerticalResolution - 4)
    {
        if (left && pressed)
        {
            if (mouseX >= 32)
            {
                Taskbar* item = (Taskbar*)&taskbar;
                uint64_t i = 1;
                while (iterateList((void**)&item))
                {
                    uint64_t x = 5 + i * 32;
                    if (mouseX >= x && mouseX < x + 24)
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
                        break;
                    }
                    i++;
                }
            }
            else
            {
                for (uint8_t i = 0; i < 1; i++)
                {
                    uint64_t x = 4 + i * 32;
                    if (mouseX >= x && mouseX < x + 24)
                    {
                        uint64_t id = 0;
                        Program* iterator = (Program*)&running;
                        while (iterateList((void**)&iterator))
                        {
                            if (id == iterator->id)
                            {
                                id++;
                                iterator = (Program*)&running;
                            }
                        }
                        Program* program = addItem((void**)&running, sizeof(Program));
                        program->size = programs[i].size;
                        program->id = id;
                        program->start = allocate(programs[i].size);
                        uint8_t* source = programs[i].data;
                        uint8_t* destination = (uint8_t*)program->start;
                        for (uint64_t i2 = 0; i2 < programs[i].size; i2++)
                        {
                            *destination++ = *source++;
                        }
                        program->update = program->start + 5;
                        program->start(id);
                    }
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
                            event->id = 2;
                            event->size = sizeof(ClickEvent);
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
                        event->id = 2;
                        event->size = sizeof(ClickEvent);
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
                        Event* event = addItem((void**)&newWindows[i]->events, sizeof(Event));
                        event->id = 0;
                        event->size = sizeof(Event);
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
        unallocate(newWindows, length * 8);
    }
    if (left && !pressed)
    {
        dragging = NULL;
    }
}

void start()
{
    uint8_t program = 0;
    File* file = (File*)&files;
    while (iterateList((void**)&file))
    {
        if (StrnCmp(file->name, L"programs/", 9) == 0)
        {
            if (StrCmp(file->name + StrLen(file->name) - 3, L"bmp") == 0)
            {
                readBitmap(file->data, programs[program].icon);
            }
            else if (StrCmp(file->name + StrLen(file->name) - 3, L"bin") == 0)
            {
                programs[program].size = file->size;
                programs[program].data = file->data;
                program++;
            }
        }
    }
    mouseX = GOP->Mode->Info->HorizontalResolution / 2;
    mouseY = GOP->Mode->Info->VerticalResolution / 2;
    while (TRUE)
    {
        if (!focus || !focus->fullscreen)
        {
            blit(wallpaper, videoBuffer);
        }
        Program* program = (Program*)&running;
        while (iterateList((void**)&program))
        {
            program->update();
        }
        Window* window = (Window*)&windows;
        while (iterateList((void**)&window))
        {
            if (!window->minimised)
            {
                if (window->fullscreen)
                {
                    blit(window->buffer, videoBuffer);
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
            for (uint8_t i = 0; i < 1; i++)
            {
                drawImage(4 + i * 32, GOP->Mode->Info->VerticalResolution - 28, 24, 24, programs[i].icon);
            }
            drawRectangle(1 * 32 - 1, GOP->Mode->Info->VerticalResolution - 28, 2, 24, black);
            Taskbar* item = (Taskbar*)&taskbar;
            uint64_t i = 1;
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
        }
        if (!focus || !focus->hideMouse || (!focus->fullscreen && (mouseX < focus->x + 10 || mouseX >= focus->x + focus->width + 10 || mouseY < focus->y + 47 || mouseY >= focus->y + 47 + focus->height)))
        {
            EFI_GRAPHICS_OUTPUT_BLT_PIXEL* address = videoBuffer + mouseY * GOP->Mode->Info->HorizontalResolution + mouseX;
            uint8_t* buffer = mouseIcon;
            for (uint32_t y = 0; y < 16; y++)
            {
                for (uint32_t x = 0; x < 16; x++)
                {
                    if (*buffer != 2)
                    {
                        *address = *buffer ? white : black;
                    }
                    buffer++;
                    address++;
                }
                address += GOP->Mode->Info->HorizontalResolution - 16;
            }
        }
        waitForPit();
        blit(videoBuffer, (EFI_GRAPHICS_OUTPUT_BLT_PIXEL*)GOP->Mode->FrameBufferBase);
    }
}
