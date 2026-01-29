#ifdef ESP32

#include <drivers/esp32/TFT_eSPI_Drawer.h>
#include <stdio.h>
#include <cstdarg>
#include <cstdio>

#ifndef IRAM_ATTR
#define IRAM_ATTR
#endif

namespace pr32 = pixelroot32;

// --------------------------------------------------
// Constructor / Destructor
// --------------------------------------------------

pr32::drivers::esp32::TFT_eSPI_Drawer::TFT_eSPI_Drawer()
    : tft()
    , spr(&tft) 
    , cursorX(0)
    , cursorY(0)    
    , textColor(TFT_WHITE)
    , textSize(1)
    , rotation(0)
{
}

pr32::drivers::esp32::TFT_eSPI_Drawer::~TFT_eSPI_Drawer() {
}

// --------------------------------------------------
// Init & configuration
// --------------------------------------------------

void pr32::drivers::esp32::TFT_eSPI_Drawer::init() {
    tft.init();
    tft.setRotation(0);
    tft.fillScreen(TFT_BLACK);

    spr.setColorDepth(8);
    spr.createSprite(displayWidth, displayHeight);
    
    spr.initDMA();
}

void pr32::drivers::esp32::TFT_eSPI_Drawer::setRotation(uint8_t rotation) {
    this->rotation = rotation;
    spr.setRotation(rotation);
}

// --------------------------------------------------
// Buffer control (no framebuffer in TFT_eSPI)
// --------------------------------------------------

void pr32::drivers::esp32::TFT_eSPI_Drawer::clearBuffer() { 
    spr.fillSprite(TFT_BLACK);
}

void pr32::drivers::esp32::TFT_eSPI_Drawer::sendBuffer() {
    spr.pushSprite(0, 0);
}

// --------------------------------------------------
// Primitive drawing
// --------------------------------------------------

void pr32::drivers::esp32::TFT_eSPI_Drawer::drawLine(int x1, int y1, int x2, int y2, uint16_t color) {
    spr.drawLine(x1, y1, x2, y2, color);
}

void pr32::drivers::esp32::TFT_eSPI_Drawer::drawRectangle(int x, int y, int width, int height, uint16_t color) {
    spr.drawRect(x, y, width, height, color);
}

void pr32::drivers::esp32::TFT_eSPI_Drawer::drawFilledRectangle(int x, int y, int width, int height, uint16_t color) {
    spr.fillRect(x, y, width, height, color);
}

void pr32::drivers::esp32::TFT_eSPI_Drawer::drawFilledCircle(int x, int y, int radius, uint16_t color) {
    spr.fillCircle(x, y, radius, color);
}

void pr32::drivers::esp32::TFT_eSPI_Drawer::drawCircle(int x, int y, int radius, uint16_t color) {
    spr.drawCircle(x, y, radius, color);
}

// --------------------------------------------------
// Bitmap 
// --------------------------------------------------

void pr32::drivers::esp32::TFT_eSPI_Drawer::drawBitmap(int x, int y, int width, int height, const uint8_t *bitmap , uint16_t color) {
    spr.drawBitmap(x, y, bitmap, width, height, color);
}

void IRAM_ATTR pr32::drivers::esp32::TFT_eSPI_Drawer::drawPixel(int x, int y, uint16_t color) {
    spr.drawPixel(x, y, color);
}

void pr32::drivers::esp32::TFT_eSPI_Drawer::setDisplaySize(int width, int height) {
    displayWidth = width;
    displayHeight = height;
}

// --------------------------------------------------
// Text handling
// --------------------------------------------------
void pr32::drivers::esp32::TFT_eSPI_Drawer::setTextColor(uint16_t color) {
    textColor = color;
    spr.setTextColor(textColor, TFT_BLACK);
}

void pr32::drivers::esp32::TFT_eSPI_Drawer::setTextSize(uint8_t size) {
    textSize = size;
    spr.setTextSize(textSize);
}

void pr32::drivers::esp32::TFT_eSPI_Drawer::setCursor(int16_t x, int16_t y) {
    cursorX = x;
    cursorY = y;
    spr.setCursor(cursorX, cursorY);
}

// @deprecated These methods are obsolete. Text rendering is now handled by Renderer
// using the native bitmap font system. These methods are kept as empty stubs
// only for interface compatibility (DrawSurface requires them).
// The Renderer never calls these methods - all text goes through the font system.
void pr32::drivers::esp32::TFT_eSPI_Drawer::drawText(const char* text, int16_t x, int16_t y, uint16_t color, uint8_t size) {
    // Obsolete: This method should never be called.
    // All text rendering is handled by Renderer::drawText() using the font system.
    (void)text; (void)x; (void)y; (void)color; (void)size;
}

void pr32::drivers::esp32::TFT_eSPI_Drawer::drawTextCentered(const char* text, int16_t y, uint16_t color, uint8_t size) {
    // Obsolete: This method should never be called.
    // All text rendering is handled by Renderer::drawTextCentered() using the font system.
    (void)text; (void)y; (void)color; (void)size;
}

// --------------------------------------------------
// Color helper
// --------------------------------------------------

uint16_t pr32::drivers::esp32::TFT_eSPI_Drawer::color565(uint8_t r, uint8_t g, uint8_t b) {
    return spr.color565(r, g, b);
}

bool pr32::drivers::esp32::TFT_eSPI_Drawer::processEvents() {
    tft.dmaWait();
    return true;
}

void pr32::drivers::esp32::TFT_eSPI_Drawer::present() {
    tft.pushImageDMA(
        0, 0,
        spr.width(),
        spr.height(),
        (uint16_t*)spr.getPointer()
    );

}

#endif // ESP32
