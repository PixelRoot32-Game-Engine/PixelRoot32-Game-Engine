/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#pragma once

#include "DrawSurface.h"
#include <cmath>

namespace pixelroot32::graphics {

/**
 * @class BaseDrawSurface
 * @brief Optional base class for DrawSurface implementations that provides default primitive rendering.
 * 
 * Users can inherit from this class to avoid implementing every single primitive.
 * At minimum, a subclass should implement:
 * - init()
 * - drawPixel()
 * - sendBuffer()
 * - clearBuffer()
 */
class BaseDrawSurface : public DrawSurface {
public:
    virtual ~BaseDrawSurface() = default;

    // State management defaults
    // State management defaults
    /**
     * @brief Sets the text color.
     * @param color The 16-bit text color.
     */
    void setTextColor(uint16_t color) override { textColor = color; }

    /**
     * @brief Sets the text size.
     * @param size The text size multiplier.
     */
    void setTextSize(uint8_t size) override { textSize = size; }

    /**
     * @brief Sets the cursor position for text drawing.
     * @param x The X coordinate.
     * @param y The Y coordinate.
     */
    void setCursor(int16_t x, int16_t y) override { cursorX = x; cursorY = y; }

    /**
     * @brief Sets the display contrast.
     * @param level Contrast level (0-255).
     */
    void setContrast(uint8_t level) override { contrast = level; }

    /**
     * @brief Sets the display rotation.
     * @param rot Rotation index or degrees.
     */
    void setRotation(uint16_t rot) override { rotation = rot; }
    
    // Size management defaults
    // Size management defaults
    /**
     * @brief Sets the logical display size.
     * @param w The logical width.
     * @param h The logical height.
     */
    void setDisplaySize(int w, int h) override { logicalWidth = w; logicalHeight = h; }

    /**
     * @brief Sets the physical display size.
     * @param w The physical width.
     * @param h The physical height.
     */
    void setPhysicalSize(int w, int h) override { physicalWidth = w; physicalHeight = h; }

    /**
     * @brief Sets the display offset.
     * @param x The X offset.
     * @param y The Y offset.
     */
    void setOffset(int x, int y) override { xOffset = x; yOffset = y; }

    /**
     * @brief Presents the current frame buffer.
     */
    void present() override { sendBuffer(); }

    /**
     * @brief Converts RGB888 color to RGB565 format.
     * @param r Red component (0-255).
     * @param g Green component (0-255).
     * @param b Blue component (0-255).
     * @return The 16-bit color value.
     */
    uint16_t color565(uint8_t r, uint8_t g, uint8_t b) override {
        return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
    }

    // Default primitive implementations using drawPixel (slow but functional)
    /**
     * @brief Draws a line between two points.
     * @param x1 Start X coordinate.
     * @param y1 Start Y coordinate.
     * @param x2 End X coordinate.
     * @param y2 End Y coordinate.
     * @param color The line color.
     */
    void drawLine(int x1, int y1, int x2, int y2, uint16_t color) override {
        int dx = abs(x2 - x1), sx = x1 < x2 ? 1 : -1;
        int dy = -abs(y2 - y1), sy = y1 < y2 ? 1 : -1;
        int err = dx + dy, e2;
        while (true) {
            drawPixel(x1, y1, color);
            if (x1 == x2 && y1 == y2) break;
            e2 = 2 * err;
            if (e2 >= dy) { err += dy; x1 += sx; }
            if (e2 <= dx) { err += dx; y1 += sy; }
        }
    }

    /**
     * @brief Draws a rectangle outline.
     * @param x Top-left X coordinate.
     * @param y Top-left Y coordinate.
     * @param w Width of the rectangle.
     * @param h Height of the rectangle.
     * @param color The outline color.
     */
    void drawRectangle(int x, int y, int w, int h, uint16_t color) override {
        for (int i = 0; i < w; i++) {
            drawPixel(x + i, y, color);
            drawPixel(x + i, y + h - 1, color);
        }
        for (int i = 0; i < h; i++) {
            drawPixel(x, y + i, color);
            drawPixel(x + w - 1, y + i, color);
        }
    }

    /**
     * @brief Draws a filled rectangle.
     * @param x Top-left X coordinate.
     * @param y Top-left Y coordinate.
     * @param w Width of the rectangle.
     * @param h Height of the rectangle.
     * @param color The fill color.
     */
    void drawFilledRectangle(int x, int y, int w, int h, uint16_t color) override {
        for (int j = 0; j < h; j++) {
            for (int i = 0; i < w; i++) {
                drawPixel(x + i, y + j, color);
            }
        }
    }

    /**
     * @brief Draws a circle outline.
     * @param x0 Center X coordinate.
     * @param y0 Center Y coordinate.
     * @param r Radius of the circle.
     * @param color The outline color.
     */
    void drawCircle(int x0, int y0, int r, uint16_t color) override {
        int x = -r, y = 0, err = 2 - 2 * r;
        do {
            drawPixel(x0 - x, y0 + y, color);
            drawPixel(x0 - y, y0 - x, color);
            drawPixel(x0 + x, y0 - y, color);
            drawPixel(x0 + y, y0 + x, color);
            r = err;
            if (r <= y) err += ++y * 2 + 1;
            if (r > x || err > y) err += ++x * 2 + 1;
        } while (x < 0);
    }

    /**
     * @brief Draws a filled circle.
     * @param x0 Center X coordinate.
     * @param y0 Center Y coordinate.
     * @param r Radius of the circle.
     * @param color The fill color.
     */
    void drawFilledCircle(int x0, int y0, int r, uint16_t color) override {
        int x = -r, y = 0, err = 2 - 2 * r;
        do {
            for (int i = x0 + x; i <= x0 - x; i++) {
                drawPixel(i, y0 + y, color);
                drawPixel(i, y0 - y, color);
            }
            r = err;
            if (r <= y) err += ++y * 2 + 1;
            if (r > x || err > y) err += ++x * 2 + 1;
        } while (x < 0);
    }

    /**
     * @brief Draws a bitmap.
     * @param x Top-left X coordinate.
     * @param y Top-left Y coordinate.
     * @param w Width of the bitmap.
     * @param h Height of the bitmap.
     * @param bitmap Pointer to the bitmap data.
     * @param color The color to draw.
     */
    void drawBitmap(int x, int y, int w, int h, const uint8_t *bitmap, uint16_t color) override {
        for (int j = 0; j < h; j++) {
            for (int i = 0; i < w; i++) {
                if (bitmap[j * w + i]) {
                    drawPixel(x + i, y + j, color);
                }
            }
        }
    }

protected:
    uint16_t textColor = 0xFFFF;
    uint8_t textSize = 1;
    int16_t cursorX = 0, cursorY = 0;
    uint8_t contrast = 255;
    uint16_t rotation = 0;
    int logicalWidth = 240, logicalHeight = 240;
    int physicalWidth = 240, physicalHeight = 240;
    int xOffset = 0, yOffset = 0;
};

} // namespace pixelroot32::graphics
