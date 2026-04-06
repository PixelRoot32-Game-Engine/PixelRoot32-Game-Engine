/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#pragma once

#include <cstdint>

namespace pixelroot32::graphics {

/**
 * @class DrawSurface
 * @brief Abstract interface for platform-specific drawing operations.
 *
 * This class defines the contract for any graphics driver (e.g., TFT_eSPI for ESP32,
 * SDL2 for Windows). It implements the Bridge pattern, allowing the Renderer to
 * remain platform-agnostic.
 */
class DrawSurface {
public:

    virtual ~DrawSurface() = default;

    /**
     * @brief Initializes the hardware or window.
     */
    virtual void init() = 0;

    /**
     * @brief Sets the display rotation.
     * @param rotation Rotation value. Can be index (0-3) or degrees (0, 90, 180, 270).
     */
    virtual void setRotation(uint16_t rotation) = 0;

    /**
     * @brief Clears the frame buffer (fills with black or background color).
     */
    virtual void clearBuffer() = 0;

    /**
     * @brief Sends the frame buffer to the physical display.
     */
    virtual void sendBuffer() = 0;

    // Drawing Primitives
    virtual void drawFilledCircle(int x, int y, int radius, uint16_t color) = 0;
    virtual void drawCircle(int x, int y, int radius, uint16_t color) = 0;
    virtual void drawRectangle(int x, int y, int width, int height, uint16_t color) = 0;
    virtual void drawFilledRectangle(int x, int y, int width, int height, uint16_t color) = 0;
    virtual void drawLine(int x1, int y1, int x2, int y2, uint16_t color) = 0;
    virtual void drawBitmap(int x, int y, int width, int height, const uint8_t *bitmap, uint16_t color) = 0;
    virtual void drawPixel(int x, int y, uint16_t color) = 0;

    /**
     * @brief Direct tile write to sprite buffer (optimized for tilemap rendering).
     * 
     * Default implementation returns without doing anything.
     * Override in drivers that support direct buffer access (e.g., TFT_eSPI).
     * 
     * @param x Tile X position in sprite coordinates
     * @param y Tile Y position in sprite coordinates
     * @param width Tile width in pixels
     * @param height Tile height in pixels
     * @param data Pointer to 8bpp tile data (one byte per pixel, index into palette)
     */
    virtual void drawTileDirect(uint16_t x, uint16_t y, uint16_t width, uint16_t height, const uint8_t* data) {
        (void)x; (void)y; (void)width; (void)height; (void)data;
    }

    /**
     * @brief Get pointer to sprite buffer for direct manipulation.
     * 
     * Default implementation returns nullptr.
     * Override in drivers that support direct buffer access.
     * 
     * @return Pointer to 8bpp sprite buffer, or nullptr if not supported
     */
    virtual uint8_t* getSpriteBuffer() {
        return nullptr;
    }

    /**
     * @brief Sets the display contrast/brightness.
     * @param level Contrast level (0-255).
     */
    virtual void setContrast(uint8_t level) = 0;

    // Text State Management
    virtual void setTextColor(uint16_t color) = 0;
    virtual void setTextSize(uint8_t size) = 0;
    virtual void setCursor(int16_t x, int16_t y) = 0;
    // virtual int16_t textWidth(const char* text) = 0;

    /**
     * @brief Converts RGB888 color to RGB565 format.
     * @param r Red component (0-255).
     * @param g Green component (0-255).
     * @param b Blue component (0-255).
     * @return 16-bit color value.
     */
    virtual uint16_t color565(uint8_t r, uint8_t g, uint8_t b) = 0;

    /**
     * @brief Sets the logical display size (rendering resolution).
     * @param w Width of the logical framebuffer.
     * @param h Height of the logical framebuffer.
     */
    virtual void setDisplaySize(int w, int h) = 0;

    /**
     * @brief Sets the physical display size (hardware resolution).
     * 
     * Used when the logical rendering resolution differs from the
     * physical display resolution. The driver will scale output
     * from logical to physical resolution.
     * 
     * @param w Physical display width.
     * @param h Physical display height.
     */
    virtual void setPhysicalSize(int w, int h) {
        // Default implementation: assume no scaling (physical = logical)
        // Override in drivers that support resolution scaling
        (void)w; (void)h;
    }

    /**
     * @brief Sets the display offset (positioning of the active area).
     * @param x X offset.
     * @param y Y offset.
     */
    virtual void setOffset(int x, int y) {
        (void)x; (void)y;
    }

    /**
     * @brief Processes platform events (e.g., SDL window events).
     * @return false if the application should quit, true otherwise.
     */
    virtual bool processEvents() { return true; }

    /**
     * @brief Swaps buffers (for double-buffered systems like SDL).
     */
    virtual void present() = 0;
};

}
