/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#include <drivers/esp32/U8G2_Drawer.h>

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

bool pr32::drivers::esp32::U8G2_Drawer::init() {
    if (!_u8g2) return false;
    _u8g2->begin();
    setRotation(rotation); // Apply initial rotation
    buildScaleLUTs();
    return true;
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
            std::memset(_internalBuffer, 0, (logicalWidth * logicalHeight + 7) / 8);
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
        int idx = y * logicalWidth + x;
        if (c) {
            _internalBuffer[idx >> 3] |= (1 << (idx & 7));
        } else {
            _internalBuffer[idx >> 3] &= ~(1 << (idx & 7));
        }
    } else {
        _u8g2->setDrawColor(c);
        _u8g2->drawPixel(x, y);
    }
}

void IRAM_ATTR pr32::drivers::esp32::U8G2_Drawer::drawLine(int x1, int y1, int x2, int y2, uint16_t color) {
    if (!_u8g2) return;
    if (needsScaling()) {
        BaseDrawSurface::drawLine(x1, y1, x2, y2, color);
    } else {
        _u8g2->setDrawColor(rgb565To1Bit(color));
        _u8g2->drawLine(x1, y1, x2, y2);
    }
}

void IRAM_ATTR pr32::drivers::esp32::U8G2_Drawer::drawRectangle(int x, int y, int w, int h, uint16_t color) {
    if (!_u8g2) return;
    if (needsScaling()) {
        BaseDrawSurface::drawRectangle(x, y, w, h, color);
    } else {
        _u8g2->setDrawColor(rgb565To1Bit(color));
        _u8g2->drawFrame(x, y, w, h);
    }
}

void IRAM_ATTR pr32::drivers::esp32::U8G2_Drawer::drawFilledRectangle(int x, int y, int w, int h, uint16_t color) {
    if (!_u8g2) return;
    if (needsScaling()) {
        BaseDrawSurface::drawFilledRectangle(x, y, w, h, color);
    } else {
        _u8g2->setDrawColor(rgb565To1Bit(color));
        _u8g2->drawBox(x, y, w, h);
    }
}

void IRAM_ATTR pr32::drivers::esp32::U8G2_Drawer::drawCircle(int x0, int y0, int r, uint16_t color) {
    if (!_u8g2) return;
    if (needsScaling()) {
        BaseDrawSurface::drawCircle(x0, y0, r, color);
    } else {
        _u8g2->setDrawColor(rgb565To1Bit(color));
        _u8g2->drawCircle(x0, y0, r);
    }
}

void IRAM_ATTR pr32::drivers::esp32::U8G2_Drawer::drawFilledCircle(int x0, int y0, int r, uint16_t color) {
    if (!_u8g2) return;
    if (needsScaling()) {
        BaseDrawSurface::drawFilledCircle(x0, y0, r, color);
    } else {
        _u8g2->setDrawColor(rgb565To1Bit(color));
        _u8g2->drawDisc(x0, y0, r);
    }
}

void IRAM_ATTR pr32::drivers::esp32::U8G2_Drawer::drawBitmap(int x, int y, int w, int h, const uint8_t *bitmap, uint16_t color) {
    if (!_u8g2) return;
    if (needsScaling()) {
        BaseDrawSurface::drawBitmap(x, y, w, h, bitmap, color);
    } else {
        _u8g2->setDrawColor(rgb565To1Bit(color));
        _u8g2->drawXBM(x, y, w, h, bitmap);
    }
}

// --------------------------------------------------
// Scaling logic
// --------------------------------------------------

void pr32::drivers::esp32::U8G2_Drawer::buildScaleLUTs() {
    freeScalingBuffers();
    if (!needsScaling()) return;

    // Allocate internal buffer (1 bit per pixel)
    size_t bufferSize = (logicalWidth * logicalHeight + 7) / 8;
#ifdef ESP32
    _internalBuffer = (uint8_t*)heap_caps_malloc(bufferSize, MALLOC_CAP_8BIT);
#else
    _internalBuffer = new uint8_t[bufferSize];
#endif
    if (_internalBuffer) {
        std::memset(_internalBuffer, 0, bufferSize);
    }

    // Build LUTs
    _xLUT = new uint16_t[physicalWidth];
    _yLUT = new uint16_t[physicalHeight];

    for (int i = 0; i < physicalWidth; ++i) {
        _xLUT[i] = (i * logicalWidth) / physicalWidth;
    }
    for (int i = 0; i < physicalHeight; ++i) {
        _yLUT[i] = (i * logicalHeight) / physicalHeight;
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
    if (!_internalBuffer || !_xLUT || !_yLUT) return;

#ifdef PIXELROOT32_ENABLE_PROFILING
    uint32_t start = micros();
#endif

    _u8g2->clearBuffer();
    for (int physY = 0; physY < physicalHeight; ++physY) {
        int srcY = _yLUT[physY];
        int rowOffset = srcY * logicalWidth;
        for (int physX = 0; physX < physicalWidth; ++physX) {
            int srcX = _xLUT[physX];
            int idx = rowOffset + srcX;
            if (_internalBuffer[idx >> 3] & (1 << (idx & 7))) {
                _u8g2->setDrawColor(1);
                _u8g2->drawPixel(physX, physY);
            }
        }
    }
    _u8g2->sendBuffer();

#ifdef PIXELROOT32_ENABLE_PROFILING
    uint32_t elapsed = micros() - start;
    static uint32_t lastReport = 0;
    if (millis() - lastReport > 1000) {
        Serial.printf("[PROFILING] U8G2 Scaled Transfer: %u us (%u FPS max)\n", elapsed, 1000000 / (elapsed > 0 ? elapsed : 1));
        lastReport = millis();
    }
#endif
}

#endif // PIXELROOT32_USE_U8G2_DRIVER
