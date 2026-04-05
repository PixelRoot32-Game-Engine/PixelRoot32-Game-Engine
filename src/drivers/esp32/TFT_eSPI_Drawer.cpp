#include "drivers/esp32/TFT_eSPI_Drawer.h"
#include "platforms/EngineConfig.h"
#include "core/Log.h"

#if defined(PIXELROOT32_USE_TFT_ESPI_DRIVER)

#include "drivers/esp32/TFT_eSPI_TouchBridge.h"
#include <stdio.h>
#include <cstdarg>
#include <cstdio>
#include <cstring>

#ifndef IRAM_ATTR
#define IRAM_ATTR
#endif

#ifdef PIXELROOT32_ENABLE_PROFILING
#include <Arduino.h>
#endif

namespace pr32 = pixelroot32;
namespace logging = pixelroot32::core::logging;

using logging::log;
using logging::LogLevel;


// --------------------------------------------------
// Constructor / Destructor
// --------------------------------------------------

pr32::drivers::esp32::TFT_eSPI_Drawer::TFT_eSPI_Drawer()
    : tft()
    , spr(&tft) 
{
}

pr32::drivers::esp32::TFT_eSPI_Drawer::~TFT_eSPI_Drawer() {
    freeScalingBuffers();
}

// --------------------------------------------------
// Init & configuration
// --------------------------------------------------

void pr32::drivers::esp32::TFT_eSPI_Drawer::init() {
    log("[TFT_eSPI_Drawer] Initializing TFT...");
    tft.init();
    tft.setRotation(rotation);
    tft.fillScreen(TFT_BLACK);

    log("[TFT_eSPI_Drawer] Initializing DMA...");
    // Initialize DMA for the TFT. 
    // We call it with 'false' to indicate we don't want to re-initialize the bus if possible,
    // but TFT_eSPI on ESP32 usually needs this to setup DMA descriptors.
    tft.initDMA();

    log("[TFT_eSPI_Drawer] Creating Sprite...");
    // Create sprite with LOGICAL resolution (smaller = less memory)
    spr.setColorDepth(8);
    if (!spr.createSprite(logicalWidth, logicalHeight)) {
        log(LogLevel::Error, "Failed to create sprite of size %dx%d", logicalWidth, logicalHeight);
    }
    
    // Build scaling lookup tables and palette conversion buffers
    buildScaleLUTs();
    log("[TFT_eSPI_Drawer] Initialization complete.");

    pixelroot32::drivers::esp32::registerTftForXpt2046Touch(&tft);
}

void pr32::drivers::esp32::TFT_eSPI_Drawer::setRotation(uint16_t rot) {
    // Standardize rotation to index 0-3 (0, 90, 180, 270)
    if (rot == 90) rotation = 1;
    else if (rot == 180) rotation = 2;
    else if (rot == 270) rotation = 3;
    else if (rot >= 360) rotation = (rot / 90) % 4;
    else rotation = rot % 4;
    
    if constexpr (pixelroot32::platforms::config::EnableProfiling) {
        log("[TFT_eSPI_Drawer] Rotation set to %d (%d degrees)", rotation, rotation * 90);
    }

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

void IRAM_ATTR pr32::drivers::esp32::TFT_eSPI_Drawer::drawTileDirect(uint16_t x, uint16_t y, uint16_t width, uint16_t height, const uint8_t* data) {
    if (!data || x >= (uint16_t)logicalWidth || y >= (uint16_t)logicalHeight) {
        return;
    }
    
    // Get direct pointer to sprite buffer (8bpp)
    uint8_t* buffer = (uint8_t*)spr.getPointer();
    if (!buffer) {
        return;
    }
    
    // Clip to sprite bounds
    uint16_t clippedW = width;
    uint16_t clippedH = height;
    
    if (x + width > (uint16_t)logicalWidth) {
        clippedW = logicalWidth - x;
    }
    if (y + height > (uint16_t)logicalHeight) {
        clippedH = logicalHeight - y;
    }
    
    // Copy tile data directly to sprite buffer (fast memcpy)
    for (uint16_t row = 0; row < clippedH; row++) {
        uint16_t destOffset = (y + row) * logicalWidth + x;
        std::memcpy(&buffer[destOffset], data + row * width, clippedW);
    }
}

uint8_t* pr32::drivers::esp32::TFT_eSPI_Drawer::getSpriteBuffer() {
    return (uint8_t*)spr.getPointer();
}

// --------------------------------------------------
// Scaling Functions
// --------------------------------------------------

void pr32::drivers::esp32::TFT_eSPI_Drawer::buildScaleLUTs() {
    freeScalingBuffers();
    
    // Determine actual lines per block - try optimal first, fallback if IRAM constrained
    int linesPerBlock = LINES_PER_BLOCK;
    size_t blockSize = physicalWidth * linesPerBlock * sizeof(uint16_t);
    
    // Allocate double line buffers for DMA
#ifdef ESP32
    lineBuffer[0] = (uint16_t*)heap_caps_malloc(blockSize, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    
    // If first buffer succeeded but second failed, try fallback size
    if (lineBuffer[0] && !lineBuffer[1]) {
        heap_caps_free(lineBuffer[0]);
        
        linesPerBlock = LINES_PER_BLOCK_FALLBACK;
        blockSize = physicalWidth * linesPerBlock * sizeof(uint16_t);
        
        lineBuffer[0] = (uint16_t*)heap_caps_malloc(blockSize, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    }
    
    lineBuffer[1] = (uint16_t*)heap_caps_malloc(blockSize, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    
    // Force LUTs to Internal RAM for speed
    paletteLUT = (uint16_t*)heap_caps_malloc(256 * sizeof(uint16_t), MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    xLUT = (uint16_t*)heap_caps_malloc(physicalWidth * sizeof(uint16_t), MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    yLUT = (uint16_t*)heap_caps_malloc(physicalHeight * sizeof(uint16_t), MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);

    if (!lineBuffer[0] || !lineBuffer[1] || !paletteLUT || !xLUT || !yLUT) {
        log(LogLevel::Error, "Failed to allocate DMA or Palette buffers in Internal RAM!");
    } else {
        activeLinesPerBlock = linesPerBlock;
    }
#else
    lineBuffer[0] = new uint16_t[physicalWidth * linesPerBlock];
    lineBuffer[1] = new uint16_t[physicalWidth * linesPerBlock];
    paletteLUT = new uint16_t[256];
    xLUT = new uint16_t[physicalWidth];
    yLUT = new uint16_t[physicalHeight];
#endif
    
    // Pre-calculate palette LUT (8bpp -> 16bpp)
    // We store the colors in NATIVE endianness to avoid swapping in the inner loop
    // But pushPixelsDMA expects BIG endian (or whatever the display needs)
    // The ESP32 is Little Endian. The display is Big Endian usually.
    // TFT_eSPI handles this by swapping bytes in pushPixels usually, BUT
    // pushPixelsDMA with raw buffer might just dump memory.
    //
    // Let's assume we want to store the PRE-SWAPPED value in the LUT
    // so the CPU loop does strictly: dst[i] = LUT[src[i]]
    for (int i = 0; i < 256; ++i) {
        uint16_t color16 = spr.color8to16(i);
        // Swap bytes because pushPixelsDMA expects big-endian (TFT order)
        // Check if SPI_FREQUENCY is high, maybe we need to be careful?
        paletteLUT[i] = (color16 >> 8) | (color16 << 8);
    }

    // Build X lookup table
    for (int i = 0; i < physicalWidth; ++i) {
        xLUT[i] = (i * logicalWidth) / physicalWidth;
    }
    
    // Build Y lookup table
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
#ifdef ESP32
        heap_caps_free(xLUT);
#else
        delete[] xLUT;
#endif
        xLUT = nullptr;
    }
    if (yLUT) {
#ifdef ESP32
        heap_caps_free(yLUT);
#else
        delete[] yLUT;
#endif
        yLUT = nullptr;
    }
}

void IRAM_ATTR pr32::drivers::esp32::TFT_eSPI_Drawer::sendBufferScaled() {
    uint32_t start = 0;
    if constexpr (pixelroot32::platforms::config::EnableProfiling) {
        uint32_t start = micros();
    }

    uint8_t* spritePtr = (uint8_t*)spr.getPointer();
    if (!spritePtr) {
        return;
    }

    tft.startWrite();
    tft.setAddrWindow(xOffset, yOffset, physicalWidth, physicalHeight);
    
    currentBuffer = 0;
    int startY = 0;

    // ---------------------------------------------------------
    // STAGE 1: Pre-fill (First Block)
    // ---------------------------------------------------------
    bool is2x = (physicalWidth == logicalWidth * 2 && physicalHeight == logicalHeight * 2);

    if (startY < physicalHeight) {
        int endY = startY + activeLinesPerBlock;
        if (endY > physicalHeight) endY = physicalHeight;
        int numLines = endY - startY;

        // Scale block 0
        uint16_t* dst = lineBuffer[currentBuffer];

        if (!needsScaling()) {
             // 1:1 Optimization for the first block
             for (int i = 0; i < numLines; ++i) {
                 uint8_t* srcRow = (uint8_t*)spr.getPointer() + ((startY + i) * logicalWidth);
                 const uint16_t* __restrict pLUT = paletteLUT;
                 int x = 0;
                 uint32_t* dst32 = (uint32_t*)dst;
                 
                 for (; x <= physicalWidth - 8; x += 8) {
                     uint32_t p01 = ((uint32_t)pLUT[srcRow[x+1]] << 16) | pLUT[srcRow[x]];
                     uint32_t p23 = ((uint32_t)pLUT[srcRow[x+3]] << 16) | pLUT[srcRow[x+2]];
                     uint32_t p45 = ((uint32_t)pLUT[srcRow[x+5]] << 16) | pLUT[srcRow[x+4]];
                     uint32_t p67 = ((uint32_t)pLUT[srcRow[x+7]] << 16) | pLUT[srcRow[x+6]];

                     dst32[x/2]     = p01;
                     dst32[x/2 + 1] = p23;
                     dst32[x/2 + 2] = p45;
                     dst32[x/2 + 3] = p67;
                 }
                 for (; x < physicalWidth; ++x) {
                     dst[x] = pLUT[srcRow[x]];
                 }
                 dst += physicalWidth;
             }
        } else if (is2x) {
            // 2x Fast-Path: Duplicate pixels and rows using 32-bit writes
            for (int physY = startY; physY < endY; physY += 2) {
                int srcY = physY / 2;
                uint8_t* srcRow = (uint8_t*)spr.getPointer() + (srcY * logicalWidth);
                const uint16_t* __restrict pLUT = paletteLUT;
                uint32_t* dst32 = (uint32_t*)dst;

                for (int lx = 0; lx < logicalWidth; ++lx) {
                    uint16_t color = pLUT[srcRow[lx]];
                    dst32[lx] = (color << 16) | color; // Store two identical pixels
                }
                // Duplicate this line for the next physical row
                std::memcpy(dst + physicalWidth, dst, physicalWidth * sizeof(uint16_t));
                dst += physicalWidth * 2;
            }
        } else {
            // Normal path with scaling
            for (int physY = startY; physY < endY; ++physY) {
                int srcY = yLUT[physY];
                scaleLine(srcY, dst);
                dst += physicalWidth;
            }
        }

        // Start DMA transfer of block 0
        tft.pushPixelsDMA(lineBuffer[currentBuffer], physicalWidth * numLines);

        // Prepare indices for the next one
        currentBuffer = 1 - currentBuffer; // Switch to the other buffer
        startY += activeLinesPerBlock;
    }

    // ---------------------------------------------------------
    // STAGE 2: Pipeline (Main Loop)
    // While DMA sends the PREVIOUS buffer, CPU calculates the CURRENT one
    // ---------------------------------------------------------
    while (startY < physicalHeight) {
        int endY = startY + activeLinesPerBlock;
        if (endY > physicalHeight) endY = physicalHeight;
        int numLines = endY - startY;

        // 2. CPU calculates the next block in the free buffer
        // (SPI hardware is busy sending the opposite buffer in the background)
        uint16_t* dst = lineBuffer[currentBuffer];
        
        // Optimization for 1:1 case (No scaling)
        // We avoid xLUT and yLUT indirection for maximum speed
        if (!needsScaling()) {
             // Process block of lines directly
             for (int i = 0; i < numLines; ++i) {
                 uint8_t* srcRow = (uint8_t*)spr.getPointer() + ((startY + i) * logicalWidth);
                 const uint16_t* __restrict pLUT = paletteLUT;
                 int x = 0;
                 uint32_t* dst32 = (uint32_t*)dst;
                 
                 for (; x <= physicalWidth - 8; x += 8) {
                     uint32_t p01 = ((uint32_t)pLUT[srcRow[x+1]] << 16) | pLUT[srcRow[x]];
                     uint32_t p23 = ((uint32_t)pLUT[srcRow[x+3]] << 16) | pLUT[srcRow[x+2]];
                     uint32_t p45 = ((uint32_t)pLUT[srcRow[x+5]] << 16) | pLUT[srcRow[x+4]];
                     uint32_t p67 = ((uint32_t)pLUT[srcRow[x+7]] << 16) | pLUT[srcRow[x+6]];

                     dst32[x/2]     = p01;
                     dst32[x/2 + 1] = p23;
                     dst32[x/2 + 2] = p45;
                     dst32[x/2 + 3] = p67;
                 }
                 for (; x < physicalWidth; ++x) {
                     dst[x] = pLUT[srcRow[x]];
                 }
                 dst += physicalWidth;
             }
        } else if (is2x) {
            // 2x Fast-Path for the main loop blocks
            for (int physY = startY; physY < endY; physY += 2) {
                int srcY = physY / 2;
                uint8_t* srcRow = (uint8_t*)spr.getPointer() + (srcY * logicalWidth);
                const uint16_t* __restrict pLUT = paletteLUT;
                uint32_t* dst32 = (uint32_t*)dst;

                for (int lx = 0; lx < logicalWidth; ++lx) {
                    uint16_t color = pLUT[srcRow[lx]];
                    dst32[lx] = (color << 16) | color;
                }
                std::memcpy(dst + physicalWidth, dst, physicalWidth * sizeof(uint16_t));
                dst += physicalWidth * 2;
            }
        } else {
            // Normal path with scaling (using LUTs)
            for (int physY = startY; physY < endY; ++physY) {
                int srcY = yLUT[physY];
                scaleLine(srcY, dst);
                dst += physicalWidth;
            }
        }

        // 2. Now we wait for DMA to finish the previous block
        // If CPU calculation was slower than SPI, this returns immediately.
        tft.dmaWait();

        // 3. Send the new calculated block
        tft.pushPixelsDMA(lineBuffer[currentBuffer], physicalWidth * numLines);

        // 4. Swap and advance
        currentBuffer = 1 - currentBuffer;
        startY += activeLinesPerBlock;
    }
    
    // Wait for the last pending transfer to finish
    tft.dmaWait();
    tft.endWrite();

    if constexpr (pixelroot32::platforms::config::EnableProfiling) {
        uint32_t elapsed = micros() - start;
        static uint32_t lastReport = 0;
        if (millis() - lastReport > 1000) {
            log(LogLevel::Profiling, "Scaled DMA Transfer: %u us (%u FPS max)", elapsed, 1000000 / (elapsed > 0 ? elapsed : 1));
            lastReport = millis();
        }
    }
}

void IRAM_ATTR pr32::drivers::esp32::TFT_eSPI_Drawer::scaleLine(int srcY, uint16_t* dst) {
    // Get pointer to source row in 8bpp sprite
    uint8_t* spritePtr = (uint8_t*)spr.getPointer();
    uint8_t* srcRow = spritePtr + (srcY * logicalWidth);
    
    // Use local pointers to help optimization
    const uint16_t* __restrict pLUT = paletteLUT;
    const uint16_t* __restrict xL = xLUT;
    
    int physX = 0;
    
    // Unroll loop 4x for better pipeline usage
    // This reduces loop overhead and allows better instruction scheduling
    for (; physX <= physicalWidth - 4; physX += 4) {
        dst[physX]     = pLUT[srcRow[xL[physX]]];
        dst[physX + 1] = pLUT[srcRow[xL[physX + 1]]];
        dst[physX + 2] = pLUT[srcRow[xL[physX + 2]]];
        dst[physX + 3] = pLUT[srcRow[xL[physX + 3]]];
    }
    
    // Handle remaining pixels
    for (; physX < physicalWidth; ++physX) {
        dst[physX] = pLUT[srcRow[xL[physX]]];
    }
}

bool pr32::drivers::esp32::TFT_eSPI_Drawer::processEvents() {
    return true;
}

#endif // ESP32
