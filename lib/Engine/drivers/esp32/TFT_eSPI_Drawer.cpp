#ifdef ESP32

#include "TFT_eSPI_Drawer.h"
#include <stdio.h>
#include <cstdarg>
#include <cstdio>

namespace pixelroot32::drivers::esp32 {

// --------------------------------------------------
// Constructor / Destructor
// --------------------------------------------------

TFT_eSPI_Drawer::TFT_eSPI_Drawer()
    : tft()
    , spr(&tft) 
    , cursorX(0)
    , cursorY(0)    
    , textColor(TFT_WHITE)
    , textSize(1)
    , rotation(0)
{
}

TFT_eSPI_Drawer::~TFT_eSPI_Drawer() {
}

// --------------------------------------------------
// Init & configuration
// --------------------------------------------------

void TFT_eSPI_Drawer::init() {
    tft.init();
    tft.setRotation(0);
    tft.fillScreen(TFT_BLACK);

    spr.setColorDepth(8);
    spr.createSprite(displayWidth, displayHeight);

    spr.setTextFont(1);
    
    spr.initDMA();
}

void TFT_eSPI_Drawer::setRotation(uint8_t rotation) {
    this->rotation = rotation;
    spr.setRotation(rotation);
}

// --------------------------------------------------
// Buffer control (no framebuffer in TFT_eSPI)
// --------------------------------------------------

void TFT_eSPI_Drawer::clearBuffer() {
    spr.fillSprite(TFT_BLACK);
}

void TFT_eSPI_Drawer::sendBuffer() {
    spr.pushSprite(0, 0);
}

// --------------------------------------------------
// Primitive drawing
// --------------------------------------------------

void TFT_eSPI_Drawer::drawLine(int x1, int y1, int x2, int y2, uint16_t color) {
    spr.drawLine(x1, y1, x2, y2, color);
}

void TFT_eSPI_Drawer::drawRectangle(int x, int y, int width, int height, uint16_t color) {
    spr.drawRect(x, y, width, height, color);
}

void TFT_eSPI_Drawer::drawFilledRectangle(int x, int y, int width, int height, uint16_t color) {
    spr.fillRect(x, y, width, height, color);
}

void TFT_eSPI_Drawer::drawFilledCircle(int x, int y, int radius, uint16_t color) {
    spr.fillCircle(x, y, radius, color);
}

void TFT_eSPI_Drawer::drawCircle(int x, int y, int radius, uint16_t color) {
    spr.drawCircle(x, y, radius, color);
}

// --------------------------------------------------
// Bitmap 
// --------------------------------------------------

void TFT_eSPI_Drawer::drawBitmap(int x, int y, int width, int height, const uint8_t *bitmap , uint16_t color) {
    spr.drawBitmap(x, y, bitmap, width, height, color);
}

void TFT_eSPI_Drawer::drawPixel(int x, int y, uint16_t color) {
    spr.drawPixel(x, y, color);
}

void TFT_eSPI_Drawer::setDisplaySize(int width, int height) {
    displayWidth = width;
    displayHeight = height;
}

// --------------------------------------------------
// Text handling
// --------------------------------------------------

void TFT_eSPI_Drawer::setTextColor(uint16_t color) {
    textColor = color;
    spr.setTextColor(textColor, TFT_BLACK);
}

void TFT_eSPI_Drawer::setTextSize(uint8_t size) {
    textSize = size;
    spr.setTextSize(textSize);
}

void TFT_eSPI_Drawer::setCursor(int16_t x, int16_t y) {
    cursorX = x;
    cursorY = y;
    spr.setCursor(cursorX, cursorY);
}

void TFT_eSPI_Drawer::drawText(const char* text, int16_t x, int16_t y, uint16_t color, uint8_t size) {
    
    spr.setTextSize(size);
    spr.setTextColor(color, TFT_BLACK);
    spr.setCursor(x, y);
    
    // Si el puntero del texto es nulo, evitar imprimir
    if (text) {
        spr.print(text);
    }
}

void TFT_eSPI_Drawer::drawTextCentered(const char* text, int16_t y, uint16_t color, uint8_t size) {
    if (!text) return;

    spr.setTextSize(size);
    spr.setTextColor(color, TFT_BLACK);

    int16_t tw = spr.textWidth(text); 
    int16_t x = (displayWidth - tw) / 2;

    spr.setCursor(x, y);
    spr.print(text);
}

// --------------------------------------------------
// Color helper
// --------------------------------------------------

uint16_t TFT_eSPI_Drawer::color565(uint8_t r, uint8_t g, uint8_t b) {
    return spr.color565(r, g, b);
}

bool TFT_eSPI_Drawer::processEvents() {
    tft.dmaWait();
    return true;
}

void TFT_eSPI_Drawer::present() {
    tft.pushImageDMA(
        0, 0,
        spr.width(),
        spr.height(),
        (uint16_t*)spr.getPointer()
    );
}

} // namespace pixelroot32::drivers::esp32

#endif // ESP32
