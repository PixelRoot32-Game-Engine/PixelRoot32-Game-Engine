#pragma once
#include <cstdint>

namespace pixelroot32::graphics {

enum class Color : uint8_t {
    Black = 0,
    White,
    LightGray,
    DarkGray,

    Red,
    DarkRed,
    Green,
    DarkGreen,
    Blue,
    DarkBlue,

    Yellow,
    Orange,
    Brown,

    Purple,
    Pink,
    Cyan,

    LightBlue,
    LightGreen,
    LightRed,

    Navy,
    Teal,
    Olive,

    Gold,
    Silver,

    Transparent,   // optional (alpha logic)
    DebugRed,
    DebugGreen,
    DebugBlue,

    COUNT
};

static constexpr uint16_t ENGINE_PALETTE[] = {

    // Grayscale
    0x0000, // Black
    0xFFFF, // White
    0xC618, // LightGray
    0x7BEF, // DarkGray

    // Reds
    0xF800, // Red
    0x7800, // DarkRed

    // Greens
    0x07E0, // Green
    0x03E0, // DarkGreen

    // Blues
    0x001F, // Blue
    0x000F, // DarkBlue

    // Warm colors
    0xFFE0, // Yellow
    0xFD20, // Orange
    0xA145, // Brown

    // Fantasy / UI
    0x780F, // Purple
    0xF81F, // Pink
    0x07FF, // Cyan

    // Light tones
    0xAEDC, // LightBlue
    0x9772, // LightGreen
    0xFBAE, // LightRed

    // Dark tones
    0x0010, // Navy
    0x0410, // Teal
    0x7BE0, // Olive

    // Metallic
    0xFEA0, // Gold
    0xC618, // Silver

    // Special
    0x0000, // Transparent (handled separately)
    0xF800, // DebugRed
    0x07E0, // DebugGreen
    0x001F, // DebugBlue
};

uint16_t resolveColor(Color color);

}
