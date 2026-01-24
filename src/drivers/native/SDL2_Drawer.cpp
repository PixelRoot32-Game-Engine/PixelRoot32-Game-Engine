#ifdef PLATFORM_NATIVE

#include <drivers/native/SDL2_Drawer.h>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <cstdarg>
#include <cstdio>

namespace pr32 = pixelroot32;

pr32::drivers::native::SDL2_Drawer::SDL2_Drawer()
    : window(nullptr)
    , renderer(nullptr)
    , texture(nullptr)
    , pixels(nullptr)
    , cursorX(0)
    , cursorY(0)
    , textColor(0xFFFF)
    , textSize(1)
    , rotation(0)
{
}

pr32::drivers::native::SDL2_Drawer::~SDL2_Drawer() {
    if (texture) SDL_DestroyTexture(texture);
    if (renderer) SDL_DestroyRenderer(renderer);
    if (window) SDL_DestroyWindow(window);
    if (pixels) delete[] pixels;
    SDL_Quit();
}

void pr32::drivers::native::SDL2_Drawer::init() {
    SDL_Init(SDL_INIT_VIDEO);

    int scale = 2;
    window = SDL_CreateWindow(
        "ESP32 Game Engine Mock",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        displayWidth * scale,
        displayHeight * scale,
        SDL_WINDOW_SHOWN
    );

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    SDL_RenderSetLogicalSize(renderer, displayWidth, displayHeight);

    texture = SDL_CreateTexture(
        renderer,
        SDL_PIXELFORMAT_RGB565,
        SDL_TEXTUREACCESS_STREAMING,
        displayWidth,
        displayHeight
    );

    pixels = new uint16_t[displayWidth * displayHeight];
    memset(pixels, 0, displayWidth * displayHeight * sizeof(uint16_t));
}

void pr32::drivers::native::SDL2_Drawer::setRotation(uint8_t rot) {
    rotation = rot;
}

void pr32::drivers::native::SDL2_Drawer::clearBuffer() {
    // LIMPIAR FRAMEBUFFER (no renderer)
    memset(pixels, 0, displayWidth * displayHeight * sizeof(uint16_t));
}

void pr32::drivers::native::SDL2_Drawer::sendBuffer() {
    updateTexture();

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    SDL_RenderCopy(renderer, texture, nullptr, nullptr);
    SDL_RenderPresent(renderer);
}

// ---------- TEXTO ----------
// @deprecated These methods are obsolete. Text rendering is now handled by Renderer
// using the native bitmap font system. These methods are kept as empty stubs
// only for interface compatibility (DrawSurface requires them).
// The Renderer never calls these methods - all text goes through the font system.

void pr32::drivers::native::SDL2_Drawer::drawText(const char* text, int16_t x, int16_t y, uint16_t color, uint8_t size) {
    // Obsolete: This method should never be called.
    // All text rendering is handled by Renderer::drawText() using the font system.
    (void)text; (void)x; (void)y; (void)color; (void)size;
}

void pr32::drivers::native::SDL2_Drawer::drawTextCentered(const char* text, int16_t y, uint16_t color, uint8_t size) {
    // Obsolete: This method should never be called.
    // All text rendering is handled by Renderer::drawTextCentered() using the font system.
    (void)text; (void)y; (void)color; (void)size;
}

// ---------- PRIMITIVAS ----------

void pr32::drivers::native::SDL2_Drawer::drawFilledCircle(int x, int y, int r, uint16_t color) {
    int f = 1 - r;
    int ddF_x = 1;
    int ddF_y = -2 * r;
    int px = 0;
    int py = r;

    // línea central
    drawHLine(x - r, y, 2 * r + 1, color);

    while (px < py) {
        if (f >= 0) {
            py--;
            ddF_y += 2;
            f += ddF_y;
        }
        px++;
        ddF_x += 2;
        f += ddF_x;

        // Parte superior e inferior
        drawHLine(x - px, y + py, 2 * px + 1, color);
        drawHLine(x - px, y - py, 2 * px + 1, color);

        // Parte izquierda y derecha
        drawHLine(x - py, y + px, 2 * py + 1, color);
        drawHLine(x - py, y - px, 2 * py + 1, color);
    }
}

void pr32::drivers::native::SDL2_Drawer::drawCircle(int x, int y, int r, uint16_t color) {
    int f = 1 - r;
    int ddF_x = 1;
    int ddF_y = -2 * r;
    int px = 0;
    int py = r;

    setPixel(x, y + r, color);
    setPixel(x, y - r, color);
    setPixel(x + r, y, color);
    setPixel(x - r, y, color);

    while (px < py) {
        if (f >= 0) {
            py--;
            ddF_y += 2;
            f += ddF_y;
        }
        px++;
        ddF_x += 2;
        f += ddF_x;

        setPixel(x + px, y + py, color);
        setPixel(x - px, y + py, color);
        setPixel(x + px, y - py, color);
        setPixel(x - px, y - py, color);
        setPixel(x + py, y + px, color);
        setPixel(x - py, y + px, color);
        setPixel(x + py, y - px, color);
        setPixel(x - py, y - px, color);
    }
}

void pr32::drivers::native::SDL2_Drawer::drawFilledRectangle(int x, int y, int w, int h, uint16_t color) {
    for (int j = y; j < y + h; j++)
        for (int i = x; i < x + w; i++)
            setPixel(i, j, color);
}

void pr32::drivers::native::SDL2_Drawer::updateTexture() {
    SDL_UpdateTexture(texture, nullptr, pixels, displayWidth * sizeof(uint16_t));
}

bool pr32::drivers::native::SDL2_Drawer::processEvents() {
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT) return false;
    }
    return true;
}

void pr32::drivers::native::SDL2_Drawer::drawRectangle(int x, int y, int width, int height, uint16_t color) {
    drawLine(x, y, x + width - 1, y, color);
    drawLine(x + width - 1, y, x + width - 1, y + height - 1, color);
    drawLine(x + width - 1, y + height - 1, x, y + height - 1, color);
    drawLine(x, y + height - 1, x, y, color);
}

void pr32::drivers::native::SDL2_Drawer::drawLine(int x1, int y1, int x2, int y2, uint16_t color) {
    int dx = abs(x2 - x1);
    int dy = -abs(y2 - y1);
    int sx = (x1 < x2) ? 1 : -1;
    int sy = (y1 < y2) ? 1 : -1;
    int err = dx + dy;

    while (true) {
        setPixel(x1, y1, color);
        if (x1 == x2 && y1 == y2) break;
        int e2 = 2 * err;
        if (e2 >= dy) { err += dy; x1 += sx; }
        if (e2 <= dx) { err += dx; y1 += sy; }
    }
}

void pr32::drivers::native::SDL2_Drawer::drawBitmap(int x, int y, int w, int h, const uint8_t* bitmap, uint16_t color) {
    if (!bitmap) return;
    int bytesPerRow = (w + 7) / 8;

    for (int j = 0; j < h; j++) {
        for (int i = 0; i < w; i++) {
            int byteIndex = j * bytesPerRow + (i >> 3);
            if (bitmap[byteIndex] & (1 << (i & 7))) {
                setPixel(x + i, y + j, color);
            }
        }
    }
}

void pr32::drivers::native::SDL2_Drawer::drawPixel(int x, int y, uint16_t color) {
    setPixel(x, y, color);
}

void pr32::drivers::native::SDL2_Drawer::setTextColor(uint16_t color) {
    textColor = color;
}

void pr32::drivers::native::SDL2_Drawer::setTextSize(uint8_t size) {
    textSize = size;
}

void pr32::drivers::native::SDL2_Drawer::setCursor(int16_t x, int16_t y) {
    cursorX = x;
    cursorY = y;
}

uint16_t pr32::drivers::native::SDL2_Drawer::color565(uint8_t r, uint8_t g, uint8_t b) {
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

void pr32::drivers::native::SDL2_Drawer::setDisplaySize(int w, int h) {
    displayWidth = w;
    displayHeight = h;
}

void pr32::drivers::native::SDL2_Drawer::present() {
    sendBuffer(); // wrapper por compatibilidad
}

#endif // PLATFORM_NATIVE
