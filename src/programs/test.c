#include "../program.h"

uint64_t id = 0;
Window* window = NULL;
BOOLEAN map[32][32] = {
    { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
    { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
    { 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1 },
    { 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1 },
    { 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1 },
    { 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 1 },
    { 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 1 },
    { 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 1, 1 },
    { 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 1, 1 },
    { 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 1, 1 },
    { 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 1, 1 },
    { 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 1, 1 },
    { 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 1, 0, 0, 0, 1, 1 },
    { 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1 },
    { 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1 },
    { 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1 },
    { 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1 },
    { 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1 },
    { 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1 },
    { 1, 1, 0, 0, 0, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1 },
    { 1, 1, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1 },
    { 1, 1, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1 },
    { 1, 1, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1 },
    { 1, 1, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1 },
    { 1, 1, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1 },
    { 1, 1, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1 },
    { 1, 1, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1 },
    { 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1 },
    { 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1 },
    { 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1 },
    { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
    { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 }
};
EFI_GRAPHICS_OUTPUT_BLT_PIXEL* texture = NULL;
BOOLEAN q = FALSE;
BOOLEAN e = FALSE;
BOOLEAN w = FALSE;
BOOLEAN a = FALSE;
BOOLEAN s = FALSE;
BOOLEAN d = FALSE;
BOOLEAN shift = FALSE;
float playerX = 16.0f;
float playerY = 16.0f;
float direction = 0.0f;

void _start(uint64_t pid)
{
    id = pid;
    texture = allocate(64 * 64 * sizeof(EFI_GRAPHICS_OUTPUT_BLT_PIXEL));
    readBitmap(readFile(L"programs/test/wall.bmp", NULL), texture);
    window = allocateWindow(640, 360, L"Game", L"programs/test/test.bmp");
    window->hideMouse = TRUE;
}

void update(uint64_t frameSkips)
{
    Event* event = (Event*)&window->events;
    while (iterateList((void**)&event))
    {
        switch (event->id)
        {
            case 0:
                unallocateWindow(window);
                unallocate(texture);
                quit(id);
                break;
            case 1:
                switch (((KeyEvent*)event)->scancode)
                {
                    case 16:
                        q = ((KeyEvent*)event)->pressed;
                        break;
                    case 18:
                        e = ((KeyEvent*)event)->pressed;
                        break;
                    case 17:
                        w = ((KeyEvent*)event)->pressed;
                        break;
                    case 30:
                        a = ((KeyEvent*)event)->pressed;
                        break;
                    case 31:
                        s = ((KeyEvent*)event)->pressed;
                        break;
                    case 32:
                        d = ((KeyEvent*)event)->pressed;
                        break;
                    case 42:
                        shift = ((KeyEvent*)event)->pressed;
                        break;
                }
                break;
        }
        removeItem((void**)&window->events, event);
        event = (Event*)&window->events;
    }
    if (q)
    {
        direction -= 0.05f * frameSkips;
    }
    if (e)
    {
        direction += 0.05f * frameSkips;
    }
    float speed = 0.05f;
    if (shift)
    {
        speed = 0.1f;
    }
    if (w)
    {
        float newX = playerX + cos(direction) * speed * frameSkips;
        if (!map[(uint8_t)(playerY)][(uint8_t)(newX)])
        {
            playerX = newX;
        }
        float newY = playerY + sin(direction) * speed * frameSkips;
        if (!map[(uint8_t)(newY)][(uint8_t)(playerX)])
        {
            playerY = newY;
        }
    }
    if (a)
    {
        float moveDirection = direction - 1.57f;
        float newX = playerX + cos(moveDirection) * speed * frameSkips;
        if (!map[(uint8_t)(playerY)][(uint8_t)(newX)])
        {
            playerX = newX;
        }
        float newY = playerY + sin(moveDirection) * speed * frameSkips;
        if (!map[(uint8_t)(newY)][(uint8_t)(playerX)])
        {
            playerY = newY;
        }
    }
    if (s)
    {
        float newX = playerX - cos(direction) * speed * frameSkips;
        if (!map[(uint8_t)(playerY)][(uint8_t)(newX)])
        {
            playerX = newX;
        }
        float newY = playerY - sin(direction) * speed * frameSkips;
        if (!map[(uint8_t)(newY)][(uint8_t)(playerX)])
        {
            playerY = newY;
        }
    }
    if (d)
    {
        float moveDirection = direction + 1.57f;
        float newX = playerX + cos(moveDirection) * speed * frameSkips;
        if (!map[(uint8_t)(playerY)][(uint8_t)(newX)])
        {
            playerX = newX;
        }
        float newY = playerY + sin(moveDirection) * speed * frameSkips;
        if (!map[(uint8_t)(newY)][(uint8_t)(playerX)])
        {
            playerY = newY;
        }
    }
    int halfHeight = window->height / 2;
    for (int y = 0; y < window->height; y++)
    {
        uint8_t brightness = 0;
        if (y < halfHeight)
        {
            brightness = 128 - (uint8_t)(y / (float)halfHeight * 128);
        }
        else
        {
            brightness = (uint8_t)((y - halfHeight) / (float)halfHeight * 128);
        }
        EFI_GRAPHICS_OUTPUT_BLT_PIXEL pixel;
        pixel.Red = brightness;
        pixel.Green = brightness;
        pixel.Blue = brightness;
        for (uint32_t x = 0; x < window->width; x++)
        {
            window->buffer[y * window->width + x] = pixel;
        }
    }
    int32_t halfWidth = window->width / 2;
    float distribution = halfWidth / tan(1.57f / 2.0f);
    for (int64_t x = 0; x < window->width; x++)
    {
        float raycastX = playerX;
        float raycastY = playerY;
        float raycastDirection = direction + atan((x - halfWidth) / distribution);
        while (!map[(uint8_t)(raycastY)][(uint8_t)(raycastX)])
        {
            raycastX += cos(raycastDirection) * 0.01f;
            raycastY += sin(raycastDirection) * 0.01f;
        }
        float distance = sqrt((raycastX - playerX) * (raycastX - playerX) + (raycastY - playerY) * (raycastY - playerY));
        uint64_t wallHeight = (uint64_t)(distribution * 2 / (distance * cos(raycastDirection - direction)));
        uint64_t halfWallHeight = wallHeight / 2;
        float uncappedBrightness = 1.5f - distance / 7.5f;
        float brightness = max(min(uncappedBrightness, 1.0f), 0.0f);
        for (uint64_t y = 0; y < wallHeight; y++)
        {
            uint32_t yPixel = halfHeight - halfWallHeight + y;
            if (yPixel >= 0 && yPixel < window->height)
            {
                EFI_GRAPHICS_OUTPUT_BLT_PIXEL colour = texture[(uint8_t)((float)(y) / wallHeight * 64) * 64 + (uint8_t)((int64_t)((raycastX + raycastY) * 20.0f) % 40 / 40.0f * 64)];
                EFI_GRAPHICS_OUTPUT_BLT_PIXEL pixel;
                pixel.Red = brightness * colour.Red;
                pixel.Green = brightness * colour.Green;
                pixel.Blue = brightness * colour.Blue;
                window->buffer[yPixel * window->width + x] = pixel;
            }
        }
    }
}
