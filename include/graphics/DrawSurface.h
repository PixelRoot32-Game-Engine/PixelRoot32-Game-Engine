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
     * @param rotation Rotation index (0-3).
     */
    virtual void setRotation(uint8_t rotation) = 0;

    /**
     * @brief Clears the frame buffer (fills with black or background color).
     */
    virtual void clearBuffer() = 0;

    /**
     * @brief Sends the frame buffer to the physical display.
     */
    virtual void sendBuffer() = 0;

    // Drawing Primitives
    /**
     * @deprecated Text rendering is now handled by Renderer using the native bitmap font system.
     * This method is kept for interface compatibility but should never be called.
     * All text rendering goes through Renderer::drawText() which uses the font system.
     */
    virtual void drawText(const char* text, int16_t x, int16_t y, uint16_t color, uint8_t size) = 0;
    
    /**
     * @deprecated Text rendering is now handled by Renderer using the native bitmap font system.
     * This method is kept for interface compatibility but should never be called.
     * All text rendering goes through Renderer::drawTextCentered() which uses the font system.
     */
    virtual void drawTextCentered(const char* text, int16_t y, uint16_t color, uint8_t size) = 0;
    virtual void drawFilledCircle(int x, int y, int radius, uint16_t color) = 0;
    virtual void drawCircle(int x, int y, int radius, uint16_t color) = 0;
    virtual void drawRectangle(int x, int y, int width, int height, uint16_t color) = 0;
    virtual void drawFilledRectangle(int x, int y, int width, int height, uint16_t color) = 0;
    virtual void drawLine(int x1, int y1, int x2, int y2, uint16_t color) = 0;
    virtual void drawBitmap(int x, int y, int width, int height, const uint8_t *bitmap, uint16_t color) = 0;
    virtual void drawPixel(int x, int y, uint16_t color) = 0;

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
     * @brief Sets the logical display size.
     * @param w Width.
     * @param h Height.
     */
    virtual void setDisplaySize(int w, int h) = 0;

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