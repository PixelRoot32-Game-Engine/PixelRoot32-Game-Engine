#pragma once
#include "DrawSurface.h"
#include "DisplayConfig.h"

#ifdef PLATFORM_ESP32
    #include <mock/MockSafeString.h>
#endif

class Renderer {
public:
    Renderer(const DisplayConfig& config);

    ~Renderer() {
       delete drawer;
    }

    void init();
    void beginFrame();
    void endFrame();

    // Return a pointer to the DrawSurface object
    DrawSurface& getDrawSurface() { return *drawer; }

    void drawText(const char* text, int16_t x, int16_t y, uint16_t color, uint8_t size);
    void drawTextCentered(const char* text, int16_t y, uint16_t color, uint8_t size);
    void drawFilledCircle(int x, int y, int radius,uint16_t color);
    void drawCircle(int x, int y, int radius,uint16_t color);
    void drawRectangle(int x, int y, int width, int height,uint16_t color);
    void drawFilledRectangle(int x, int y, int width, int height, uint16_t color);
    void drawLine(int x1, int y1, int x2, int y2,uint16_t color);
    void drawBitmap(int x, int y, int width, int height, const uint8_t *bitmap, uint16_t color);
    void drawPixel(int x, int y, uint16_t color);

    void setDisplaySize(int w, int h) {
        width = w;
        height = h;
    }

    int getWidth() const { return width; }
    int getHeight() const { return height; }

    void setDisplayOffset(int x, int y) {
        xOffset = x;
        yOffset = y;
    }

    void setContrast(uint8_t level) {
        getDrawSurface().setContrast(level);
    }

    void setFont(const uint8_t* font);

    int getXOffset() const { return xOffset; }
    int getYOffset() const { return yOffset; }




private:
    DrawSurface* drawer;

    DisplayConfig config;

    int width = 240;
    int height = 240;

    int xOffset = 0;
    int yOffset = 0;
};