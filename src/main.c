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
struct Program
{
    struct Program* next;
    uint64_t size;
    uint64_t id;
    void (*start)(uint64_t id);
    void (*update)();
};
struct Program* running = NULL;
struct Window* windows = NULL;
struct Window* dragging = NULL;
struct Window* focus = NULL;

void quit(uint64_t id)
{
    struct Program* program = (struct Program*)&running;
    while (iterateList((void**)&program))
    {
        if (program->id == id)
        {
            unallocate(program->start, program->size);
            removeItem((void**)&running, program, sizeof(struct Program));
            break;
        }
    }
}

struct Window* allocateWindow(uint32_t width, uint32_t height, CHAR16* title)
{
    struct Window* window = addItem((void**)&windows, sizeof(struct Window));
    window->x = GOP->Mode->Info->HorizontalResolution / 2 - (width + 20) / 2;
    window->y = GOP->Mode->Info->VerticalResolution / 2 - (height + 57) / 2;
    window->width = width;
    window->height = height;
    window->title = title;
    window->buffer = allocate(width * height * 4);
    window->hideMouse = FALSE;
    window->fullscreen = FALSE;
    window->events = NULL;
    focus = window;
    return window;
}

struct Window* allocateFullscreenWindow()
{
    struct Window* window = addItem((void**)&windows, sizeof(struct Window));
    window->x = 0;
    window->y = 0;
    window->width = GOP->Mode->Info->HorizontalResolution;
    window->height = GOP->Mode->Info->VerticalResolution;
    window->title = NULL;
    window->buffer = allocate(window->width * window->height * 4);
    window->hideMouse = FALSE;
    window->fullscreen = TRUE;
    window->events = NULL;
    focus = window;
    return window;
}

void unallocateWindow(struct Window* window)
{
    if (focus == window)
    {
        focus = NULL;
    }
    unallocate(window->buffer, window->width * window->height * 4);
    removeItem((void**)&windows, window, sizeof(struct Window));
}

uint64_t syscallHandle(uint64_t code, uint64_t arg1, uint64_t arg2, uint64_t arg3)
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
            return (uint64_t)allocateWindow(arg1, arg2, (CHAR16*)arg3);
            break;
        case 4:
            return (uint64_t)allocateFullscreenWindow();
            break;
        case 5:
            unallocateWindow((struct Window*)arg1);
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
        struct KeyEvent* event = addItem((void**)&focus->events, sizeof(struct KeyEvent));
        event->id = 1;
        event->size = sizeof(struct KeyEvent);
        event->scancode = scancode;
        event->pressed = pressed;
    }
}

void mouseMove(int8_t x, int8_t y)
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
    if (left)
    {
        if (pressed)
        {
            struct Window* window = (struct Window*)&windows;
            while (iterateList((void**)&window))
            {
                if (window->fullscreen)
                {
                    if (focus || mouseY < GOP->Mode->Info->VerticalResolution - 32)
                    {
                        if (focus)
                        {
                            struct ClickEvent* event = addItem((void**)&window->events, sizeof(struct ClickEvent));
                            event->id = 2;
                            event->size = sizeof(struct ClickEvent);
                            event->left = left;
                            event->pressed = pressed;
                        }
                        focus = window;
                        break;
                    }
                }
                else if (mouseX >= window->x + window->width - 22 && mouseX < window->x + window->width + 10 && mouseY >= window->y + 10 && mouseY < window->y + 42)
                {
                    struct Event* event = addItem((void**)&window->events, sizeof(struct Event));
                    event->id = 0;
                    event->size = sizeof(struct Event);
                    break;
                }
                else if (mouseX >= window->x && mouseX < window->x + window->width + 20 && mouseY >= window->y && mouseY < window->y + 47)
                {
                    dragging = window;
                    focus = window;
                    break;
                }
                else if (mouseX >= window->x + 10 && mouseX < window->x + window->width + 10 && mouseY >= window->y + 47 && mouseY < window->y + 47 + window->height)
                {
                    focus = window;
                    struct ClickEvent* event = addItem((void**)&window->events, sizeof(struct ClickEvent));
                    event->id = 2;
                    event->size = sizeof(struct ClickEvent);
                    event->left = left;
                    event->pressed = pressed;
                    break;
                }
            }
            if (!focus || !focus->fullscreen)
            {
                for (uint8_t i = 0; i < 1; i++)
                {
                    uint64_t x = 4 + i * 24 + i * 8;
                    if (mouseX >= x && mouseX < x + 24 && mouseY >= GOP->Mode->Info->VerticalResolution - 28 && mouseY < GOP->Mode->Info->VerticalResolution - 4)
                    {
                        uint64_t id = 0;
                        struct Program* iterator = (struct Program*)&running;
                        while (iterateList((void**)&iterator))
                        {
                            if (id == iterator->id)
                            {
                                id++;
                                iterator = (struct Program*)&running;
                            }
                        }
                        struct Program* program = addItem((void**)&running, sizeof(struct Program));
                        program->size = programs[i].size;
                        program->id = id;
                        program->start = allocate(programs[i].size);
                        uint8_t* source = programs[i].binary;
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
        else
        {
            dragging = NULL;
        }
    }
}

void start()
{
    mouseX = GOP->Mode->Info->HorizontalResolution / 2;
    mouseY = GOP->Mode->Info->VerticalResolution / 2;
    while (TRUE)
    {
        if (!focus || !focus->fullscreen)
        {
            blit(wallpaper, videoBuffer);
        }
        struct Program* program = (struct Program*)&running;
        while (iterateList((void**)&program))
        {
            program->update();
        }
        struct Window* window = (struct Window*)&windows;
        while (iterateList((void**)&window))
        {
            if (window->fullscreen)
            {
                blit(window->buffer, videoBuffer);
            }
            else
            {
                drawRectangle(window->x, window->y, window->width + 20, window->height + 57, focus == window ? white : black);
                drawRectangle(window->x + 5, window->y + 5, window->width + 10, window->height + 47, grey);
                drawString(window->title, window->x + 10, window->y + 10, black);
                drawRectangle(window->x + window->width - 22, window->y + 10, 32, 32, red);
                drawImage(window->x + 10, window->y + 47, window->width, window->height, window->buffer);
            }
        }
        if (!focus || !focus->fullscreen)
        {
            drawRectangle(0, GOP->Mode->Info->VerticalResolution - 32, GOP->Mode->Info->HorizontalResolution, 32, grey);
            for (uint8_t i = 0; i < 1; i++)
            {
                uint64_t x = 4 + i * 24 + i * 8;
                drawImage(x, GOP->Mode->Info->VerticalResolution - 28, 24, 24, programs[i].icon);
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
