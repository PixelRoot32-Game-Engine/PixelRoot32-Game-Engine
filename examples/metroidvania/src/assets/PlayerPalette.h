#pragma once
#include "graphics/Color.h"
#include <stdint.h>

namespace metroidvania {

using pixelroot32::graphics::Color;

// Mapping: sprite index (0-7) -> engine palette index (Color 0-7).
// The engine uses sprite.palette[i] as Color and resolves with the sprite palette (setSpriteCustomPalette).
// So index 0 = first color of our RGB565 palette, etc.
static const Color PLAYER_PALETTE_MAPPING[8] = {
    (Color)0, (Color)1, (Color)2, (Color)3,
    (Color)4, (Color)5, (Color)6, (Color)7,
};

// Actual RGB565 palette for the sprite (per export: Index 0 Transparent, 1-7 RGB in comments).
// Order: index 0 = transparent, 1 = RGB(31,16,42), 2 = RGB(74,48,82), ...
// Conversion: (R>>3)<<11 | (G>>2)<<5 | (B>>3)
static const uint16_t PLAYER_SPRITE_PALETTE_RGB565[16] = {
    0x0000,  // 0: Transparent
    0x1885,  // 1: RGB(31, 16, 42)  - dark purple
    0x498A,  // 2: RGB(74, 48, 82)  - purple
    0xCB8E,  // 3: RGB(201, 115, 115) - salmon
    0xCA0B,  // 4: RGB(204, 66, 94)   - pink/magenta
    0xEB0C,  // 5: RGB(234, 98, 98)   - salmon red
    0xFF71,  // 6: RGB(252, 239, 141) - peach yellow (highlight)
    0xFDAF,  // 7: RGB(255, 184, 121) - light peach/orange
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000
};

} // namespace metroidvania
