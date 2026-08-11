/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#pragma once

#include <cstdint>

/**
 * @file Rgb444.h
 * @brief 12-bit RGB444 wire packing for the display SPI stream.
 *
 * Display panels are capped at 40 MHz SPI, so the frame push is bus bound: a
 * 240x240 RGB565 frame is 115,200 bytes = 23.04 ms = a 43.4 FPS ceiling.
 *
 * The framebuffer is 8bpp RGB332, so a frame carries at most 256 distinct
 * colours. Sending them as RGB444 (12 bits, two pixels per three bytes) cuts
 * 25% of the bus time and loses nothing: TFT_eSPI expands RGB332 into 8 red
 * levels, 8 green levels and 4 blue levels, and all 8x8x4 = 256 combinations
 * survive the drop to 4 bits per channel without a single collision. That
 * bijectivity is asserted by test/unit/test_rgb444.
 *
 * These helpers are deliberately platform neutral (no TFT_eSPI, no ESP32
 * guard) so the property can be verified by the native test suite.
 */

namespace pixelroot32::graphics {

/**
 * @brief Truncates an RGB565 colour to 12-bit RGB444.
 *
 * Takes the top 4 bits of each RGB565 channel field.
 *
 * @param rgb565 Colour in native-endian RGB565 (R5 G6 B5).
 * @return 12-bit colour in the low bits as 0x0RGB; the high nibble is zero.
 */
inline constexpr uint16_t packRgb565ToRgb444(uint16_t rgb565) {
    return static_cast<uint16_t>(
        ((rgb565 >> 4) & 0x0F00) |  // red5   bits 15..12
        ((rgb565 >> 3) & 0x00F0) |  // green6 bits 10..7
        ((rgb565 >> 1) & 0x000F));  // blue5  bits  4..1
}

/**
 * @brief Writes one pixel pair as the panel's three-byte 12-bit stream.
 *
 * The 12-bit interface (MIPI DCS COLMOD 0x03) carries a continuous nibble
 * stream of R,G,B per pixel, so two pixels fill exactly three bytes:
 *
 *     byte0 = R0 << 4 | G0
 *     byte1 = B0 << 4 | R1
 *     byte2 = G1 << 4 | B1
 *
 * Bits above the low 12 of either colour are ignored, so callers may pass raw
 * palette entries without masking.
 *
 * @param dst   Destination for exactly three bytes. Must not be null.
 * @param left  12-bit colour of the first (lower-x) pixel.
 * @param right 12-bit colour of the second pixel.
 */
inline void packRgb444Pair(uint8_t* dst, uint16_t left, uint16_t right) {
    dst[0] = static_cast<uint8_t>((left >> 4) & 0xFF);
    dst[1] = static_cast<uint8_t>(((left & 0x0F) << 4) | ((right >> 8) & 0x0F));
    dst[2] = static_cast<uint8_t>(right & 0xFF);
}

} // namespace pixelroot32::graphics
