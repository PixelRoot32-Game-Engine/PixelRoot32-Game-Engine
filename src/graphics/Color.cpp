/*
 * Copyright (c) 2026 Gabriel Perez
 * Licensed under the GNU GPL v3
 */
#include "graphics/Color.h"
#include "graphics/PaletteDefs.h"

namespace pixelroot32::graphics {

struct PaletteEntry {
    PaletteType type;
    const uint16_t* colors;
};

static constexpr PaletteEntry kPalettes[] = {
    { PaletteType::NES,   PALETTE_NES   },
    { PaletteType::GB,    PALETTE_GB    },
    { PaletteType::GBC,   PALETTE_GBC   },
    { PaletteType::PICO8, PALETTE_PICO8 },
    { PaletteType::PR32,  PALETTE_PR32  },
};


// Current active palette pointer. Defaults to PR32.
static const uint16_t* currentPalette = PALETTE_PR32;

/** 
 * @brief Set palette.
 * @param palette type of palette.
 * 
*/
void setPalette(PaletteType palette) {
    for (const auto& entry : kPalettes) {
        if (entry.type == palette) {
            currentPalette = entry.colors;
            return;
        }
    }
    currentPalette = PALETTE_PR32; // fallback
}

/** 
 * @brief Set custom palette.
 * @param palette array colors.
 * 
 * Note:
 * The engine does NOT copy the palette.
 * The palette pointer must remain valid for the entire usage period.
*/
void setCustomPalette(const uint16_t* palette) {
    if (palette != nullptr) {
        currentPalette = palette;
    }
}

/**
 * @brief Resolves a Color enum to its corresponding 16-bit color value.
 * @param color The Color enum value.
 * @return The 16-bit color value.
 */
uint16_t resolveColor(Color color) {
    uint8_t idx = static_cast<uint8_t>(color);

    // Color::Transparent must be handled by the renderer, not here
    if (idx >= PALETTE_SIZE) {
        return 0; // value is irrelevant, should never be drawn
    }

    return currentPalette[idx];
}

}
