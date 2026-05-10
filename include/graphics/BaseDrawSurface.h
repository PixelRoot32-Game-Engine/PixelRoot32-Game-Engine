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
 * @brief Optional base class for DrawSurface implementations providing default primitive rendering.
 *
 * Inherits from DrawSurface.
 *
 * Users can inherit from this class to avoid implementing every single primitive.
 * At minimum, a subclass must implement:
 * - init() — initialize the hardware or window
 * - drawPixel() — draw a single pixel (all other primitives delegate here)
 * - sendBuffer() — transfer framebuffer to physical display
 * - clearBuffer() — fill framebuffer with background color
 *
 * The default implementations use Bresenham-style algorithms for lines,
 * rectangles, circles, and bitmaps, all building on drawPixel().
 * These are slow but correct; override in performance-critical drivers.
 */
class BaseDrawSurface : public DrawSurface {
public:
    virtual ~BaseDrawSurface() = default;

    /**
     * @brief Sets the text color.
     * @param color 16-bit RGB565 text color.
     */
    void setTextColor(uint16_t color) override { textColor = color; }

    /**
     * @brief Sets the text size multiplier.
     * @param size Size multiplier (1 = normal, 2 = double, etc.).
     */
    void setTextSize(uint8_t size) override { textSize = size; }

    /**
     * @brief Sets the text cursor position.
     * @param x X coordinate in pixels.
     * @param y Y coordinate in pixels.
     */
    void setCursor(int16_t x, int16_t y) override { cursorX = x; cursorY = y; }

    /**
     * @brief Sets the display brightness.
     * @param level Contrast level (0–255).
     */
    void setContrast(uint8_t level) override { contrast = level; }

    /**
     * @brief Sets the display rotation.
     * @param rot Rotation index (0, 1, 2, 3) or degrees (0, 90, 180, 270).
     */
    void setRotation(uint16_t rot) override { rotation = rot; }
    
    /**
     * @brief Sets the logical rendering resolution.
     * @param w Logical width in pixels.
     * @param h Logical height in pixels.
     */
    void setDisplaySize(int w, int h) override { logicalWidth = w; logicalHeight = h; }

    /**
     * @brief Sets the physical display resolution.
     * @param w Physical width in pixels.
     * @param h Physical height in pixels.
     */
    void setPhysicalSize(int w, int h) override { physicalWidth = w; physicalHeight = h; }

    /**
     * @brief Sets the display origin offset.
     * @param x Horizontal offset in pixels.
     * @param y Vertical offset in pixels.
     */
    void setOffset(int x, int y) override { xOffset = x; yOffset = y; }

    /**
     * @brief Transfers the framebuffer to the physical display.
     * 
     * Default implementation calls sendBuffer(). Override if you need
     * additional logic (e.g., double-buffered swap on SDL).
     */
    void present() override { sendBuffer(); }

    /**
     * @brief Converts 24-bit RGB to RGB565.
     * @param r Red component (0-255).
     * @param g Green component (0-255).
     * @param b Blue component (0-255).
     * @return 16-bit RGB565 color.
     */
    uint16_t color565(uint8_t r, uint8_t g, uint8_t b) override {
        return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
    }

    // Default primitive implementations using drawPixel (slow but functional)

    /**
     * @brief Draws a line between two points (Bresenham algorithm).
     * @param x1 Start X coordinate.
     * @param y1 Start Y coordinate.
     * @param x2 End X coordinate.
     * @param y2 End Y coordinate.
     * @param color 16-bit RGB565 line color.
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
     * @param w Width in pixels.
     * @param h Height in pixels.
     * @param color 16-bit RGB565 outline color.
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
     * @param w Width in pixels.
     * @param h Height in pixels.
     * @param color 16-bit RGB565 fill color.
     */
    void drawFilledRectangle(int x, int y, int w, int h, uint16_t color) override {
        for (int j = 0; j < h; j++) {
            for (int i = 0; i < w; i++) {
                drawPixel(x + i, y + j, color);
            }
        }
    }

    /**
     * @brief Draws a circle outline (midpoint algorithm).
     * @param x0 Center X coordinate.
     * @param y0 Center Y coordinate.
     * @param r Radius in pixels.
     * @param color 16-bit RGB565 outline color.
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
     * @brief Draws a filled circle (midpoint algorithm).
     * @param x0 Center X coordinate.
     * @param y0 Center Y coordinate.
     * @param r Radius in pixels.
     * @param color 16-bit RGB565 fill color.
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
     * @brief Draws a 1bpp bitmap, rendering only non-zero pixels.
     * @param x Top-left X coordinate.
     * @param y Top-left Y coordinate.
     * @param w Width in pixels.
     * @param h Height in pixels.
     * @param bitmap Pointer to packed 1bpp row data (byte per row, MSB left).
     * @param color 16-bit RGB565 color for "on" pixels.
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
    uint16_t textColor = 0xFFFF;     ///< Current 16-bit text color (RGB565).
    uint8_t textSize = 1;           ///< Text size multiplier (1 = normal, 2 = double, etc.).
    int16_t cursorX = 0;            ///< Text cursor X position in pixels.
    int16_t cursorY = 0;            ///< Text cursor Y position in pixels.
    uint8_t contrast = 255;         ///< Display brightness (0-255).
    uint16_t rotation = 0;          ///< Display rotation (0, 1, 2, 3 for 0°, 90°, 180°, 270°).
    int logicalWidth = 240;         ///< Logical rendering width in pixels.
    int logicalHeight = 240;         ///< Logical rendering height in pixels.
    int physicalWidth = 240;        ///< Physical display width in pixels.
    int physicalHeight = 240;       ///< Physical display height in pixels.
    int xOffset = 0;                 ///< Horizontal offset for display positioning.
    int yOffset = 0;                 ///< Vertical offset for display positioning.
};

} // namespace pixelroot32::graphics
