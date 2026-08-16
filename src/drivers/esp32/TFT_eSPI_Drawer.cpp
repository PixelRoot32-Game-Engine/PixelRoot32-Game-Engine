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

namespace {

/// Drawer currently owning the SPI bus, so the touch bridge can flush its
/// deferred DMA before running a transaction on the same bus. Mirrors the
/// gTftForTouch global registered by the touch bridge itself.
pr32::drivers::esp32::TFT_eSPI_Drawer* gDrawerForTouchFlush = nullptr;

void flushDrawerDmaBeforeTouch() {
    if (gDrawerForTouchFlush != nullptr) {
        gDrawerForTouchFlush->waitForPendingDMA();
    }
}

} // namespace

// --------------------------------------------------
// Constructor / Destructor
// --------------------------------------------------

pr32::drivers::esp32::TFT_eSPI_Drawer::TFT_eSPI_Drawer()
    : tft()
    , spr(&tft)
{
}

pr32::drivers::esp32::TFT_eSPI_Drawer::~TFT_eSPI_Drawer() {
    // Teardown touches tft and destroys the sprite: nothing may still be in flight.
    waitForPendingDMA();
    if (gDrawerForTouchFlush == this) {
        gDrawerForTouchFlush = nullptr;
        pixelroot32::drivers::esp32::registerTouchBusFlushHook(nullptr);
    }
    freeScalingBuffers();
}

void pr32::drivers::esp32::TFT_eSPI_Drawer::waitForPendingDMA() {
    // Keep in sync with the profiled flush at the top of sendBufferScaled().
    if (dmaPending) {
        tft.dmaWait();
        tft.endWrite();
        dmaPending = false;
    }
}

// --------------------------------------------------
// Init & configuration
// --------------------------------------------------

void pr32::drivers::esp32::TFT_eSPI_Drawer::init() {
    // Re-init on a live drawer would reprogram the bus under an in-flight transfer.
    waitForPendingDMA();

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

#if PIXELROOT32_TFT_12BIT_COLOR
    // Switch the panel to the 12-bit pixel format only after buildScaleLUTs()
    // has decided whether the packed stream is usable for this geometry, and
    // after the fillScreen() above, which still ran in the 16-bit format the
    // panel boots into. From here on nothing but sendBufferScaled() writes
    // pixels, so no other code path can emit RGB565 into a 12-bit window.
    if (use12BitColor) {
        tft.writecommand(0x3A); // MIPI DCS COLMOD - Interface Pixel Format
        tft.writedata(0x03);    // 12 bits/pixel (RGB444)
        log("[TFT_eSPI_Drawer] Panel switched to 12-bit colour (COLMOD 0x03).");
    }
#endif

    log("[TFT_eSPI_Drawer] Initialization complete.");

    pixelroot32::drivers::esp32::registerTftForXpt2046Touch(&tft);

    // Touch reads share the SPI bus with the display, so the bridge must be able
    // to flush the frame transfer we intentionally leave in flight.
    gDrawerForTouchFlush = this;
    pixelroot32::drivers::esp32::registerTouchBusFlushHook(&flushDrawerDmaBeforeTouch);
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
        // Talks to the panel over SPI: the deferred frame transfer must land first.
        waitForPendingDMA();
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
    int linesPerBlock = PIXELROOT32_TFT_ESPI_LINES_PER_BLOCK;
#if PIXELROOT32_TFT_12BIT_COLOR
    // The packed RGB444 stream is handed to pushPixelsDMA() as 16-bit words, so
    // every block must be a whole number of words. A block is
    // bytesPerLine444() * numLines and the LAST block of a frame can carry any
    // line count, so bytesPerLine444() itself has to be even. physicalWidth *
    // 3 / 2 is even exactly when physicalWidth is a multiple of 4 - note that
    // "even width" is not enough (e.g. 242 -> 363 bytes/line, odd).
    //
    // Rather than emit a half-word tail (which would shift every following
    // block by one byte and shear the frame), widths that fail the test simply
    // keep the RGB565 path. That covers panels such as the 135x240 ST7789.
    use12BitColor = (physicalWidth % 4) == 0;
    if (!use12BitColor) {
        log(LogLevel::Warning,
            "[TFT_eSPI_Drawer] 12-bit colour requested but physical width %d is not a multiple of 4; keeping RGB565.",
            physicalWidth);
    }

    // The pair LUT has to be resolved BEFORE the line buffers are sized. The two
    // formats need a different number of bytes per line, so a downgrade decided
    // after the buffers were already allocated would leave the RGB565 path
    // storing two bytes per pixel into a buffer sized for one and a half - a
    // silent heap overflow on every line of every frame. Taking the small table
    // first also keeps it clear of the much larger DMA buffers.
#ifdef ESP32
    if (use12BitColor) {
        pairLUT = (uint8_t(*)[3])heap_caps_malloc(256 * 3, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
        if (!pairLUT) {
            // 768 bytes we could not get: degrade to RGB565 rather than run the
            // 2x path without its table. The panel is still in its boot format
            // at this point, so init() simply never sends COLMOD.
            log(LogLevel::Warning,
                "[TFT_eSPI_Drawer] Failed to allocate the 768 byte RGB444 pair LUT; keeping RGB565 at %u bytes/line.",
                static_cast<unsigned>(static_cast<size_t>(physicalWidth) * sizeof(uint16_t)));
            use12BitColor = false;
        }
    }
#endif

    const size_t lineBytes = use12BitColor
        ? static_cast<size_t>(bytesPerLine444())
        : static_cast<size_t>(physicalWidth) * sizeof(uint16_t);
    size_t blockSize = lineBytes * static_cast<size_t>(linesPerBlock);
#else
    size_t blockSize = physicalWidth * linesPerBlock * sizeof(uint16_t);
#endif

    // Allocate double line buffers for DMA
#ifdef ESP32
    // Try the optimal block size for BOTH buffers first. Only if either allocation
    // fails do we drop to the fallback size: on boards with tight DMA-capable RAM
    // the smaller blocks still work, they just halve the pipelining window.
    lineBuffer[0] = (uint16_t*)heap_caps_malloc(blockSize, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    lineBuffer[1] = (uint16_t*)heap_caps_malloc(blockSize, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);

    if (!lineBuffer[0] || !lineBuffer[1]) {
        for (int i = 0; i < 2; ++i) {
            if (lineBuffer[i]) {
                heap_caps_free(lineBuffer[i]);
                lineBuffer[i] = nullptr;
            }
        }

        linesPerBlock = PIXELROOT32_TFT_ESPI_LINES_PER_BLOCK_FALLBACK;
#if PIXELROOT32_TFT_12BIT_COLOR
        blockSize = lineBytes * static_cast<size_t>(linesPerBlock);
#else
        blockSize = physicalWidth * linesPerBlock * sizeof(uint16_t);
#endif

        // Which block size the board actually got changes the pipelining window,
        // so it has to be visible when reading hardware frame timings.
        log(LogLevel::Warning,
            "[TFT_eSPI_Drawer] DMA block of %d lines did not fit internal RAM; falling back to %d lines/block (%u bytes/buffer).",
            PIXELROOT32_TFT_ESPI_LINES_PER_BLOCK,
            linesPerBlock,
            static_cast<unsigned>(blockSize));

        lineBuffer[0] = (uint16_t*)heap_caps_malloc(blockSize, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
        lineBuffer[1] = (uint16_t*)heap_caps_malloc(blockSize, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    }

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
#if PIXELROOT32_TFT_12BIT_COLOR
    lineBuffer[0] = new uint16_t[(lineBytes * linesPerBlock + 1) / sizeof(uint16_t)];
    lineBuffer[1] = new uint16_t[(lineBytes * linesPerBlock + 1) / sizeof(uint16_t)];
#else
    lineBuffer[0] = new uint16_t[physicalWidth * linesPerBlock];
    lineBuffer[1] = new uint16_t[physicalWidth * linesPerBlock];
#endif
    paletteLUT = new uint16_t[256];
    xLUT = new uint16_t[physicalWidth];
    yLUT = new uint16_t[physicalHeight];
#if PIXELROOT32_TFT_12BIT_COLOR
    if (use12BitColor) {
        pairLUT = (uint8_t(*)[3])new uint8_t[256 * 3];
    }
#endif
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
#if PIXELROOT32_TFT_12BIT_COLOR
    if (use12BitColor) {
        // 12-bit mode assembles the wire bytes nibble by nibble, so there is no
        // 16-bit word left to pre-swap: the LUT holds the plain 0x0RGB value.
        for (int i = 0; i < 256; ++i) {
            const uint16_t rgb444 = pixelroot32::graphics::packRgb565ToRgb444(spr.color8to16(i));
            paletteLUT[i] = rgb444;
            // The 2x fast path only ever emits pairs of IDENTICAL colours, so
            // its 3-byte output is a pure function of the palette index. Bake
            // it once here (768 bytes) instead of packing nibbles per pixel.
            pixelroot32::graphics::packRgb444Pair(pairLUT[i], rgb444, rgb444);
        }
    } else
#endif
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
    // DMA reads straight out of lineBuffer[]; freeing it mid-transfer is a
    // use-after-free that corrupts whatever the allocator hands out next.
    waitForPendingDMA();

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
#if PIXELROOT32_TFT_12BIT_COLOR
    if (pairLUT) {
#ifdef ESP32
        heap_caps_free(pairLUT);
#else
        delete[] reinterpret_cast<uint8_t*>(pairLUT);
#endif
        pairLUT = nullptr;
    }
#endif
}

void IRAM_ATTR pr32::drivers::esp32::TFT_eSPI_Drawer::sendBufferScaled() {
#ifdef PIXELROOT32_ENABLE_PROFILING
    PR32_SEND_BUF_PROFILE_VARS();
#endif

    // Flush the transfer deferred by the previous frame before reopening the bus.
    // Its DMA ran while the game did its update/draw work, so this normally
    // returns immediately - that overlap is the whole point of the deferral.
    //
    // Profiling note: pr32_acc_wait and pr32_acc_end are still accumulated here,
    // but they now measure the PREVIOUS frame's tail, charged to the frame that
    // flushes it. Over a steady-state run the per-frame averages stay comparable;
    // for a single frame the wait/endWrite figures belong to its predecessor.
    if (dmaPending) {
        tft.dmaWait();
#ifdef PIXELROOT32_ENABLE_PROFILING
        PR32_SEND_BUF_PROFILE_ACC(pr32_acc_wait);
#endif
        tft.endWrite();
#ifdef PIXELROOT32_ENABLE_PROFILING
        PR32_SEND_BUF_PROFILE_ACC(pr32_acc_end);
#endif
        dmaPending = false;
    }

    uint8_t* spritePtr = (uint8_t*)spr.getPointer();
    if (!spritePtr) {
        return;
    }

    tft.startWrite();
    tft.setAddrWindow(xOffset, yOffset, physicalWidth, physicalHeight);
#ifdef PIXELROOT32_ENABLE_PROFILING
    PR32_SEND_BUF_PROFILE_ACC(pr32_acc_setup);
#endif

    // INVARIANT - do NOT reset currentBuffer to 0 here.
    //
    // The index alternates continuously ACROSS frames. Each block is pushed from
    // lineBuffer[currentBuffer] and the index is swapped immediately afterwards,
    // so when the loop ends currentBuffer already points AWAY from the block left
    // in flight. Starting the next frame from that value guarantees the first
    // block is scaled into the buffer DMA is not reading.
    //
    // Resetting to 0 breaks that guarantee whenever the frame has an odd number of
    // blocks (e.g. 320 physical lines at 30 lines/block = 11 blocks), because the
    // in-flight buffer is then index 0 - exactly the one the reset would reuse.
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

#if PIXELROOT32_TFT_12BIT_COLOR
        if (use12BitColor) {
            convertBlockRgb444(spritePtr, startY, endY, is2x, (uint8_t*)dst);
        } else
#endif
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
        // Start DMA transfer of block 0
#if PIXELROOT32_TFT_12BIT_COLOR
        if (use12BitColor) {
            // pushPixelsDMA() dumps len 16-bit words verbatim (no byte swap, see
            // the palette LUT note), so the packed byte stream is sent as
            // byteCount/2 words. byteCount is even by the physicalWidth % 4
            // guard in buildScaleLUTs().
            tft.pushPixelsDMA(lineBuffer[currentBuffer],
                              (uint32_t)(bytesPerLine444() * numLines) / 2u);
        } else
#endif
        tft.pushPixelsDMA(lineBuffer[currentBuffer], physicalWidth * numLines);
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

#if PIXELROOT32_TFT_12BIT_COLOR
        if (use12BitColor) {
            convertBlockRgb444(spritePtr, startY, endY, is2x, (uint8_t*)dst);
        } else
#endif
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

        // 3. Send the new calculated block
#if PIXELROOT32_TFT_12BIT_COLOR
        if (use12BitColor) {
            tft.pushPixelsDMA(lineBuffer[currentBuffer],
                              (uint32_t)(bytesPerLine444() * numLines) / 2u);
        } else
#endif
        tft.pushPixelsDMA(lineBuffer[currentBuffer], physicalWidth * numLines);
#ifdef PIXELROOT32_ENABLE_PROFILING
        PR32_SEND_BUF_PROFILE_ACC(pr32_acc_push);
#endif

        // 4. Swap and advance
        currentBuffer = 1 - currentBuffer;
        startY += activeLinesPerBlock;
    }
    
    // Leave the last block in flight and return with the write transaction still
    // open. It is flushed at the top of the next sendBufferScaled(), so its SPI
    // time overlaps the next frame's update + draw work instead of blocking here.
    // waitForPendingDMA() closes it for any other bus user in the meantime.
    dmaPending = true;
#ifdef PIXELROOT32_ENABLE_PROFILING

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
                    // dmaWait/endWrite include the deferred tail of the previous frame,
                    // flushed at the top of this call (see sendBufferScaled entry).
                    "[TFT sendBufferScaled avg/%u fr] total %uu | setup %uu | scale %uu | dmaWait* %uu | pushDMA %uu | endWrite* %uu | Σparts %uu (Δ %d) | %u FPS (* = deferred tail of previous frame)",
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

#if PIXELROOT32_TFT_12BIT_COLOR

void IRAM_ATTR pr32::drivers::esp32::TFT_eSPI_Drawer::convertBlockRgb444(
    const uint8_t* spriteBase, int startY, int endY, bool is2x, uint8_t* dst) {
    const int lineBytes = bytesPerLine444();

    if (!needsScaling()) {
        // 1:1 - two source pixels produce three wire bytes. Unrolled by four
        // pairs (8 pixels -> 12 bytes) to mirror the RGB565 path's stride.
        for (int physY = startY; physY < endY; ++physY) {
            const uint8_t* srcRow = spriteBase + (physY * logicalWidth);
            const uint16_t* __restrict pLUT = paletteLUT;
            int x = 0;
            uint8_t* out = dst;

            for (; x <= physicalWidth - 8; x += 8) {
                pixelroot32::graphics::packRgb444Pair(out,     pLUT[srcRow[x]],     pLUT[srcRow[x + 1]]);
                pixelroot32::graphics::packRgb444Pair(out + 3, pLUT[srcRow[x + 2]], pLUT[srcRow[x + 3]]);
                pixelroot32::graphics::packRgb444Pair(out + 6, pLUT[srcRow[x + 4]], pLUT[srcRow[x + 5]]);
                pixelroot32::graphics::packRgb444Pair(out + 9, pLUT[srcRow[x + 6]], pLUT[srcRow[x + 7]]);
                out += 12;
            }
            // physicalWidth is a multiple of 4 here, so the tail is whole pairs.
            for (; x < physicalWidth; x += 2) {
                pixelroot32::graphics::packRgb444Pair(out, pLUT[srcRow[x]], pLUT[srcRow[x + 1]]);
                out += 3;
            }
            dst += lineBytes;
        }
    } else if (is2x) {
        // 2x fast path: every pixel is duplicated horizontally, so each pair is
        // two identical colours and its three bytes come straight out of
        // pairLUT - no nibble maths in the inner loop at all.
        const uint8_t (* __restrict pairs)[3] = pairLUT;
        for (int physY = startY; physY < endY; physY += 2) {
            const int srcY = physY / 2;
            const uint8_t* srcRow = spriteBase + (srcY * logicalWidth);
            uint8_t* out = dst;

            for (int lx = 0; lx < logicalWidth; ++lx) {
                const uint8_t* __restrict triple = pairs[srcRow[lx]];
                out[0] = triple[0];
                out[1] = triple[1];
                out[2] = triple[2];
                out += 3;
            }
            // Duplicate this line for the next physical row (unchanged policy,
            // just a byte count instead of a pixel count).
            std::memcpy(dst + lineBytes, dst, lineBytes);
            dst += lineBytes * 2;
        }
    } else {
        // Generic scaling path.
        for (int physY = startY; physY < endY; ++physY) {
            const int srcY = yLUT[physY];
            scaleLine444(spriteBase, srcY, dst);
            dst += lineBytes;
        }
    }
}

void IRAM_ATTR pr32::drivers::esp32::TFT_eSPI_Drawer::scaleLine444(
    const uint8_t* spriteBase, int srcY, uint8_t* dst) {
    const uint8_t* srcRow = spriteBase + (srcY * logicalWidth);

    const uint16_t* __restrict pLUT = paletteLUT;
    const uint16_t* __restrict xL = xLUT;

    int physX = 0;

    // Unroll two pairs (4 physical pixels -> 6 bytes) to keep the loop overhead
    // per emitted byte comparable to the RGB565 scaleLine().
    for (; physX <= physicalWidth - 4; physX += 4) {
        pixelroot32::graphics::packRgb444Pair(dst,
            pLUT[srcRow[xL[physX]]],     pLUT[srcRow[xL[physX + 1]]]);
        pixelroot32::graphics::packRgb444Pair(dst + 3,
            pLUT[srcRow[xL[physX + 2]]], pLUT[srcRow[xL[physX + 3]]]);
        dst += 6;
    }

    // physicalWidth is a multiple of 4 in 12-bit mode, so nothing is left here;
    // the loop is kept for symmetry with scaleLine() and costs one test.
    for (; physX < physicalWidth; physX += 2) {
        pixelroot32::graphics::packRgb444Pair(dst,
            pLUT[srcRow[xL[physX]]], pLUT[srcRow[xL[physX + 1]]]);
        dst += 3;
    }
}

#endif // PIXELROOT32_TFT_12BIT_COLOR

bool pr32::drivers::esp32::TFT_eSPI_Drawer::processEvents() {
    return true;
}

#endif // ESP32
