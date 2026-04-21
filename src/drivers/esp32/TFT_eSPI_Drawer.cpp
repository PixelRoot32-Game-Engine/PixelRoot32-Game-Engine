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

#ifdef PIXELROOT32_ENABLE_PROFILING
// Per-frame block timings inside sendBufferScaled (8bpp→RGB565 + DMA). Scoped vars live in sendBufferScaled().
#define PR32_SEND_BUF_PROFILE_VARS()                                   \
    uint32_t pr32_sendbuf_mark = micros();                             \
    uint32_t const pr32_sendbuf_t0 = pr32_sendbuf_mark;                \
    uint32_t pr32_acc_setup = 0, pr32_acc_scale = 0,                   \
             pr32_acc_push = 0, pr32_acc_wait = 0,                     \
             pr32_acc_end = 0;

#define PR32_SEND_BUF_PROFILE_ACC(acc_var)                             \
    do {                                                               \
        uint32_t pr32_pf_n = micros();                                 \
        acc_var += static_cast<uint32_t>(pr32_pf_n - pr32_sendbuf_mark); \
        pr32_sendbuf_mark = pr32_pf_n;                                 \
    } while (0)

#else

#define PR32_SEND_BUF_PROFILE_VARS() ((void)0)
#define PR32_SEND_BUF_PROFILE_ACC(acc_var) ((void)0)

#endif

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

    // Configure partial update tracker for actual logical resolution
    partialController_.configure(logicalWidth, logicalHeight);

    // Wire build-time configuration flags to runtime behavior
    // ENABLE_PARTIAL_UPDATES: controls whether partial updates are active
    partialController_.setPartialUpdateEnabled(
        pixelroot32::platforms::config::EnablePartialUpdates);

    // ENABLE_DIRTY_RECT_COMBINE: controls region merging algorithm
    partialController_.setCombineEnabled(
        pixelroot32::platforms::config::EnableDirtyRectCombine);

    // DISPLAY_COLOR_DEPTH: set initial color depth from build config
    colorDepthManager_.setDepth(
        pixelroot32::platforms::config::DisplayColorDepth);

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
    // Check if partial updates are enabled and beneficial
    if (partialController_.isPartialUpdateEnabled()) {
        // End frame tracking - this calculates dirty regions and decides mode
        partialController_.endFrame(logicalWidth, logicalHeight);
        
        // Check if partial update is beneficial
        if (partialController_.shouldUsePartial()) {
            // Send only the dirty regions (optimized path)
            sendBufferPartial(partialController_.getRegions());
            partialController_.clear();
            return;
        }
    }
    
    // Fallback: full frame send (original behavior for compatibility)
    sendBufferScaled();
    partialController_.clear();
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

    // Auto-mark dirty region if enabled (default behavior)
    if (autoMarkDirty_) {
        markDirty(x, y, clippedW, clippedH);
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
    int linesPerBlock = PIXELROOT32_TFT_ESPI_LINES_PER_BLOCK;
    size_t blockSize = physicalWidth * linesPerBlock * sizeof(uint16_t);
    
    // Allocate double line buffers for DMA
#ifdef ESP32
    lineBuffer[0] = (uint16_t*)heap_caps_malloc(blockSize, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    lineBuffer[1] = (uint16_t*)heap_caps_malloc(blockSize, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    
    // If either buffer failed, try fallback (smaller) size for both
    if (!lineBuffer[0] || !lineBuffer[1]) {
        // Free any partial allocation
        if (lineBuffer[0]) { heap_caps_free(lineBuffer[0]); lineBuffer[0] = nullptr; }
        if (lineBuffer[1]) { heap_caps_free(lineBuffer[1]); lineBuffer[1] = nullptr; }
        
        linesPerBlock = PIXELROOT32_TFT_ESPI_LINES_PER_BLOCK_FALLBACK;
        blockSize = physicalWidth * linesPerBlock * sizeof(uint16_t);
        
        lineBuffer[0] = (uint16_t*)heap_caps_malloc(blockSize, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
        lineBuffer[1] = (uint16_t*)heap_caps_malloc(blockSize, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    }
    
    // Force LUTs to Internal RAM for speed
    paletteLUT = (uint16_t*)heap_caps_malloc(256 * sizeof(uint16_t), MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    xLUT = (uint16_t*)heap_caps_malloc(physicalWidth * sizeof(uint16_t), MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    yLUT = (uint16_t*)heap_caps_malloc(physicalHeight * sizeof(uint16_t), MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);

    // Check each allocation individually and log specific failures
    bool allocationFailed = false;
    
    if (!lineBuffer[0]) {
        log(LogLevel::Error, "Failed to allocate lineBuffer[0] (%zu bytes) in DMA internal RAM!", blockSize);
        allocationFailed = true;
    }
    
    if (!lineBuffer[1]) {
        log(LogLevel::Error, "Failed to allocate lineBuffer[1] (%zu bytes) in DMA internal RAM!", blockSize);
        allocationFailed = true;
    }
    
    if (!paletteLUT) {
        log(LogLevel::Error, "Failed to allocate paletteLUT (%zu bytes) in internal RAM!", 256UL * sizeof(uint16_t));
        allocationFailed = true;
    }
    
    if (!xLUT) {
        log(LogLevel::Error, "Failed to allocate xLUT (%zu bytes) in internal RAM!", physicalWidth * sizeof(uint16_t));
        allocationFailed = true;
    }
    
    if (!yLUT) {
        log(LogLevel::Error, "Failed to allocate yLUT (%zu bytes) in internal RAM!", physicalHeight * sizeof(uint16_t));
        allocationFailed = true;
    }
    
    if (allocationFailed) {
        dmaAvailable_ = false;
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
    uint8_t* spritePtr = (uint8_t*)spr.getPointer();
    if (!spritePtr) {
        return;
    }

#ifdef PIXELROOT32_ENABLE_PROFILING
    PR32_SEND_BUF_PROFILE_VARS();
#endif

    tft.startWrite();
    tft.setAddrWindow(xOffset, yOffset, physicalWidth, physicalHeight);
#ifdef PIXELROOT32_ENABLE_PROFILING
    PR32_SEND_BUF_PROFILE_ACC(pr32_acc_setup);
#endif

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
                 uint8_t* srcRow = spritePtr + ((startY + i) * logicalWidth);
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
                uint8_t* srcRow = spritePtr + (srcY * logicalWidth);
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
                scaleLine(spritePtr, srcY, dst);
                dst += physicalWidth;
            }
        }

#ifdef PIXELROOT32_ENABLE_PROFILING
        PR32_SEND_BUF_PROFILE_ACC(pr32_acc_scale);
#endif
        // Start transfer of block 0 (DMA if available, otherwise software)
        if (dmaAvailable_) {
            tft.pushPixelsDMA(lineBuffer[currentBuffer], physicalWidth * numLines);
        } else {
            tft.pushPixels(lineBuffer[currentBuffer], physicalWidth * numLines);
        }
#ifdef PIXELROOT32_ENABLE_PROFILING
        PR32_SEND_BUF_PROFILE_ACC(pr32_acc_push);
#endif

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
                 uint8_t* srcRow = spritePtr + ((startY + i) * logicalWidth);
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
                uint8_t* srcRow = spritePtr + (srcY * logicalWidth);
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
                scaleLine(spritePtr, srcY, dst);
                dst += physicalWidth;
            }
        }

#ifdef PIXELROOT32_ENABLE_PROFILING
        PR32_SEND_BUF_PROFILE_ACC(pr32_acc_scale);
#endif
        // 2. Now we wait for DMA to finish the previous block
        // If CPU calculation was slower than SPI, this returns immediately.
        tft.dmaWait();
#ifdef PIXELROOT32_ENABLE_PROFILING
        PR32_SEND_BUF_PROFILE_ACC(pr32_acc_wait);
#endif

        // 3. Send the new calculated block (DMA if available, otherwise software)
        if (dmaAvailable_) {
            tft.pushPixelsDMA(lineBuffer[currentBuffer], physicalWidth * numLines);
        } else {
            tft.pushPixels(lineBuffer[currentBuffer], physicalWidth * numLines);
        }
#ifdef PIXELROOT32_ENABLE_PROFILING
        PR32_SEND_BUF_PROFILE_ACC(pr32_acc_push);
#endif

        // 4. Swap and advance
        currentBuffer = 1 - currentBuffer;
        startY += activeLinesPerBlock;
    }
    
    // Wait for the last pending transfer to finish
    tft.dmaWait();
#ifdef PIXELROOT32_ENABLE_PROFILING
    PR32_SEND_BUF_PROFILE_ACC(pr32_acc_wait);
#endif
    tft.endWrite();
#ifdef PIXELROOT32_ENABLE_PROFILING
    PR32_SEND_BUF_PROFILE_ACC(pr32_acc_end);

    {
        const uint32_t totalUs = static_cast<uint32_t>(micros() - pr32_sendbuf_t0);
        static uint32_t sumSetup = 0;
        static uint32_t sumScale = 0;
        static uint32_t sumPush = 0;
        static uint32_t sumWait = 0;
        static uint32_t sumEnd = 0;
        static uint32_t sumTotal = 0;
        static uint32_t frameCount = 0;
        static uint32_t lastReportMs = 0;

        sumSetup += pr32_acc_setup;
        sumScale += pr32_acc_scale;
        sumPush += pr32_acc_push;
        sumWait += pr32_acc_wait;
        sumEnd += pr32_acc_end;
        sumTotal += totalUs;
        ++frameCount;

        if (millis() - lastReportMs > 1000) {
            if (frameCount > 0) {
                const uint32_t n = frameCount;
                const uint32_t avgTotal = sumTotal / n;
                const uint32_t avgSetup = sumSetup / n;
                const uint32_t avgScale = sumScale / n;
                const uint32_t avgPush = sumPush / n;
                const uint32_t avgWait = sumWait / n;
                const uint32_t avgEnd = sumEnd / n;
                const uint32_t sumParts = avgSetup + avgScale + avgPush + avgWait + avgEnd;
                const int delta = static_cast<int>(avgTotal) - static_cast<int>(sumParts);

                log(LogLevel::Profiling,
                    "[TFT sendBufferScaled avg/%u fr] total %uu | setup %uu | scale %uu | dmaWait %uu | pushDMA %uu | endWrite %uu | Σparts %uu (Δ %d) | %u FPS",
                    static_cast<unsigned>(n),
                    static_cast<unsigned>(avgTotal),
                    static_cast<unsigned>(avgSetup),
                    static_cast<unsigned>(avgScale),
                    static_cast<unsigned>(avgWait),
                    static_cast<unsigned>(avgPush),
                    static_cast<unsigned>(avgEnd),
                    static_cast<unsigned>(sumParts),
                    delta,
                    static_cast<unsigned>(1000000 / (avgTotal > 0 ? avgTotal : 1)));

                log(LogLevel::Profiling, "Scaled DMA Transfer: %u us (%u FPS max)",
                    static_cast<unsigned>(avgTotal),
                    static_cast<unsigned>(1000000 / (avgTotal > 0 ? avgTotal : 1)));
            }
            sumSetup = sumScale = sumPush = sumWait = sumEnd = sumTotal = 0;
            frameCount = 0;
            lastReportMs = millis();
        }
    }
#endif
}

void IRAM_ATTR pr32::drivers::esp32::TFT_eSPI_Drawer::scaleLine(const uint8_t* spriteBase, int srcY, uint16_t* dst) {
    const uint8_t* srcRow = spriteBase + (srcY * logicalWidth);
    
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

// ============================================================================
// Partial Update API Implementations
// ============================================================================

void pr32::drivers::esp32::TFT_eSPI_Drawer::markDirty(int x, int y, int width, int height) {
    partialController_.markDirty(x, y, width, height);
}

void pr32::drivers::esp32::TFT_eSPI_Drawer::clearDirtyFlags() {
    partialController_.clear();
}

bool pr32::drivers::esp32::TFT_eSPI_Drawer::hasDirtyRegions() const {
    return partialController_.shouldUsePartial();
}

void pr32::drivers::esp32::TFT_eSPI_Drawer::setPartialUpdateEnabled(bool enabled) {
    partialController_.setPartialUpdateEnabled(enabled);
}

bool pr32::drivers::esp32::TFT_eSPI_Drawer::isPartialUpdateEnabled() const {
    return partialController_.isPartialUpdateEnabled();
}

int pr32::drivers::esp32::TFT_eSPI_Drawer::getLastRegionCount() const {
    return partialController_.getLastRegionCount();
}

int pr32::drivers::esp32::TFT_eSPI_Drawer::getLastTotalSentPixels() const {
    return partialController_.getLastTotalSentPixels();
}

int pr32::drivers::esp32::TFT_eSPI_Drawer::getDirtyPixelCount() const {
    return partialController_.getDirtyPixelCount();
}

int pr32::drivers::esp32::TFT_eSPI_Drawer::getLastFrameWidth() const {
    return partialController_.getLastFrameWidth();
}

int pr32::drivers::esp32::TFT_eSPI_Drawer::getLastFrameHeight() const {
    return partialController_.getLastFrameHeight();
}

void pr32::drivers::esp32::TFT_eSPI_Drawer::setColorDepth(int depth) {
    colorDepthManager_.setDepth(depth);
}

// ============================================================================
// Partial Update Transfer Implementations
// ============================================================================

void pr32::drivers::esp32::TFT_eSPI_Drawer::sendBufferPartial(const std::vector<pr32::graphics::DirtyRect>& regions) {
    if (regions.empty()) {
        // Fallback to full frame if no regions
        sendBufferScaled();
        return;
    }

    const uint32_t minPixels = static_cast<uint32_t>(partialController_.getMinRegionPixels());

#ifdef PIXELROOT32_ENABLE_PROFILING
    uint32_t pr32_partial_t0 = micros();
#endif

    tft.startWrite();

#ifdef PIXELROOT32_ENABLE_PROFILING
    uint32_t pr32_partial_t_setup = micros();
#endif

    // Send each dirty region that meets the minimum threshold.
    // This is consistent with the stats computed in PartialUpdateController::endFrame()
    // which also filters by minRegionPixels_.
    int sentRegionCount = 0;
    uint32_t sentPixelCount = 0;
    for (const auto& region : regions) {
        if (region.area() < minPixels) {
            continue;
        }
        sendRegion(region.x, region.y, region.width, region.height);
        ++sentRegionCount;
        sentPixelCount += region.area();
    }

#ifdef PIXELROOT32_ENABLE_PROFILING
    uint32_t pr32_partial_t_transfer = micros();
#endif

    tft.dmaWait();
    tft.endWrite();

#ifdef PIXELROOT32_ENABLE_PROFILING
    uint32_t pr32_partial_t_end = micros();
#endif

    // Update statistics - actual SPI transfer is always 2 bytes/pixel (RGB565)
    // regardless of colorDepthManager_ setting, because paletteLUT converts to 16-bit.
    colorDepthManager_.addBytesTransferred(sentPixelCount * 2);
    colorDepthManager_.incrementFrameCount();

#ifdef PIXELROOT32_ENABLE_PROFILING
    {
        static uint32_t sumTotal = 0, sumSetup = 0, sumTransfer = 0, sumWait = 0;
        static uint32_t frameCount = 0;
        static uint32_t lastReportMs = 0;
        static uint32_t sumRegions = 0, sumPixels = 0;

        const uint32_t totalUs = static_cast<uint32_t>(pr32_partial_t_end - pr32_partial_t0);
        sumTotal += totalUs;
        sumSetup += static_cast<uint32_t>(pr32_partial_t_setup - pr32_partial_t0);
        sumTransfer += static_cast<uint32_t>(pr32_partial_t_transfer - pr32_partial_t_setup);
        sumWait += static_cast<uint32_t>(pr32_partial_t_end - pr32_partial_t_transfer);
        sumRegions += sentRegionCount;
        sumPixels += sentPixelCount;
        ++frameCount;

        if (millis() - lastReportMs > 1000) {
            if (frameCount > 0) {
                const uint32_t n = frameCount;
                log(LogLevel::Profiling,
                    "[TFT Partial avg/%u fr] total %uu | setup %uu | xfer+scale %uu | dmaWait %uu | regions %u | px %u",
                    static_cast<unsigned>(n),
                    static_cast<unsigned>(sumTotal / n),
                    static_cast<unsigned>(sumSetup / n),
                    static_cast<unsigned>(sumTransfer / n),
                    static_cast<unsigned>(sumWait / n),
                    static_cast<unsigned>(sumRegions / n),
                    static_cast<unsigned>(sumPixels / n));
            }
            sumTotal = sumSetup = sumTransfer = sumWait = sumRegions = sumPixels = 0;
            frameCount = 0;
            lastReportMs = millis();
        }
    }
#endif
}

void pr32::drivers::esp32::TFT_eSPI_Drawer::sendRegion(int16_t x, int16_t y, uint16_t w, uint16_t h) {
    if (w == 0 || h == 0) {
        return;
    }
    
    // ============================================================================
    // Region coordinate scaling for physical display coordinates
    // ============================================================================
    // Scale from logical (sprite) coordinates to physical display coordinates
    // Physical = offset + (logical * scale)
    
    int physX = xOffset + (x * physicalWidth / logicalWidth);
    int physY = yOffset + (y * physicalHeight / logicalHeight);
    int physW = w * physicalWidth / logicalWidth;
    int physH = h * physicalHeight / logicalHeight;
    
    // Ensure minimum size
    if (physW < 1) physW = 1;
    if (physH < 1) physH = 1;
    
    // Set address window for this specific region (partial update)
    tft.setAddrWindow(physX, physY, physW, physH);
    
    // Get sprite buffer pointer
    uint8_t* spritePtr = (uint8_t*)spr.getPointer();
    if (!spritePtr) {
        return;
    }
    
    // Process lines in blocks
    int linesPerBlock = activeLinesPerBlock;
    
    // Total output lines: physical height when scaling, logical height for 1:1
    int totalOutLines = needsScaling() ? physH : static_cast<int>(h);
    bool firstBlock = true;
    
    for (int startLine = 0; startLine < totalOutLines; startLine += linesPerBlock) {
        int endLine = (startLine + linesPerBlock > totalOutLines) ? totalOutLines : startLine + linesPerBlock;
        int numLines = endLine - startLine;
        
        // Scale lines into line buffer (CPU work overlaps with previous DMA)
        uint16_t* dst = lineBuffer[currentBuffer];
        
        if (!needsScaling()) {
            // 1:1 path (no scaling) - optimized for partial regions
            for (int i = 0; i < numLines; ++i) {
                int srcY = y + startLine + i;
                if (srcY >= logicalHeight) break;
                
                uint8_t* srcRow = spritePtr + (srcY * logicalWidth) + x;
                const uint16_t* __restrict pLUT = paletteLUT;
                int xi = 0;
                uint32_t* dst32 = (uint32_t*)dst;
                
                // Process 8 pixels at a time using 32-bit writes
                for (; xi <= static_cast<int>(w) - 8; xi += 8) {
                    uint32_t p01 = ((uint32_t)pLUT[srcRow[xi+1]] << 16) | pLUT[srcRow[xi]];
                    uint32_t p23 = ((uint32_t)pLUT[srcRow[xi+3]] << 16) | pLUT[srcRow[xi+2]];
                    uint32_t p45 = ((uint32_t)pLUT[srcRow[xi+5]] << 16) | pLUT[srcRow[xi+4]];
                    uint32_t p67 = ((uint32_t)pLUT[srcRow[xi+7]] << 16) | pLUT[srcRow[xi+6]];
                    
                    dst32[xi/2]     = p01;
                    dst32[xi/2 + 1] = p23;
                    dst32[xi/2 + 2] = p45;
                    dst32[xi/2 + 3] = p67;
                }
                // Handle remaining pixels
                for (; xi < static_cast<int>(w); ++xi) {
                    dst[xi] = pLUT[srcRow[xi]];
                }
                dst += physW;  // Advance by physical width (== w in 1:1 case)
            }
        } else {
            // Scaling path: region-aware coordinate mapping
            // Maps from physical output lines back to logical source lines
            // using region-relative calculations instead of global yLUT/scaleLine
            const uint16_t* __restrict pLUT = paletteLUT;
            
            for (int row = 0; row < numLines; ++row) {
                int absPhysRow = startLine + row;
                // Region-relative physical→logical Y mapping
                int srcY = y + (absPhysRow * static_cast<int>(h) / physH);
                if (srcY >= logicalHeight) srcY = logicalHeight - 1;
                
                const uint8_t* srcRow = spritePtr + (srcY * logicalWidth);
                
                // Region-relative physical→logical X mapping per pixel
                for (int px = 0; px < physW; ++px) {
                    int srcX = x + (px * static_cast<int>(w) / physW);
                    if (srcX >= logicalWidth) srcX = logicalWidth - 1;
                    dst[px] = pLUT[srcRow[srcX]];
                }
                dst += physW;
            }
        }
        
        // Pipelined DMA: wait for PREVIOUS block before starting this one
        // CPU filled current buffer while DMA was transferring the previous buffer
        if (!firstBlock) {
            tft.dmaWait();
        }
        
        tft.pushPixelsDMA(lineBuffer[currentBuffer], physW * numLines);
        currentBuffer = 1 - currentBuffer;
        firstBlock = false;
    }
    
    // Wait for the final DMA block to complete
    if (!firstBlock) {
        tft.dmaWait();
    }

    // ============================================================================
    // Task 5.3: Debug overlay - draw 2px red border around sent region
    // ============================================================================
    #if PIXELROOT32_DEBUG_DIRTY_REGIONS
    if (debugDirtyRegions_) {
        // 0xF800 = RGB565 pure red
        constexpr uint16_t DEBUG_BORDER_COLOR = 0xF800;
        constexpr uint8_t BORDER_THICKNESS = 2;

        // Draw horizontal borders using fast line drawing
        for (uint8_t i = 0; i < BORDER_THICKNESS; i++) {
            tft.drawFastHLine(physX, physY + i, physW, DEBUG_BORDER_COLOR);         // Top
            tft.drawFastHLine(physX, physY + physH - 1 - i, physW, DEBUG_BORDER_COLOR); // Bottom
        }
        // Draw vertical borders using fast line drawing
        for (uint8_t i = 0; i < BORDER_THICKNESS; i++) {
            tft.drawFastVLine(physX + i, physY, physH, DEBUG_BORDER_COLOR);         // Left
            tft.drawFastVLine(physX + physW - 1 - i, physY, physH, DEBUG_BORDER_COLOR); // Right
        }
    }
    #endif
}

#endif // ESP32
