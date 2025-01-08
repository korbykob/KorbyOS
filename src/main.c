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
int64_t mouseX = 0;
int64_t mouseY = 0;
struct Button
{
    uint32_t x;
    uint32_t y;
    uint32_t width;
    uint32_t height;
    uint64_t id;
};
struct Button buttons[100];
uint8_t buttonCount = 0;
BOOLEAN mainButtonActivated = FALSE;
struct Window windows;

struct Window* allocateWindow(uint32_t width, uint32_t height, CHAR16* title)
{
    struct Window* window = &windows;
    while (window->next)
    {
        window = window->next;
    }
    window->next = allocate(sizeof(struct Window));
    window->next->x = 0;
    window->next->y = 0;
    window->next->width = width;
    window->next->height = height;
    window->next->title = title;
    window->next->buffer = allocate(width * height * 4);
    window->next->next = NULL;
    return window->next;
}

void unallocateWindow(struct Window* window)
{
    struct Window* current = &windows;
    while (current->next != window)
    {
        current = current->next;
    }
    unallocate(current->next->buffer, current->next->width * current->next->height * 4);
    unallocate(current->next, sizeof(struct Window));
    current->next = current->next->next;
}

uint64_t syscallHandle(uint64_t code, uint64_t arg1, uint64_t arg2, uint64_t arg3)
{
    switch (code)
    {
        case 0:
            return (uint64_t)allocate(arg1);
            break;
        case 1:
            unallocate((void*)arg1, arg2);
            return 0;
            break;
        case 2:
            return (uint64_t)allocateWindow(arg1, arg2, (CHAR16*)arg3);
            break;
        case 3:
            unallocateWindow((struct Window*)arg1);
            return 0;
            break;
    }
    return 0;
}

void keyPress(uint8_t scancode, BOOLEAN unpressed)
{
    if (!unpressed && scancode == 91)
    {
        mainButtonActivated = !mainButtonActivated;
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
    struct Window* window = &windows;
    while (TRUE)
    {
        window = window->next;
        if (window == NULL)
        {
            break;
        }
        if (window->dragging)
        {
            window->x += x;
            if (window->x < 0)
            {
                window->x = 0;
            }
            if (window->x > GOP->Mode->Info->HorizontalResolution - window->width - 10)
            {
                window->x = GOP->Mode->Info->HorizontalResolution - window->width - 10;
            }
            window->y -= y;
            if (window->y < 0)
            {
                window->y = 0;
            }
            if (window->y > GOP->Mode->Info->VerticalResolution - window->height - 79)
            {
                window->y = GOP->Mode->Info->VerticalResolution - window->height - 79;
            }
        }
    }
}

void mouseClick(BOOLEAN left, BOOLEAN unpressed)
{
    if (left)
    {
        if (!unpressed)
        {
            for (uint8_t i = 0; i < buttonCount; i++)
            {
                if (mouseX >= buttons[i].x && mouseX < buttons[i].x + buttons[i].width && mouseY >= buttons[i].y && mouseY < buttons[i].y + buttons[i].height)
                {
                    switch (buttons[i].id)
                    {
                        case 0:
                            mainButtonActivated = !mainButtonActivated;
                            break;
                        default:
                            if (programs[buttons[i].id - 1].running)
                            {
                                programs[buttons[i].id - 1].stop();
                            }
                            else
                            {
                                programs[buttons[i].id - 1].start();
                            }
                            programs[buttons[i].id - 1].running = !programs[buttons[i].id - 1].running;
                            break;
                    }
                }
            }
            struct Window* window = &windows;
            while (TRUE)
            {
                window = window->next;
                if (window == NULL)
                {
                    break;
                }
                if (mouseX >= window->x && mouseX < window->x + window->width + 20 && mouseY >= window->y && mouseY < window->y + 42)
                {
                    window->dragging = TRUE;
                }
            }
        }
        else
        {
            struct Window* window = &windows;
            while (TRUE)
            {
                window = window->next;
                if (window == NULL)
                {
                    break;
                }
                window->dragging = FALSE;
            }
        }
    }
}

void drawMouse()
{
    EFI_GRAPHICS_OUTPUT_BLT_PIXEL* address = videoBuffer + mouseY * GOP->Mode->Info->HorizontalResolution + mouseX;
    uint8_t* buffer = mouseIcon;
    for (uint32_t y = 0; y < 16; y++)
    {
        for (uint32_t x = 0; x < 16; x++)
        {
            EFI_GRAPHICS_OUTPUT_BLT_PIXEL colour;
            if (*buffer != 2)
            {
                if (*buffer == 0)
                {
                    colour = black;
                }
                else
                {
                    colour = white;
                }
                *address = colour;
            }
            buffer++;
            address++;
        }
        address += GOP->Mode->Info->HorizontalResolution - 16;
    }
}

void start()
{
    while (TRUE)
    {
        blit(wallpaper, videoBuffer);
        struct Window* window = &windows;
        while (TRUE)
        {
            window = window->next;
            if (window == NULL)
            {
                break;
            }
            drawRectangle(window->x, window->y, window->width + 10, window->height + 47, grey);
            drawString(window->title, window->x + 5, window->y + 5, black);
            drawImage(window->x + 5, window->y + 42, window->width, window->height, window->buffer);
        }
        drawRectangle(0, GOP->Mode->Info->VerticalResolution - 32, GOP->Mode->Info->HorizontalResolution, 32, grey);
        if (mainButtonActivated)
        {
            drawRectangle(0, GOP->Mode->Info->VerticalResolution - 432, 300, 400, grey);
            drawRectangle(0, GOP->Mode->Info->VerticalResolution - 33, 300, 1, black);
            drawRectangle(2, GOP->Mode->Info->VerticalResolution - 30, 28, 28, black);
        }
        struct Button mainButton;
        mainButton.x = 4;
        mainButton.y = GOP->Mode->Info->VerticalResolution - 28;
        mainButton.width = 24;
        mainButton.height = 24;
        mainButton.id = 0;
        drawRectangle(mainButton.x, mainButton.y, mainButton.width, mainButton.height, white);
        buttons[0] = mainButton;
        drawRectangle(32, GOP->Mode->Info->VerticalResolution - 28, 1, 24, black);
        uint8_t buttonCountBuffer = 1;
        for (uint8_t i = 0; i < 1; i++)
        {
            struct Button button;
            button.x = 37 + i * 24 + i * 8;
            button.y = GOP->Mode->Info->VerticalResolution - 28;
            button.width = 24;
            button.height = 24;
            button.id = i + 1;
            if (programs[i].running)
            {
                programs[i].update();
                drawRectangle(button.x - 2, GOP->Mode->Info->VerticalResolution - 30, 28, 28, black);
            }
            drawImage(button.x, button.y, button.width, button.height, programs[i].icon);
            buttons[buttonCountBuffer] = button;
            buttonCountBuffer++;
        }
        buttonCount = buttonCountBuffer;
        drawMouse();
        waitForPit();
        blit(videoBuffer, (EFI_GRAPHICS_OUTPUT_BLT_PIXEL*)GOP->Mode->FrameBufferBase);
    }
}
