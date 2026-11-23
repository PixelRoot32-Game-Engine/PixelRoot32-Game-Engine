#include "platforms/PlatformSelect.h"

#if PIXELROOT32_HAS_SDL

#include <drivers/native/SDL2_Drawer.h>
#include <input/InputManager.h>
#include <input/TouchEventDispatcher.h>
#include <core/Log.h>
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
    , touchDispatcher(nullptr)
    , inputManager(nullptr)
    , sdlInitialized(false)
    , drawableWidth(0)
    , drawableHeight(0)
    , windowWidth(0)
    , windowHeight(0)
{
}

pr32::drivers::native::SDL2_Drawer::~SDL2_Drawer() {
    if (texture) SDL_DestroyTexture(texture);
    if (renderer) SDL_DestroyRenderer(renderer);
    if (window) SDL_DestroyWindow(window);
    if (pixels) delete[] pixels;
    // Quit only the subsystem init() acquired. SDL_Quit() tore down the whole
    // library from here, including the audio subsystem a backend owns and the
    // device it still had open, which faulted at shutdown in issue #273.
    if (sdlInitialized) {
        SDL_QuitSubSystem(SDL_INIT_VIDEO);
        sdlInitialized = false;
    }
}

void pr32::drivers::native::SDL2_Drawer::init() {
    if (SDL_InitSubSystem(SDL_INIT_VIDEO) != 0) {
        pixelroot32::core::logging::log(pixelroot32::core::logging::LogLevel::Error,
                                        "[SDL2_Drawer] SDL video initialization failed: %s\n",
                                        SDL_GetError());
    } else {
        // Paired with the SDL_QuitSubSystem(SDL_INIT_VIDEO) in the destructor,
        // so a double init cannot leave a reference behind.
        sdlInitialized = true;
    }

    // Set nearest neighbor scaling hint BEFORE creating renderer
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");

    // Window size comes from the display instead of a hardcoded 2: the same
    // 240x240 game should not open as a thumbnail on a 27" panel and larger
    // than the screen on a netbook. SDL reports the usable bounds in window
    // units, so on a HiDPI screen this sizes the window in points and the
    // drawable comes out larger -- ALLOW_HIGHDPI is what makes that crisp.
    SDL_Rect usable = {0, 0, 0, 0};
    if (SDL_GetNumVideoDisplays() > 0) {
        SDL_GetDisplayUsableBounds(0, &usable);
    }
    const int windowScale = defaultWindowScale(physicalWidth, physicalHeight, usable.w, usable.h);
    const int winWidth = physicalWidth * windowScale;
    const int winHeight = physicalHeight * windowScale;

    // Resizable: the user decides how big the game is, and refreshLayout()
    // re-fits the framebuffer to whatever the window becomes.
    window = SDL_CreateWindow(
        "PixelRoot32 Engine",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        winWidth,
        winHeight,
        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI
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

    refreshLayout();

    pixelroot32::core::logging::log("[SDL2_Drawer] Initialized: Logical=%dx%d, Physical=%dx%d, Window=%dx%d (%dx scale)\n", 
           logicalWidth, logicalHeight, physicalWidth, physicalHeight, winWidth, winHeight, windowScale);
}

void pr32::drivers::native::SDL2_Drawer::setRotation(uint16_t rot) {
    // Standardize rotation to index 0-3 (0, 90, 180, 270)
    if (rot == 90) rotation = 1;
    else if (rot == 180) rotation = 2;
    else if (rot == 270) rotation = 3;
    else if (rot >= 360) rotation = (rot / 90) % 4;
    else rotation = rot % 4;
    
    pixelroot32::core::logging::log("[SDL2_Drawer] Rotation set to %d (%d degrees)\n", rotation, rotation * 90);
}

void pr32::drivers::native::SDL2_Drawer::clearBuffer() {
    // Clear framebuffer at logical resolution
    memset(pixels, 0, logicalWidth * logicalHeight * sizeof(uint16_t));
}

void pr32::drivers::native::SDL2_Drawer::sendBuffer() {
    updateTexture();
    refreshLayout();

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    // Same layout that maps the mouse back: one scale, one letterbox origin, so
    // clicks land where the player clicked at any window size.
    const WindowRect rect = framebufferRect(presentLayout, xOffset, yOffset,
                                            logicalWidth, logicalHeight);
    SDL_Rect dstRect = {rect.x, rect.y, rect.width, rect.height};

    if (rotation == 0) {
        SDL_RenderCopy(renderer, texture, nullptr, &dstRect);
    } else {
        double angle = rotation * 90.0;
        SDL_RenderCopyEx(renderer, texture, nullptr, &dstRect, angle, nullptr, SDL_FLIP_NONE);
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
        switch (e.type) {
            case SDL_QUIT:
                return false;

            case SDL_KEYDOWN:
                // F11 and Alt+Enter are the two shortcuts a desktop player tries
                // first. Auto-repeat is ignored, or holding the key would toggle
                // fullscreen on every repeat tick.
                if (e.key.repeat == 0 &&
                    (e.key.keysym.sym == SDLK_F11 ||
                     ((e.key.keysym.sym == SDLK_RETURN || e.key.keysym.sym == SDLK_KP_ENTER) &&
                      (e.key.keysym.mod & KMOD_ALT) != 0))) {
                    toggleFullscreen();
                }
                // Keyboard state is polled by the engine, not pushed by events.
                continue;

            case SDL_WINDOWEVENT:
                // Nothing to do: the layout is derived from the current sizes on
                // the next frame and on every mouse event, so a resize or a move
                // to another display is picked up without extra state.
                continue;

            default:
                break;
        }

        if (e.type != SDL_MOUSEBUTTONDOWN && e.type != SDL_MOUSEBUTTONUP &&
            e.type != SDL_MOUSEMOTION) {
            continue;
        }

        // Mouse events are in window units (points on a HiDPI screen), so the
        // inverse mapping uses inputLayout, not the drawable size that
        // presentation uses. They differ by the display scale factor.
        const int windowX = e.type == SDL_MOUSEMOTION ? e.motion.x : e.button.x;
        const int windowY = e.type == SDL_MOUSEMOTION ? e.motion.y : e.button.y;

        int fbX = 0;
        int fbY = 0;
        refreshLayout();
        mapToFramebuffer(inputLayout, windowX, windowY, xOffset, yOffset,
                         logicalWidth, logicalHeight, fbX, fbY);

        // Prefer using touchDispatcher if available (new integration)
        if (touchDispatcher != nullptr) {
            // Determine if button is pressed
            bool pressed = false;
            if (e.type == SDL_MOUSEBUTTONDOWN) pressed = true;
            else if (e.type == SDL_MOUSEBUTTONUP) pressed = false;
            else {
                // Check if left button is held
                pressed = (SDL_GetMouseState(nullptr, nullptr) & SDL_BUTTON(SDL_BUTTON_LEFT)) != 0;
            }

            touchDispatcher->processTouch(0, pressed,
                                          static_cast<int16_t>(fbX),
                                          static_cast<int16_t>(fbY),
                                          SDL_GetTicks());
        }
        // Fallback to inputManager for backwards compatibility. It maps the SDL
        // event itself, so it gets a copy carrying the already-mapped
        // coordinates -- it used to receive raw window pixels and treat them as
        // framebuffer pixels, which was only ever right at a scale of 1.
        else if (inputManager != nullptr) {
            SDL_Event mapped = e;
            if (mapped.type == SDL_MOUSEMOTION) {
                mapped.motion.x = fbX;
                mapped.motion.y = fbY;
            } else {
                mapped.button.x = fbX;
                mapped.button.y = fbY;
            }
            inputManager->processSDLEvent(&mapped);
        }
    }
    return true;
}

void pr32::drivers::native::SDL2_Drawer::refreshLayout() {
    if (renderer == nullptr || window == nullptr) return;

    int currentDrawableWidth = 0;
    int currentDrawableHeight = 0;
    if (SDL_GetRendererOutputSize(renderer, &currentDrawableWidth, &currentDrawableHeight) != 0) {
        return;
    }

    int currentWindowWidth = 0;
    int currentWindowHeight = 0;
    SDL_GetWindowSize(window, &currentWindowWidth, &currentWindowHeight);

    if (currentDrawableWidth == drawableWidth && currentDrawableHeight == drawableHeight &&
        currentWindowWidth == windowWidth && currentWindowHeight == windowHeight) {
        return;
    }

    drawableWidth = currentDrawableWidth;
    drawableHeight = currentDrawableHeight;
    windowWidth = currentWindowWidth;
    windowHeight = currentWindowHeight;

    presentLayout = fitWindowLayout(drawableWidth, drawableHeight, physicalWidth, physicalHeight);
    inputLayout = fitWindowLayout(windowWidth, windowHeight, physicalWidth, physicalHeight);
}

bool pr32::drivers::native::SDL2_Drawer::isFullscreen() const {
    if (window == nullptr) return false;
    return (SDL_GetWindowFlags(window) & SDL_WINDOW_FULLSCREEN_DESKTOP) == SDL_WINDOW_FULLSCREEN_DESKTOP;
}

void pr32::drivers::native::SDL2_Drawer::setFullscreen(bool fullscreen) {
    if (window == nullptr) return;

    // Borderless desktop fullscreen, not a display-mode switch: it never
    // changes the desktop resolution and leaving it restores the same window.
    if (SDL_SetWindowFullscreen(window, fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0) != 0) {
        pixelroot32::core::logging::log(pixelroot32::core::logging::LogLevel::Warning,
                                        "[SDL2_Drawer] Fullscreen toggle failed: %s\n",
                                        SDL_GetError());
        return;
    }

    // The sizes may be reported unchanged across the transition; invalidate the
    // cache so the next frame recomputes the letterbox regardless.
    drawableWidth = -1;
    windowWidth = -1;
    refreshLayout();
}

void pr32::drivers::native::SDL2_Drawer::toggleFullscreen() {
    setFullscreen(!isFullscreen());
}

void pr32::drivers::native::SDL2_Drawer::setInputManager(pixelroot32::input::InputManager* im) {
    inputManager = im;
}

void pr32::drivers::native::SDL2_Drawer::setTouchDispatcher(pixelroot32::input::TouchEventDispatcher* td) {
    touchDispatcher = td;
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

void pr32::drivers::native::SDL2_Drawer::drawTileDirect(uint16_t x, uint16_t y, uint16_t width, uint16_t height, const uint8_t* data) {
    // Not optimized for SDL2 - fall back to pixel-by-pixel
    if (!data) return;
    
    for (uint16_t row = 0; row < height; row++) {
        for (uint16_t col = 0; col < width; col++) {
            uint8_t colorIndex = data[row * width + col];
            if (colorIndex != 0) {  // Skip transparent pixels
                setPixel(x + col, y + row, pixels[colorIndex]);  // Use palette
            }
        }
    }
}

#endif // PIXELROOT32_HAS_SDL
