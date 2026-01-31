#ifdef ESP32

#include <drivers/esp32/TFT_eSPI_Drawer.h>
#include <stdio.h>
#include <cstdarg>
#include <cstdio>

#ifndef IRAM_ATTR
#define IRAM_ATTR
#endif

#ifdef PIXELROOT32_ENABLE_PROFILING
#include <Arduino.h>
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
    freeScalingBuffers();
}

// --------------------------------------------------
// Init & configuration
// --------------------------------------------------

void pr32::drivers::esp32::TFT_eSPI_Drawer::init() {
    Serial.println("[TFT_eSPI_Drawer] Initializing TFT...");
    tft.init();
    tft.setRotation(rotation);
    tft.fillScreen(TFT_BLACK);

    Serial.println("[TFT_eSPI_Drawer] Initializing DMA...");
    // Initialize DMA for the TFT. 
    // We call it with 'false' to indicate we don't want to re-initialize the bus if possible,
    // but TFT_eSPI on ESP32 usually needs this to setup DMA descriptors.
    tft.initDMA();

    Serial.println("[TFT_eSPI_Drawer] Creating Sprite...");
    // Create sprite with LOGICAL resolution (smaller = less memory)
    spr.setColorDepth(8);
    if (!spr.createSprite(logicalWidth, logicalHeight)) {
        Serial.printf("[ERROR] Failed to create sprite of size %dx%d\n", logicalWidth, logicalHeight);
    }
    
    // Build scaling lookup tables and palette conversion buffers
    buildScaleLUTs();
    Serial.println("[TFT_eSPI_Drawer] Initialization complete.");
}

void pr32::drivers::esp32::TFT_eSPI_Drawer::setRotation(uint16_t rot) {
    // Standardize rotation to index 0-3 (0, 90, 180, 270)
    if (rot == 90) rotation = 1;
    else if (rot == 180) rotation = 2;
    else if (rot == 270) rotation = 3;
    else if (rot >= 360) rotation = (rot / 90) % 4;
    else rotation = rot % 4;
    
    #ifdef PIXELROOT32_ENABLE_PROFILING
    Serial.printf("[TFT_eSPI_Drawer] Rotation set to %d (%d degrees)\n", rotation, rotation * 90);
    #endif

    if (tft.getRotation() != rotation) {
        tft.setRotation(rotation);
    }
}

// --------------------------------------------------
// Buffer control (no framebuffer in TFT_eSPI)
// --------------------------------------------------

void pr32::drivers::esp32::TFT_eSPI_Drawer::clearBuffer() { 
    spr.fillSprite(TFT_BLACK);
}

void pr32::drivers::esp32::TFT_eSPI_Drawer::sendBuffer() {
    sendBufferScaled();
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
    logicalWidth = width;
    logicalHeight = height;
}

void pr32::drivers::esp32::TFT_eSPI_Drawer::setPhysicalSize(int w, int h) {
    physicalWidth = w;
    physicalHeight = h;
}

// --------------------------------------------------
// Scaling Functions
// --------------------------------------------------

void pr32::drivers::esp32::TFT_eSPI_Drawer::buildScaleLUTs() {
    freeScalingBuffers();
    
    // We will use blocks of 10 lines to reduce DMA overhead
    const int linesPerBlock = 10;
    size_t blockSize = physicalWidth * linesPerBlock * sizeof(uint16_t);

    // Allocate double line buffers for DMA
#ifdef ESP32
    lineBuffer[0] = (uint16_t*)heap_caps_malloc(blockSize, MALLOC_CAP_DMA | MALLOC_CAP_8BIT);
    lineBuffer[1] = (uint16_t*)heap_caps_malloc(blockSize, MALLOC_CAP_DMA | MALLOC_CAP_8BIT);
    paletteLUT = (uint16_t*)heap_caps_malloc(256 * sizeof(uint16_t), MALLOC_CAP_8BIT);

    if (!lineBuffer[0] || !lineBuffer[1] || !paletteLUT) {
        Serial.println("[ERROR] Failed to allocate DMA or Palette buffers!");
    }
#else
    lineBuffer[0] = new uint16_t[physicalWidth * linesPerBlock];
    lineBuffer[1] = new uint16_t[physicalWidth * linesPerBlock];
    paletteLUT = new uint16_t[256];
#endif
    
    // Pre-calculate palette LUT (8bpp -> 16bpp)
    for (int i = 0; i < 256; ++i) {
        uint16_t color16 = spr.color8to16(i);
        // Swap bytes because pushPixelsDMA expects big-endian (TFT order)
        paletteLUT[i] = (color16 >> 8) | (color16 << 8);
    }

    // Allocate and build X lookup table
    xLUT = new uint16_t[physicalWidth];
    for (int i = 0; i < physicalWidth; ++i) {
        xLUT[i] = (i * logicalWidth) / physicalWidth;
    }
    
    // Allocate and build Y lookup table
    yLUT = new uint16_t[physicalHeight];
    for (int i = 0; i < physicalHeight; ++i) {
        yLUT[i] = (i * logicalHeight) / physicalHeight;
    }
}

void pr32::drivers::esp32::TFT_eSPI_Drawer::freeScalingBuffers() {
    for (int i = 0; i < 2; ++i) {
        if (lineBuffer[i]) {
#ifdef ESP32
            heap_caps_free(lineBuffer[i]);
#else
            delete[] lineBuffer[i];
#endif
            lineBuffer[i] = nullptr;
        }
    }
    if (paletteLUT) {
#ifdef ESP32
        heap_caps_free(paletteLUT);
#else
        delete[] paletteLUT;
#endif
        paletteLUT = nullptr;
    }
    if (xLUT) {
        delete[] xLUT;
        xLUT = nullptr;
    }
    if (yLUT) {
        delete[] yLUT;
        yLUT = nullptr;
    }
}

void IRAM_ATTR pr32::drivers::esp32::TFT_eSPI_Drawer::sendBufferScaled() {
#ifdef PIXELROOT32_ENABLE_PROFILING
    uint32_t start = micros();
#endif

    uint8_t* spritePtr = (uint8_t*)spr.getPointer();
    if (!spritePtr) return;

    tft.startWrite();
    tft.setAddrWindow(0, 0, physicalWidth, physicalHeight);
    
    const int linesPerBlock = 10;
    currentBuffer = 0;

    for (int startY = 0; startY < physicalHeight; startY += linesPerBlock) {
        int endY = startY + linesPerBlock;
        if (endY > physicalHeight) endY = physicalHeight;
        int numLines = endY - startY;

        // 1. Wait for previous DMA block to finish
        tft.dmaWait();

        // 2. Scale a block of lines into the current buffer
        uint16_t* dst = lineBuffer[currentBuffer];
        for (int physY = startY; physY < endY; ++physY) {
            int srcY = yLUT[physY];
            scaleLine(srcY, dst);
            dst += physicalWidth;
        }
        
        // 3. Start DMA for the WHOLE BLOCK
        tft.pushPixelsDMA(lineBuffer[currentBuffer], physicalWidth * numLines);

        // 4. Swap buffers for next block
        currentBuffer = 1 - currentBuffer;
    }
    
    tft.dmaWait();
    tft.endWrite();

#ifdef PIXELROOT32_ENABLE_PROFILING
    uint32_t elapsed = micros() - start;
    static uint32_t lastReport = 0;
    if (millis() - lastReport > 1000) {
        Serial.printf("[PROFILING] Scaled DMA Transfer: %u us (%u FPS max)\n", elapsed, 1000000 / (elapsed > 0 ? elapsed : 1));
        lastReport = millis();
    }
#endif
}

void IRAM_ATTR pr32::drivers::esp32::TFT_eSPI_Drawer::scaleLine(int srcY, uint16_t* dst) {
    // Get pointer to source row in 8bpp sprite
    uint8_t* spritePtr = (uint8_t*)spr.getPointer();
    uint8_t* srcRow = spritePtr + (srcY * logicalWidth);
    
    // Use local pointers to help optimization
    const uint16_t* __restrict pLUT = paletteLUT;
    const uint16_t* __restrict xL = xLUT;
    
    // Use LUT for X scaling and Palette LUT for color conversion
    for (int physX = 0; physX < physicalWidth; ++physX) {
        dst[physX] = pLUT[srcRow[xL[physX]]];
    }
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
    return true;
}

void pr32::drivers::esp32::TFT_eSPI_Drawer::present() {
    sendBufferScaled();
}

#endif // ESP32
