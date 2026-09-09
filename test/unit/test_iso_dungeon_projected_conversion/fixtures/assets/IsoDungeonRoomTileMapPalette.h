// GENERATED - do not edit by hand.
#ifndef ISODUNGEONROOMTILEMAP_TILEMAP_PALETTE_H
#define ISODUNGEONROOMTILEMAP_TILEMAP_PALETTE_H

#include "graphics/Renderer.h"
#include <stdint.h>

/**
 * @file IsoDungeonRoomTileMapPalette.h
 * @brief The room tilemap's 16-colour RGB565 palette, in the shape the
 *        Tilemap Editor emits alongside a map.
 *
 * Same three symbols and the same roles as the orthogonal reference export in
 * examples/metroidvania (MetroidvaniaSceneOneTileMapPalette.h): the RGB565
 * table an engine palette slot is loaded with, and the pixel-value -> Color
 * mapping every exported Sprite4bpp points at. Nothing about a palette changes
 * when the map is isometric, which is why this file is a plain rename rather
 * than a new shape.
 *
 * This game installs it with setDualCustomPalette(PAL, PAL) rather than with
 * setBackgroundCustomPalette() alone, so tiles AND sprites resolve through the
 * same table -- one coherent colour scheme, and no reason to split it across
 * slots. That is why the hero and the props include this header too despite
 * its tilemap name, and it is not incidental: drawSprite() resolves through
 * getSpritePaletteSlot() while drawTileMap() resolves through
 * getBackgroundPaletteSlot(), which are backed by separate arrays. A game that
 * loads only the background slot and then draws a sprite from this mapping
 * gets silently wrong colours. See
 * docs/architecture/projected-tilemap-producer-obligations.md.
 *
 * CAUTION: under a custom palette a graphics::Color is a palette INDEX and its
 * name says nothing about what it renders as. Color::White is index 1, which
 * this palette defines as pure black (the void around the room). Reach for the
 * kVoidColor constant in IsoDungeonConstants.h rather than naming a Color
 * directly, or you will paint the backdrop teal.
 *
 * The pixel-value -> Color mapping is deliberately the identity, so
 * TILEMAP_PALETTE_DATA[v] IS the colour of 4bpp pixel value v. Value 0 maps
 * to Color::Black, which the renderer treats as transparent.
 */

namespace iso_dungeon {

    // --- RGB565 by engine Color slot ---
    static const uint16_t TILEMAP_PALETTE_DATA[16] = {
        0x0000, //  0 Black      - transparent - never drawn
        0x0000, //  1 White      - void / backdrop
        0x0862, //  2 Navy       - outline
        0x1AEC, //  3 Blue       - floor dark teal
        0x2C30, //  4 Cyan       - floor light teal
        0x8CE7, //  5 DarkGreen  - floor accent (ritual square)
        0x09A7, //  6 Green      - stone mortar / shadow
        0x1ACC, //  7 LightGreen - stone face, shaded side
        0x2BF0, //  8 Yellow     - stone face, lit side
        0x5596, //  9 Orange     - stone top
        0xFE53, // 10 LightRed   - skin
        0xEDA6, // 11 Red        - hair / gold
        0x3CCB, // 12 DarkRed    - tunic
        0xD6DD, // 13 Purple     - polished stone / metal
        0xF465, // 14 Magenta    - torch flame
        0x6A04, // 15 Gray       - boots
    };

    // --- 4bpp pixel value to engine Color slot (identity) ---
    static const pixelroot32::graphics::Color TILEMAP_PALETTE_MAPPING[16] = {
        pixelroot32::graphics::Color::Black,       //  0 transparent - never drawn
        pixelroot32::graphics::Color::White,       //  1 void / backdrop
        pixelroot32::graphics::Color::Navy,        //  2 outline
        pixelroot32::graphics::Color::Blue,        //  3 floor dark teal
        pixelroot32::graphics::Color::Cyan,        //  4 floor light teal
        pixelroot32::graphics::Color::DarkGreen,   //  5 floor accent (ritual square)
        pixelroot32::graphics::Color::Green,       //  6 stone mortar / shadow
        pixelroot32::graphics::Color::LightGreen,  //  7 stone face, shaded side
        pixelroot32::graphics::Color::Yellow,      //  8 stone face, lit side
        pixelroot32::graphics::Color::Orange,      //  9 stone top
        pixelroot32::graphics::Color::LightRed,    // 10 skin
        pixelroot32::graphics::Color::Red,         // 11 hair / gold
        pixelroot32::graphics::Color::DarkRed,     // 12 tunic
        pixelroot32::graphics::Color::Purple,      // 13 polished stone / metal
        pixelroot32::graphics::Color::Magenta,     // 14 torch flame
        pixelroot32::graphics::Color::Gray,        // 15 boots
    };

} // namespace iso_dungeon

#endif // ISODUNGEONROOMTILEMAP_TILEMAP_PALETTE_H
