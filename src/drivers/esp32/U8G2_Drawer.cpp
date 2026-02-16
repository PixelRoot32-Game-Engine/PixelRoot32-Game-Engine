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

    // 1:1 Mapping with Offset (Logical Resolution matches Physical Resolution)
    // Optimization: Direct XBM blit if no scaling is needed (just offset)
    if (xOffset != 0 || yOffset != 0) {
        _u8g2->setDrawColor(1);
        _u8g2->drawXBM(xOffset, yOffset, logicalWidth, logicalHeight, _internalBuffer);
    } 
    // Scaling needed (Logical != Physical)
    else if (_xLUT && _yLUT && _physicalBuffer) {
        // Clear physical buffer first
        std::memset(_physicalBuffer, 0, _physicalStride * physicalHeight);

        // Map Logical -> Physical using LUTs (Nearest Neighbor)
        // Optimized to write directly to XBM-formatted physical buffer
        for (int physY = 0; physY < physicalHeight; ++physY) {
            int srcY = _yLUT[physY];
            // Get pointer to source row
            const uint8_t* srcRow = _internalBuffer + (srcY * _logicalStride);
            
            // Get pointer to dest row
            uint8_t* dstRow = _physicalBuffer + (physY * _physicalStride);

            for (int physX = 0; physX < physicalWidth; ++physX) {
                int srcX = _xLUT[physX];
                
                // Check if source pixel is set
                // srcRow[srcX >> 3] & (1 << (srcX & 7))
                if (srcRow[srcX >> 3] & (1 << (srcX & 7))) {
                    // Set destination pixel
                    // dstRow[physX >> 3] |= (1 << (physX & 7))
                    dstRow[physX >> 3] |= (1 << (physX & 7));
                }
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
