#pragma once
#ifndef SDL2_DRAWER_H
#define SDL2_DRAWER_H

#ifdef PLATFORM_NATIVE

#include "DrawSurface.h"
// SDL2 specific includes would go here
#include <SDL2/SDL.h>
#include <stdint.h>

constexpr int FONT_W = 5;
constexpr int FONT_H = 7;
constexpr int FONT_SPACING = 1;

namespace pixelroot32::drivers::native {
class SDL2_Drawer : public pixelroot32::graphics::DrawSurface {
public:
    SDL2_Drawer();
    virtual ~SDL2_Drawer();

    void init() override;
    void setRotation(uint8_t rotation) override;

    void clearBuffer() override;
    void sendBuffer() override;

    void drawText(const char* text, int16_t x, int16_t y, uint16_t color, uint8_t size) override;
    void drawTextCentered(const char* text, int16_t y, uint16_t color, uint8_t size) override;
    void drawFilledCircle(int x, int y, int radius, uint16_t color) override;
    void drawCircle(int x, int y, int radius, uint16_t color) override;
    void drawRectangle(int x, int y, int width, int height, uint16_t color) override;
    void drawFilledRectangle(int x, int y, int width, int height, uint16_t color) override;
    void drawLine(int x1, int y1, int x2, int y2, uint16_t color) override;
    void drawBitmap(int x, int y, int width, int height, const uint8_t *bitmap, uint16_t color) override;
    void drawPixel(int x, int y, uint16_t color) override;

    void setTextColor(uint16_t color) override;
    void setTextSize(uint8_t size) override;
    void setCursor(int16_t x, int16_t y) override;
    
    uint16_t color565(uint8_t r, uint8_t g, uint8_t b) override;

    void setDisplaySize(int w, int h) override;

    void present() override;

    bool processEvents() override;

    void setContrast(uint8_t value) override {
        (void)value;
        // SDL2 no soporta contraste real → noop
    }


private:
    SDL_Window* window;
    SDL_Renderer* renderer;
    SDL_Texture* texture;
    uint16_t* pixels; // Framebuffer (RGB565 format, matches texture)
    int16_t cursorX, cursorY;
    uint16_t textColor;
    uint8_t textSize;
    uint8_t rotation;

    int displayWidth;
    int displayHeight;

    void updateTexture();

    inline int16_t textWidth(const char* text) {
        // Simple calculation
        int len = strlen(text);
        return len * 6 * textSize;
    }

    inline void print(const char* text, const uint8_t font5x7[][7]) {
        if (!text) return;

        int16_t x = cursorX;
        int16_t y = cursorY;

        int cx = x;

        while (*text) {
            char c = *text++;
            if (c < 32 || c > 126) {
                cx += FONT_W + FONT_SPACING;
                continue;
            }

            const uint8_t* glyph = font5x7[c - 32];

            for (int row = 0; row < FONT_H; ++row) {
                uint8_t bits = glyph[row];
                for (int col = 0; col < FONT_W; ++col) {
                    if (bits & (1 << (FONT_W - 1 - col))) {
                        setPixel(cx + col, y + row, textColor);
                    }
                }
            }

            cx += FONT_W + FONT_SPACING;
        }
    }

    inline void rgb565ToRGBA(
        uint16_t color,
        uint8_t& r,
        uint8_t& g,
        uint8_t& b,
        uint8_t& a
    ) {
        r = ((color >> 11) & 0x1F) * 255 / 31;
        g = ((color >> 5)  & 0x3F) * 255 / 63;
        b = ( color        & 0x1F) * 255 / 31;
        a = 255;
    }

    inline void drawHLine(int x, int y, int w, uint16_t color) {
        for (int i = 0; i < w; i++) {
            setPixel(x + i, y, color);
        }
    }

    inline void setPixel(int x, int y, uint16_t color) {
        if (x < 0 || y < 0 || x >= displayWidth || y >= displayHeight) return;
        
        pixels[y * displayWidth + x] = color;
    }
};

} // namespace pixelroot32::drivers::native

#endif // PLATFORM_NATIVE

#endif // SDL2_DRAWER_H
