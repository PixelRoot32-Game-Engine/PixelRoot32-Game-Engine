/*
 * Copyright (c) 2026 Gabriel Perez
 * Licensed under the GNU GPL v3
 */
#pragma once
#include <cstdint>

namespace pixelroot32::graphics {

enum class PaletteType {
    NES,
    GB,
    GBC,
    PICO8,
    PR32
};

static constexpr uint8_t PALETTE_SIZE = 16;

// Default palette is PR32.
// The Color enum is mapped to the PR32 palette indices (0-15).
// Some legacy colors are aliased to the nearest available color in the 16-color palette.
enum class Color : uint8_t {
    // Standard Colors (PR32 indices)
    Black = 0,
    White = 1,
    Navy = 2,
    Blue = 3,
    Cyan = 4,
    DarkGreen = 5,
    Green = 6,
    LightGreen = 7,
    Yellow = 8,
    Orange = 9,
    LightRed = 10,
    Red = 11,
    DarkRed = 12,
    Purple = 13,
    Magenta = 14,
    Gray = 15,

    // Aliases for compatibility
    DarkBlue = Navy,
    LightBlue = Blue,   // Closest match
    Teal = Cyan,        // Closest match
    Olive = DarkGreen,  // Closest match
    Gold = Yellow,      // Closest match
    Brown = DarkRed,    // Closest match (Maroon)
    Pink = Magenta,
    LightPurple = Magenta,
    Maroon = DarkRed,
    MidGray = Gray,
    LightGray = Gray,
    DarkGray = Gray,
    Silver = Gray,

    // Special

    // Color::Transparent is not a real color.
    // It must be handled by the renderer/blitter and must never be resolved.
    // Using it in drawing primitives results in a no-op.
    Transparent = 255,
    DebugRed = Red,
    DebugGreen = Green,
    DebugBlue = Blue
};


// -----------------------------------------------------------------------------
// Palette system
// -----------------------------------------------------------------------------
//
// Only one palette can be active at a time.
// All Color values are resolved against the currently active palette.
//
// Switching the active palette affects all subsequent draw calls.
// Custom palettes are not copied; the provided palette pointer must remain valid.
//

/**
 * @brief Selects the active color palette.
 * @param palette The palette to use.
 */
void setPalette(PaletteType palette);

/**
 * @brief Sets a custom color palette.
 * @param palette Pointer to an array of 16 uint16_t RGB565 color values.
 *                The array must remain valid (e.g., static or global) as the engine does not copy it.
 */
void setCustomPalette(const uint16_t* palette);

/**
 * @brief Resolves a Color enum to its corresponding 16-bit color value.
 * @note Color::Transparent is not a real color and must not be resolved.
 */
uint16_t resolveColor(Color color);

}
