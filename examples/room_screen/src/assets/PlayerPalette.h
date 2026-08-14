// Generated asset for the room_screen example.
// The hero's RGB565 palette, built from colors already present in the exported
// tilemap palettes (BACKGROUND/ITEMS/DETAILS in roomscreen_main_scene.h).
#ifndef ROOMSCREEN_PLAYER_PALETTE_H
#define ROOMSCREEN_PLAYER_PALETTE_H

#include "graphics/Renderer.h"
#include <stdint.h>

/**
 * @file PlayerPalette.h
 * @brief The RGB565 colors the hero renders with, indexed by engine Color slot.
 *
 * Installed with setSpriteCustomPaletteSlot(0, ...). Sprites resolve their own
 * pixel values through PLAYER_PALETTE_MAPPING into these slots, so the hero and
 * the tilemaps never compete for the same 16-entry table.
 *
 * Every color is lifted straight from the tilemap palettes so the hero reads as
 * part of the same world:
 *   - skin  0xFF15 (peach)       from all three tile palettes
 *   - hair  0x654B (orange)      BACKGROUND[3] / ITEMS[6]
 *   - tunic 0x2C0F (green)       BACKGROUND[1] / ITEMS[4] / DETAILS[4]
 *   - pants 0x62F2 (blue)        BACKGROUND[2] / ITEMS[5] / DETAILS[5]
 *   - boots 0x924D (red)         ITEMS[7]
 *   - eyes  0x18E6 (dark blue)   ITEMS[1] / DETAILS[1]
 */

namespace room_screen {

    // --- RGB565 by engine Color slot ---
    static const uint16_t PLAYER_SPRITE_PALETTE_RGB565[16] = {
        0x0000, //  0 Black - unused
        0xFFFF, //  1 White - text and error messages
        0x18E6, //  2 Navy - eyes / outline
        0x62F2, //  3 Blue - pants
        0xFF15, //  4 Cyan - skin
        0x0000, //  5 DarkGreen - unused
        0x0000, //  6 Green - unused
        0x2C0F, //  7 LightGreen - tunic
        0x0000, //  8 Yellow - unused
        0x654B, //  9 Orange - hair
        0x924D, // 10 LightRed - boots
        0xF9C0, // 11 Red - error text - must never resolve to black
        0x0000, // 12 DarkRed - unused
        0x0000, // 13 Purple - unused
        0x0000, // 14 Magenta - unused
        0x0000, // 15 Gray - unused
    };

    // --- 4bpp pixel value to engine Color slot ---
    static const pixelroot32::graphics::Color PLAYER_PALETTE_MAPPING[16] = {
        pixelroot32::graphics::Color::Black,      //  0 transparent - never read
        pixelroot32::graphics::Color::Cyan,       //  1 's' skin
        pixelroot32::graphics::Color::Orange,     //  2 'h' hair
        pixelroot32::graphics::Color::LightGreen, //  3 't' tunic
        pixelroot32::graphics::Color::Blue,       //  4 'p' pants
        pixelroot32::graphics::Color::LightRed,   //  5 'b' boots
        pixelroot32::graphics::Color::Navy,       //  6 'o' eyes / outline
        pixelroot32::graphics::Color::Black,      //  7 unused
        pixelroot32::graphics::Color::Black,      //  8 unused
        pixelroot32::graphics::Color::Black,      //  9 unused
        pixelroot32::graphics::Color::Black,      // 10 unused
        pixelroot32::graphics::Color::Black,      // 11 unused
        pixelroot32::graphics::Color::Black,      // 12 unused
        pixelroot32::graphics::Color::Black,      // 13 unused
        pixelroot32::graphics::Color::Black,      // 14 unused
        pixelroot32::graphics::Color::Black,      // 15 unused
    };

} // namespace room_screen

#endif // ROOMSCREEN_PLAYER_PALETTE_H
