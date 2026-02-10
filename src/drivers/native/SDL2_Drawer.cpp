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

    // Set nearest neighbor scaling hint BEFORE creating renderer
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");

    // We use a scale factor for the window so it's not too small on high-res monitors
    // but the window itself will be our physical resolution scaled.
    int windowScale = 2; 
    int winWidth = physicalWidth * windowScale;
    int winHeight = physicalHeight * windowScale;

    // Create window
    window = SDL_CreateWindow(
        "PixelRoot32 Engine",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        winWidth,
        winHeight,
        SDL_WINDOW_SHOWN
    );

    // Create renderer - No logical size needed, we'll let SDL scale Copy to viewport
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    
    // Create texture at logical resolution (the actual game framebuffer size)
    texture = SDL_CreateTexture(
        renderer,
        SDL_PIXELFORMAT_RGB565,
        SDL_TEXTUREACCESS_STREAMING,
        logicalWidth,
        logicalHeight
    );
    
    // Allocate framebuffer at logical resolution
    if (pixels) delete[] pixels;
    pixels = new uint16_t[logicalWidth * logicalHeight];
    memset(pixels, 0, logicalWidth * logicalHeight * sizeof(uint16_t));

    printf("[SDL2_Drawer] Initialized: Logical=%dx%d, Physical=%dx%d, Window=%dx%d\n", 
           logicalWidth, logicalHeight, physicalWidth, physicalHeight, winWidth, winHeight);
}

void pr32::drivers::native::SDL2_Drawer::setRotation(uint16_t rot) {
    // Standardize rotation to index 0-3 (0, 90, 180, 270)
    if (rot == 90) rotation = 1;
    else if (rot == 180) rotation = 2;
    else if (rot == 270) rotation = 3;
    else if (rot >= 360) rotation = (rot / 90) % 4;
    else rotation = rot % 4;
    
    printf("[SDL2_Drawer] Rotation set to %d (%d degrees)\n", rotation, rotation * 90);
}

void pr32::drivers::native::SDL2_Drawer::clearBuffer() {
    // Clear framebuffer at logical resolution
    memset(pixels, 0, logicalWidth * logicalHeight * sizeof(uint16_t));
}

void pr32::drivers::native::SDL2_Drawer::sendBuffer() {
    updateTexture();

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    // SDL_RenderCopy with nullptr as dstrect will scale to fill the entire renderer viewport (the window)
    if (rotation == 0) {
        SDL_RenderCopy(renderer, texture, nullptr, nullptr);
    } else {
        double angle = rotation * 90.0;
        SDL_RenderCopyEx(renderer, texture, nullptr, nullptr, angle, nullptr, SDL_FLIP_NONE);
    }
    
    SDL_RenderPresent(renderer);
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
    SDL_UpdateTexture(texture, nullptr, pixels, logicalWidth * sizeof(uint16_t));
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

#endif // PLATFORM_NATIVE
