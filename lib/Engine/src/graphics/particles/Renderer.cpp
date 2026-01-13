#include "graphics/Renderer.h"
#include <stdarg.h>
#ifdef PLATFORM_NATIVE
    #include "SDL2_Drawer.h"
    #include "MockSPI.h"
#else
    #include "TFT_eSPI_Drawer.h"
    #include <SPI.h>
    #include <SafeString.h>
#endif

Renderer::Renderer(const DisplayConfig& config) : config(config) {
    #if defined(PLATFORM_NATIVE)
        drawer = new SDL2_Drawer();
    #else
        drawer = new TFT_eSPI_Drawer();
    #endif

    xOffset = config.xOffset;
    yOffset = config.yOffset;
}


void Renderer::init() {
    setDisplaySize(config.width, config.height);
    getDrawSurface().setDisplaySize(getHeight(), getWidth());
    getDrawSurface().init();
    // getDrawSurface().setContrast(255);
    // getDrawSurface().setRotation(config.rotation);
}

void Renderer::beginFrame() {
    getDrawSurface().clearBuffer();
}

void Renderer::endFrame() {
    getDrawSurface().sendBuffer();
}

void Renderer::drawText(const char* text, int16_t x, int16_t y, uint16_t color, uint8_t size) {
    getDrawSurface().drawText(text, x, y, color, size);
}

void Renderer::drawTextCentered(const char* text, int16_t y, uint16_t color, uint8_t size) {
    getDrawSurface().drawTextCentered(text, y, color, size);
}

void Renderer::drawFilledCircle(int x, int y, int radius, uint16_t color) {
    getDrawSurface().drawFilledCircle(xOffset + x, yOffset + y, radius, color);
}

void Renderer::drawCircle(int x, int y, int radius, uint16_t color) {
    getDrawSurface().drawCircle(xOffset + x, yOffset + y, radius, color);
}

void Renderer::drawRectangle(int x, int y, int width, int height, uint16_t color) {
    getDrawSurface().drawRectangle(xOffset + x, yOffset + y, width, height, color);
}

void Renderer::drawFilledRectangle(int x, int y, int width, int height, uint16_t color) {
    getDrawSurface().drawFilledRectangle(xOffset + x, yOffset + y, width, height, color);
}

void Renderer::drawLine(int x1, int y1, int x2, int y2, uint16_t color) {
    getDrawSurface().drawLine(xOffset + x1, yOffset + y1, xOffset + x2, yOffset + y2, color);
}

void Renderer::setFont(const uint8_t* font) {
    // Optional: Implement font setting if your DrawSurface supports it.
}

//draw an image to the screen in an bitmap format
void Renderer::drawBitmap(int x, int y, int width, int height, const uint8_t *bitmap, uint16_t color) {
    getDrawSurface().drawBitmap(xOffset + x, yOffset + y, width, height, bitmap, color);
}

void Renderer::drawPixel(int x, int y, uint16_t color) {
    getDrawSurface().drawPixel(x, y, color);
}


