#pragma once

#include <cstdint>

/**
 * Abstract interface for drawer operations
 * Allows different implementations for ESP32 (TFT_eSPI) and Windows (SDL2)
 */
class DrawSurface {
public:

    virtual ~DrawSurface() = default;

    // Initialization
    virtual void init() = 0;
    virtual void setRotation(uint8_t rotation) = 0;

    // Buffer management
    virtual void clearBuffer() = 0;
    virtual void sendBuffer() = 0;

    // Drawing
    virtual void drawText(const char* text, int16_t x, int16_t y, uint16_t color, uint8_t size) = 0;
    virtual void drawTextCentered(const char* text, int16_t y, uint16_t color, uint8_t size) = 0;
    virtual void drawFilledCircle(int x, int y, int radius, uint16_t color) = 0;
    virtual void drawCircle(int x, int y, int radius, uint16_t color) = 0;
    virtual void drawRectangle(int x, int y, int width, int height, uint16_t color) = 0;
    virtual void drawFilledRectangle(int x, int y, int width, int height, uint16_t color) = 0;
    virtual void drawLine(int x1, int y1, int x2, int y2, uint16_t color) = 0;
    virtual void drawBitmap(int x, int y, int width, int height, const uint8_t *bitmap, uint16_t color) = 0;
    virtual void drawPixel(int x, int y, uint16_t color) = 0;

    // Contrast control
    virtual void setContrast(uint8_t level) = 0;

    // Text operations
    virtual void setTextColor(uint16_t color) = 0;
    virtual void setTextSize(uint8_t size) = 0;
    virtual void setCursor(int16_t x, int16_t y) = 0;
    // virtual int16_t textWidth(const char* text) = 0;

    // Color conversion (RGB565)
    virtual uint16_t color565(uint8_t r, uint8_t g, uint8_t b) = 0;

    virtual void setDisplaySize(int w, int h) = 0;

    // Present/swap buffers (for SDL2, no-op for TFT_eSPI)
    virtual bool processEvents() { return true; }
    virtual void present() = 0;
};