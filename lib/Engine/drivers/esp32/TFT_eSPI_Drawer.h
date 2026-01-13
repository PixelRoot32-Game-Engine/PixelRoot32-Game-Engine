
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

class TFT_eSPI_Drawer : public DrawSurface {
public:
    TFT_eSPI_Drawer();
    virtual ~TFT_eSPI_Drawer();

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

    void setContrast(uint8_t value) override {
        // ST7789 no soporta contraste real → noop
    }

    bool processEvents() override;

    void present() override;

private:
    TFT_eSPI tft;
    TFT_eSprite spr;
    int16_t cursorX, cursorY;
    uint16_t textColor;
    uint8_t textSize;
    uint8_t rotation;

    int displayWidth;
    int displayHeight;
};

#endif // ESP32

#endif // TFT_eSPI_DRAEWER_H