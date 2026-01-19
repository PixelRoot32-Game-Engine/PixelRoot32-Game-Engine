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
    Transparent = 0, // Treated as Black or handled by renderer logic
    DebugRed = Red,
    DebugGreen = Green,
    DebugBlue = Blue,

    COUNT = 16
};

/**
 * @brief Selects the active color palette.
 * @param palette The palette to use.
 */
void setPalette(PaletteType palette);

// Removed static constexpr ENGINE_PALETTE to support dynamic switching.


uint16_t resolveColor(Color color);

}
