/*
 * Copyright (c) 2026 Gabriel Perez
 * Licensed under the GNU GPL v3
 */
#include "graphics/Color.h"
#include "graphics/PaletteDefs.h"

namespace pixelroot32::graphics {

// Current active palette pointer. Defaults to PR32.
static const uint16_t* currentPalette = PALETTE_PR32;

void setPalette(PaletteType palette) {
    switch (palette) {
        case PaletteType::NES: currentPalette = PALETTE_NES; break;
        case PaletteType::GB: currentPalette = PALETTE_GB; break;
        case PaletteType::GBC: currentPalette = PALETTE_GBC; break;
        case PaletteType::PICO8: currentPalette = PALETTE_PICO8; break;
        case PaletteType::PR32: currentPalette = PALETTE_PR32; break;
        default: currentPalette = PALETTE_PR32; break;
    }
}

/**
 * @brief Resolves a Color enum to its corresponding 16-bit color value.
 * @param color The Color enum value.
 * @return The 16-bit color value.
 */
uint16_t resolveColor(Color color) {
    uint8_t idx = static_cast<uint8_t>(color);
    if (idx >= static_cast<uint8_t>(Color::COUNT)) {
        return 0xFFFF; // fallback white
    }
    return currentPalette[idx];
}

}
