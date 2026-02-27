/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#include <drivers/esp32/U8G2_Drawer.h>
#include <cstring>

#if defined(PIXELROOT32_USE_U8G2_DRIVER)

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

pr32::drivers::esp32::U8G2_Drawer::U8G2_Drawer(U8G2* u8g2, bool ownsInstance) 
    : _u8g2(u8g2), _ownsInstance(ownsInstance) 
{
    if (_u8g2) {
        logicalWidth = _u8g2->getDisplayWidth();
        logicalHeight = _u8g2->getDisplayHeight();
        physicalWidth = logicalWidth;
        physicalHeight = logicalHeight;
    }
}

pr32::drivers::esp32::U8G2_Drawer::~U8G2_Drawer() {
    freeScalingBuffers();
    if (_ownsInstance && _u8g2) {
        delete _u8g2;
        _u8g2 = nullptr;
    }
}

// --------------------------------------------------
// Init & configuration
// --------------------------------------------------

void pr32::drivers::esp32::U8G2_Drawer::init() {
    if (!_u8g2) return;

    Serial.println("[U8G2_Drawer] Initializing U8G2...");
    _u8g2->begin();
    _u8g2->setContrast(255); // Ensure maximum visibility
    setRotation(rotation); // Apply initial rotation
    buildScaleLUTs();
    Serial.println("[U8G2_Drawer] Initialization complete.");
}

void pr32::drivers::esp32::U8G2_Drawer::setRotation(uint16_t rot) {
    // Standardize rotation to index 0-3 (0, 90, 180, 270)
    if (rot == 90) rotation = 1;
    else if (rot == 180) rotation = 2;
    else if (rot == 270) rotation = 3;
    else if (rot >= 360) rotation = (rot / 90) % 4;
    else rotation = rot % 4;
    
    if (_u8g2) {
        switch (rotation) {
            case 0: _u8g2->setDisplayRotation(U8G2_R0); break;
            case 1: _u8g2->setDisplayRotation(U8G2_R1); break;
            case 2: _u8g2->setDisplayRotation(U8G2_R2); break;
            case 3: _u8g2->setDisplayRotation(U8G2_R3); break;
        }
        // After rotation, physical dimensions might have changed
        physicalWidth = _u8g2->getDisplayWidth();
        physicalHeight = _u8g2->getDisplayHeight();
        
        if (physicalWidth > 0 && physicalHeight > 0) {
            buildScaleLUTs();
        }
    }
}

void pr32::drivers::esp32::U8G2_Drawer::setDisplaySize(int w, int h) {
    BaseDrawSurface::setDisplaySize(w, h);
    if (logicalWidth > 0 && logicalHeight > 0) {
        buildScaleLUTs();
    }
}

void pr32::drivers::esp32::U8G2_Drawer::setPhysicalSize(int w, int h) {
    BaseDrawSurface::setPhysicalSize(w, h);
    if (physicalWidth > 0 && physicalHeight > 0) {
        buildScaleLUTs();
    }
}

// --------------------------------------------------
// Buffer control
// --------------------------------------------------

void IRAM_ATTR pr32::drivers::esp32::U8G2_Drawer::clearBuffer() {
    if (!_u8g2) return;
    if (needsScaling()) {
        if (_internalBuffer) {
            std::memset(_internalBuffer, 0, _logicalStride * logicalHeight);
        }
    } else {
        _u8g2->clearBuffer();
    }
}

void IRAM_ATTR pr32::drivers::esp32::U8G2_Drawer::sendBuffer() {
    if (!_u8g2) return;

    if (needsScaling()) {
        sendBufferScaled();
    } else {
#ifdef PIXELROOT32_ENABLE_PROFILING
        uint32_t start = micros();
#endif
        _u8g2->sendBuffer();
#ifdef PIXELROOT32_ENABLE_PROFILING
        uint32_t elapsed = micros() - start;
        static uint32_t lastReport = 0;
        if (millis() - lastReport > 1000) {
            Serial.printf("[PROFILING] U8G2 Transfer: %u us (%u FPS max)\n", elapsed, 1000000 / (elapsed > 0 ? elapsed : 1));
            lastReport = millis();
        }
#endif
    }
}

// --------------------------------------------------
// Primitive drawing
// --------------------------------------------------

void IRAM_ATTR pr32::drivers::esp32::U8G2_Drawer::drawPixel(int x, int y, uint16_t color) {
    if (!_u8g2) return;
    uint8_t c = rgb565To1Bit(color);
    if (needsScaling()) {
        if (!_internalBuffer) return;
        if (x < 0 || x >= logicalWidth || y < 0 || y >= logicalHeight) return;
        
        // Row-aligned access (XBM format)
        int idx = (y * _logicalStride) + (x >> 3);
        if (c) {
            _internalBuffer[idx] |= (1 << (x & 7));
        } else {
            _internalBuffer[idx] &= ~(1 << (x & 7));
        }
    } else {
        _u8g2->setDrawColor(c);
        _u8g2->drawPixel(x + xOffset, y + yOffset);
    }
}

void IRAM_ATTR pr32::drivers::esp32::U8G2_Drawer::drawLine(int x1, int y1, int x2, int y2, uint16_t color) {
    if (!_u8g2) return;
    if (needsScaling()) {
        BaseDrawSurface::drawLine(x1, y1, x2, y2, color);
    } else {
        _u8g2->setDrawColor(rgb565To1Bit(color));
        _u8g2->drawLine(x1 + xOffset, y1 + yOffset, x2 + xOffset, y2 + yOffset);
    }
}

void IRAM_ATTR pr32::drivers::esp32::U8G2_Drawer::drawRectangle(int x, int y, int w, int h, uint16_t color) {
    if (!_u8g2) return;
    if (needsScaling()) {
        BaseDrawSurface::drawRectangle(x, y, w, h, color);
    } else {
        _u8g2->setDrawColor(rgb565To1Bit(color));
        _u8g2->drawFrame(x + xOffset, y + yOffset, w, h);
    }
}

void IRAM_ATTR pr32::drivers::esp32::U8G2_Drawer::drawFilledRectangle(int x, int y, int w, int h, uint16_t color) {
    if (!_u8g2) return;
    if (needsScaling()) {
        BaseDrawSurface::drawFilledRectangle(x, y, w, h, color);
    } else {
        _u8g2->setDrawColor(rgb565To1Bit(color));
        _u8g2->drawBox(x + xOffset, y + yOffset, w, h);
    }
}

void IRAM_ATTR pr32::drivers::esp32::U8G2_Drawer::drawCircle(int x0, int y0, int r, uint16_t color) {
    if (!_u8g2) return;
    if (needsScaling()) {
        BaseDrawSurface::drawCircle(x0, y0, r, color);
    } else {
        _u8g2->setDrawColor(rgb565To1Bit(color));
        _u8g2->drawCircle(x0 + xOffset, y0 + yOffset, r);
    }
}

void IRAM_ATTR pr32::drivers::esp32::U8G2_Drawer::drawFilledCircle(int x0, int y0, int r, uint16_t color) {
    if (!_u8g2) return;
    if (needsScaling()) {
        BaseDrawSurface::drawFilledCircle(x0, y0, r, color);
    } else {
        _u8g2->setDrawColor(rgb565To1Bit(color));
        _u8g2->drawDisc(x0 + xOffset, y0 + yOffset, r);
    }
}

void IRAM_ATTR pr32::drivers::esp32::U8G2_Drawer::drawBitmap(int x, int y, int w, int h, const uint8_t *bitmap, uint16_t color) {
    if (!_u8g2) return;
    if (needsScaling()) {
        BaseDrawSurface::drawBitmap(x, y, w, h, bitmap, color);
    } else {
        _u8g2->setDrawColor(rgb565To1Bit(color));
        _u8g2->drawXBM(x + xOffset, y + yOffset, w, h, bitmap);
    }
}

// --------------------------------------------------
// Scaling logic
// --------------------------------------------------

void pr32::drivers::esp32::U8G2_Drawer::buildScaleLUTs() {
    freeScalingBuffers();
    if (!needsScaling()) return;

    // Build LUTs
    _xLUT = new uint16_t[physicalWidth];
    _yLUT = new uint16_t[physicalHeight];

    for (int i = 0; i < physicalWidth; ++i) {
        _xLUT[i] = (i * logicalWidth) / physicalWidth;
    }
    for (int i = 0; i < physicalHeight; ++i) {
        _yLUT[i] = (i * logicalHeight) / physicalHeight;
    }

    // Allocate internal buffer (1 bit per pixel, row aligned for XBM compatibility)
    // XBM format requires each row to be byte-aligned.
    _logicalStride = (logicalWidth + 7) / 8;
    size_t bufferSize = _logicalStride * logicalHeight;

#ifdef ESP32
    _internalBuffer = (uint8_t*)heap_caps_malloc(bufferSize, MALLOC_CAP_8BIT);
#else
    _internalBuffer = new uint8_t[bufferSize];
#endif
    if (_internalBuffer) {
        std::memset(_internalBuffer, 0, bufferSize);
    }

    // Allocate physical buffer for scaling (1 bit per pixel, row aligned)
    _physicalStride = (physicalWidth + 7) / 8;
    size_t physBufferSize = _physicalStride * physicalHeight;
#ifdef ESP32
    _physicalBuffer = (uint8_t*)heap_caps_malloc(physBufferSize, MALLOC_CAP_8BIT);
#else
    _physicalBuffer = new uint8_t[physBufferSize];
#endif
    if (_physicalBuffer) {
        std::memset(_physicalBuffer, 0, physBufferSize);
    }
}

void pr32::drivers::esp32::U8G2_Drawer::freeScalingBuffers() {
    if (_internalBuffer) {
#ifdef ESP32
        heap_caps_free(_internalBuffer);
#else
        delete[] _internalBuffer;
#endif
        _internalBuffer = nullptr;
    }
    if (_physicalBuffer) {
#ifdef ESP32
        heap_caps_free(_physicalBuffer);
#else
        delete[] _physicalBuffer;
#endif
        _physicalBuffer = nullptr;
    }
    if (_xLUT) {
        delete[] _xLUT;
        _xLUT = nullptr;
    }
    if (_yLUT) {
        delete[] _yLUT;
        _yLUT = nullptr;
    }
}

void IRAM_ATTR pr32::drivers::esp32::U8G2_Drawer::sendBufferScaled() {
    if (!_internalBuffer) return;

#ifdef PIXELROOT32_ENABLE_PROFILING
    uint32_t start = micros();
#endif

    _u8g2->clearBuffer();

    // 1:1 Mapping with Offset (Bypass scaling if offsets are defined)
    // This allows centering a logical resolution on a larger physical screen without stretching.
    // 1:1 Mapping with Offset (Bypass scaling if offsets are defined)
    // This allows centering a logical resolution on a larger physical screen without stretching.
    if (xOffset != 0 || yOffset != 0) {
        // Zero-copy attempt: If no rotation and 1:1, we could write to u8g2 buffer.
        // For now, keep drawXBM as it handles the "Page/Tile" conversion which is complex.
        _u8g2->setDrawColor(1);
        _u8g2->drawXBM(xOffset, yOffset, logicalWidth, logicalHeight, _internalBuffer);
    } 
    // Optimized 2x Fast-Path (Logical -> Physical 2x)
    else if (_internalBuffer && physicalWidth == logicalWidth * 2 && physicalHeight == logicalHeight * 2) {
        // Pre-calculated nibble-to-byte expansion table
        // Map 4 bits ABCD -> Byte AABBCCDD (XBM order: bit 0 is leftmost)
        static const uint8_t expandLUT[16] = {
            0x00, 0x03, 0x0C, 0x0F, 0x30, 0x33, 0x3C, 0x3F,
            0xC0, 0xC3, 0xCC, 0xCF, 0xF0, 0xF3, 0xFC, 0xFF
        };

        for (int ly = 0; ly < logicalHeight; ++ly) {
            const uint8_t* srcRow = _internalBuffer + (ly * _logicalStride);
            uint8_t* dstRow1 = _physicalBuffer + ((ly * 2) * _physicalStride);
            uint8_t* dstRow2 = _physicalBuffer + ((ly * 2 + 1) * _physicalStride);

            for (int lx = 0; lx < _logicalStride; ++lx) {
                uint8_t s = srcRow[lx];
                // Expand 8 bits to 2 bytes (16 bits)
                uint8_t d1 = expandLUT[s & 0x0F];       // Low nibble (pixels 0-3)
                uint8_t d2 = expandLUT[(s >> 4) & 0x0F]; // High nibble (pixels 4-7)
                
                dstRow1[lx * 2] = d1;
                dstRow1[lx * 2 + 1] = d2;
                dstRow2[lx * 2] = d1;
                dstRow2[lx * 2 + 1] = d2;
            }
        }
        _u8g2->setDrawColor(1);
        _u8g2->drawXBM(0, 0, physicalWidth, physicalHeight, _physicalBuffer);
    }
    // Scaling needed (Logical != Physical)
    else if (_xLUT && _yLUT && _physicalBuffer) {
        // Map Logical -> Physical using LUTs (Nearest Neighbor)
        // Optimized: Process by PHYSICAL bytes instead of bits to reduce stores and avoid memset
        for (int physY = 0; physY < physicalHeight; ++physY) {
            const uint8_t* srcRow = _internalBuffer + (_yLUT[physY] * _logicalStride);
            uint8_t* dstRow = _physicalBuffer + (physY * _physicalStride);

            for (int physByte = 0; physByte < _physicalStride; ++physByte) {
                uint8_t b = 0;
                int basePhysX = physByte << 3;

                // Build a physical byte from 8 logical samples
                for (int bit = 0; bit < 8; ++bit) {
                    int px = basePhysX + bit;
                    if (px >= physicalWidth) break;
                    
                    int sx = _xLUT[px];
                    // Check bit in logical row: srcRow[sx / 8] & (1 << (sx % 8))
                    if (srcRow[sx >> 3] & (1 << (sx & 7))) {
                        b |= (1 << bit);
                    }
                }
                dstRow[physByte] = b;
            }
        }
        
        // Blit the constructed physical buffer
        _u8g2->setDrawColor(1);
        _u8g2->drawXBM(0, 0, physicalWidth, physicalHeight, _physicalBuffer);
    }
    
    _u8g2->sendBuffer();

#ifdef PIXELROOT32_ENABLE_PROFILING
    uint32_t elapsed = micros() - start;
    static uint32_t lastReport = 0;
    if (millis() - lastReport > 1000) {
        Serial.printf("[PROFILING] U8G2 Scaled/Offset Transfer: %u us (%u FPS max)\n", elapsed, 1000000 / (elapsed > 0 ? elapsed : 1));
        lastReport = millis();
    }
#endif
}

#endif // PIXELROOT32_USE_U8G2_DRIVER
