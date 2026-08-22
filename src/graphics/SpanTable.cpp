/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 *
 * @file SpanTable.cpp
 * @brief Implementation of computeSpanTable for 4bpp/2bpp sprites.
 *
 * No dynamic allocation. NOT IRAM_ATTR (init-time only).
 * See SpanTable.h for the lifetime contract and convex-row limitation.
 *
 * Sprite pixel data on ESP32 lives in flash (PROGMEM) and must be read
 * via PIXELROOT32_READ_BYTE_P, which expands to pgm_read_byte() on ESP32
 * and to a plain dereference on desktop/native. Reading flash directly
 * from a raw pointer triggers a LoadStoreError on ESP32.
 */

#include "graphics/SpanTable.h"
#include "platforms/PlatformMemory.h"

namespace pixelroot32::graphics {

void computeSpanTable(Sprite4bpp& s, uint8_t* outMinX, uint8_t* outMaxX) {
    const int width = s.width;
    const int height = s.height;
    const int rowBytes = (width * 4 + 7) / 8;  // 4bpp: 2 nibbles per byte

    for (int row = 0; row < height; ++row) {
        const uint8_t* rowData = s.data + row * rowBytes;
        uint8_t minX = static_cast<uint8_t>(width);
        uint8_t maxX = 0;
        for (int col = 0; col < width; ++col) {
            const uint8_t byte = PIXELROOT32_READ_BYTE_P(&rowData[col >> 1]);
            const uint8_t nibble = (col & 1) ? ((byte >> 4) & 0x0F) : (byte & 0x0F);
            if (nibble != 0) {
                if (static_cast<uint8_t>(col) < minX) minX = static_cast<uint8_t>(col);
                if (static_cast<uint8_t>(col + 1) > maxX) maxX = static_cast<uint8_t>(col + 1);
            }
        }
        if (maxX == 0) {
            outMinX[row] = 0;
            outMaxX[row] = 0;
        } else {
            outMinX[row] = minX;
            outMaxX[row] = maxX;
        }
    }
}

void computeSpanTable(Sprite2bpp& s, uint8_t* outMinX, uint8_t* outMaxX) {
    const int width = s.width;
    const int height = s.height;
    const int rowBytes = (width * 2 + 7) / 8;  // 2bpp: 4 pixels per byte

    for (int row = 0; row < height; ++row) {
        const uint8_t* rowData = s.data + row * rowBytes;
        uint8_t minX = static_cast<uint8_t>(width);
        uint8_t maxX = 0;
        for (int col = 0; col < width; ++col) {
            const uint8_t byte = PIXELROOT32_READ_BYTE_P(&rowData[col >> 2]);
            const uint8_t shift = (3 - (col & 3)) * 2;
            const uint8_t val = (byte >> shift) & 0x03;
            if (val != 0) {
                if (static_cast<uint8_t>(col) < minX) minX = static_cast<uint8_t>(col);
                if (static_cast<uint8_t>(col + 1) > maxX) maxX = static_cast<uint8_t>(col + 1);
            }
        }
        if (maxX == 0) {
            outMinX[row] = 0;
            outMaxX[row] = 0;
        } else {
            outMinX[row] = minX;
            outMaxX[row] = maxX;
        }
    }
}

}  // namespace pixelroot32::graphics
