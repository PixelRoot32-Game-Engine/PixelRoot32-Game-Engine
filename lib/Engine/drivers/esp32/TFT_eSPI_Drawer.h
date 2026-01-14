
#pragma once
#ifndef TFT_eSPI_DRAEWER_H
#define TFT_eSPI_DRAEWER_H

#ifdef ESP32

#include "graphics/DrawSurface.h"
// TFT_eSPI specific includes would go here
#include "Config.h"
// ESP32 platform: use TFT_eSPI
// Force TFT_eSPI configuration before including the library
#ifndef ST7789_DRIVER
#define ST7789_DRIVER
#endif
#ifndef TFT_WIDTH
#define TFT_WIDTH 240
#endif
#ifndef TFT_HEIGHT
#define TFT_HEIGHT 240
#endif
#ifndef TFT_MOSI
#define TFT_MOSI 23
#endif
#ifndef TFT_SCLK
#define TFT_SCLK 18
#endif
#ifndef TFT_DC
#define TFT_DC 2
#endif
#ifndef TFT_RST
#define TFT_RST 4
#endif
#ifndef TFT_CS
#define TFT_CS -1
#endif
#ifndef SPI_FREQUENCY
#define SPI_FREQUENCY 40000000
#endif
#include <TFT_eSPI.h>
#include <stdint.h>

/**
 * @class TFT_eSPI_Drawer
 * @brief Concrete implementation of DrawSurface for ESP32 using the TFT_eSPI library.
 *
 * This class handles low-level interaction with the display hardware via SPI.
 * It uses a sprite (framebuffer) to minimize flickering and tearing.
 */
class TFT_eSPI_Drawer : public DrawSurface {
public:
    TFT_eSPI_Drawer();
    virtual ~TFT_eSPI_Drawer();

    /**
     * @brief Initializes the TFT_eSPI library and the sprite buffer.
     * Sets up the SPI communication and allocates memory for the framebuffer.
     */
    void init() override;

    /**
     * @brief Sets the screen rotation.
     * @param rotation 0-3 corresponding to 0, 90, 180, 270 degrees.
     */
    void setRotation(uint8_t rotation) override;

    /**
     * @brief Fills the sprite buffer with black color.
     */
    void clearBuffer() override;

    /**
     * @brief Pushes the sprite buffer to the physical display.
     * This is the "flip" operation in double buffering.
     */
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

    /**
     * @brief Sets contrast. No-op for ST7789 usually.
     */
    void setContrast(uint8_t value) override {
        // ST7789 does not support real contrast control via this API -> noop
    }

    /**
     * @brief Processes system events. Always true for embedded.
     */
    bool processEvents() override;

    /**
     * @brief Present buffer. Calls sendBuffer().
     */
    void present() override;

private:
    TFT_eSPI tft;   ///< The underlying TFT_eSPI driver instance.
    TFT_eSprite spr; ///< The sprite used as a framebuffer.
    int16_t cursorX, cursorY;
    uint16_t textColor;
    uint8_t textSize;
    uint8_t rotation;

    int displayWidth;
    int displayHeight;
};

#endif // ESP32

#endif // TFT_eSPI_DRAEWER_H