#include <drivers/esp32/TFT_eSPI_Drawer.h>

#if defined(PIXELROOT32_USE_TFT_ESPI_DRIVER)

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

// --------------------------------------------------
// Scaling Functions
// --------------------------------------------------

void pr32::drivers::esp32::TFT_eSPI_Drawer::buildScaleLUTs() {
    freeScalingBuffers();
    
    // We will use blocks of LINES_PER_BLOCK lines to reduce DMA overhead
    size_t blockSize = physicalWidth * LINES_PER_BLOCK * sizeof(uint16_t);

    // Allocate double line buffers for DMA
#ifdef ESP32
    lineBuffer[0] = (uint16_t*)heap_caps_malloc(blockSize, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    lineBuffer[1] = (uint16_t*)heap_caps_malloc(blockSize, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    // Force LUTs to Internal RAM for speed
    paletteLUT = (uint16_t*)heap_caps_malloc(256 * sizeof(uint16_t), MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    xLUT = (uint16_t*)heap_caps_malloc(physicalWidth * sizeof(uint16_t), MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    yLUT = (uint16_t*)heap_caps_malloc(physicalHeight * sizeof(uint16_t), MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);

    if (!lineBuffer[0] || !lineBuffer[1] || !paletteLUT || !xLUT || !yLUT) {
        Serial.println("[ERROR] Failed to allocate DMA or Palette buffers in Internal RAM!");
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
#ifdef PIXELROOT32_ENABLE_PROFILING
    uint32_t start = micros();
#endif

    uint8_t* spritePtr = (uint8_t*)spr.getPointer();
    if (!spritePtr) return;

    tft.startWrite();
    tft.setAddrWindow(xOffset, yOffset, physicalWidth, physicalHeight);
    
    currentBuffer = 0;
    int startY = 0;

    // ---------------------------------------------------------
    // STAGE 1: Pre-fill (First Block)
    // Calculate the first block before starting any DMA
    // ---------------------------------------------------------
    if (startY < physicalHeight) {
        int endY = startY + LINES_PER_BLOCK;
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
                 
                 // EXTREME OPTIMIZATION: 32-bit writes
                 // Instead of writing uint16_t twice, we write uint32_t once
                 uint32_t* dst32 = (uint32_t*)dst;
                 
                 // Process 8 pixels per iteration (4 writes of 32 bits)
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
        startY += LINES_PER_BLOCK;
    }

    // ---------------------------------------------------------
    // STAGE 2: Pipeline (Main Loop)
    // While DMA sends the PREVIOUS buffer, CPU calculates the CURRENT one
    // ---------------------------------------------------------
    while (startY < physicalHeight) {
        int endY = startY + LINES_PER_BLOCK;
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
                 
                 // EXTREME OPTIMIZATION: 32-bit writes
                 // Instead of writing uint16_t twice, we write uint32_t once
                 // This reduces memory access count by half
                 uint32_t* dst32 = (uint32_t*)dst;
                 
                 // Process 8 pixels per iteration (4 writes of 32 bits)
                 for (; x <= physicalWidth - 8; x += 8) {
                     // Combine 2 pixels into 1 32-bit word
                     // Note: ESP32 is Little Endian
                     uint32_t p01 = ((uint32_t)pLUT[srcRow[x+1]] << 16) | pLUT[srcRow[x]];
                     uint32_t p23 = ((uint32_t)pLUT[srcRow[x+3]] << 16) | pLUT[srcRow[x+2]];
                     uint32_t p45 = ((uint32_t)pLUT[srcRow[x+5]] << 16) | pLUT[srcRow[x+4]];
                     uint32_t p67 = ((uint32_t)pLUT[srcRow[x+7]] << 16) | pLUT[srcRow[x+6]];

                     dst32[x/2]     = p01;
                     dst32[x/2 + 1] = p23;
                     dst32[x/2 + 2] = p45;
                     dst32[x/2 + 3] = p67;
                 }
                 
                 // Remainder
                 for (; x < physicalWidth; ++x) {
                     dst[x] = pLUT[srcRow[x]];
                 }
                 
                 // Advance destination pointer to the next line
                 dst += physicalWidth;
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
        startY += LINES_PER_BLOCK;
    }
    
    // Wait for the last pending transfer to finish
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
