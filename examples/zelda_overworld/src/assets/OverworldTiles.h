#pragma once

#include "TileFormat.h"

/**
 * @file OverworldTiles.h
 * @brief The 16x16 tileset every overworld layer indexes into.
 *
 * Iteration 1 art: 1bpp, one bit per pixel, drawn in whatever ink color the
 * layer asks for. Each `data` row is a 16-bit literal written left to right —
 * the leftmost pixel is the most significant bit, which is why the binary
 * literals below read as the picture they draw.
 *
 * Because a 1bpp layer is single-colored, the tiles carry their identity in
 * their *pattern*, not their hue: rock is masonry hatch, trees are a round
 * canopy on a trunk, grass is a pair of tufts, ground is a sparse speckle.
 * That survives the 4bpp migration unchanged — the shapes stay, the palette
 * arrives.
 *
 * @note Tile id 0 is reserved. `Renderer::drawTileMap` treats index 0 as
 *       "nothing here" and skips the cell, so OVERWORLD_TILES[0] exists only
 *       to keep the ids 1-based and is never drawn.
 */
namespace zelda_overworld {

// ---------------------------------------------------------------------------
// Tile ids — what the layer index arrays store.
// ---------------------------------------------------------------------------

inline constexpr uint8_t TILE_EMPTY  = 0;  ///< Reserved: skipped by the renderer.
inline constexpr uint8_t TILE_GROUND = 1;  ///< Walkable sand/dirt.
inline constexpr uint8_t TILE_GRASS  = 2;  ///< Walkable decoration on top of ground.
inline constexpr uint8_t TILE_TREE   = 3;  ///< Blocking.
inline constexpr uint8_t TILE_ROCK   = 4;  ///< Blocking mountain wall.
inline constexpr uint8_t TILE_CAVE   = 5;  ///< Blocking cave mouth (not enterable yet).

inline constexpr uint16_t kTileCount = 6;

// ---------------------------------------------------------------------------
// Bitmaps
// ---------------------------------------------------------------------------

/// Placeholder for id 0. Never reaches the rasterizer.
inline constexpr uint16_t TILE_EMPTY_DATA[16] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

/**
 * Sand: a ~75%-filled dither that tiles seamlessly.
 *
 * Dense on purpose. A 1bpp layer has one ink color, so the only way ground
 * reads as a filled field rather than scattered dots is to fill most of it and
 * let the holes supply the texture.
 */
inline constexpr uint16_t TILE_GROUND_DATA[16] = {
    0b1111111111111111,
    0b1011101110111011,
    0b1111111111111111,
    0b1110111011101110,
    0b1111111111111111,
    0b1011101110111011,
    0b1111111111111111,
    0b1110111011101110,
    0b1111111111111111,
    0b1011101110111011,
    0b1111111111111111,
    0b1110111011101110,
    0b1111111111111111,
    0b1011101110111011,
    0b1111111111111111,
    0b1110111011101110,
};

/// Four small tufts. Walkable — grass is decoration, not an obstacle.
inline constexpr uint16_t TILE_GRASS_DATA[16] = {
    0b0000000000000000,
    0b0000000000000000,
    0b0010000000100000,
    0b0101000001010000,
    0b0010000000100000,
    0b0000000000000000,
    0b0000000000000000,
    0b0000001000000010,
    0b0000010100000101,
    0b0000001000000010,
    0b0000000000000000,
    0b0000000000000000,
    0b0010000000100000,
    0b0101000001010000,
    0b0010000000100000,
    0b0000000000000000,
};

/// Round canopy over a short trunk.
inline constexpr uint16_t TILE_TREE_DATA[16] = {
    0b0000011111100000,
    0b0001111111111000,
    0b0011111111111100,
    0b0111111111111110,
    0b0111110110111110,
    0b1111111111111111,
    0b1111101111011111,
    0b1111111111111111,
    0b0111111111111110,
    0b0111110110111110,
    0b0011111111111100,
    0b0001111111111000,
    0b0000011111100000,
    0b0000000110000000,
    0b0000000110000000,
    0b0000011111100000,
};

/// Masonry hatch. Tiles seamlessly so a block of rock reads as one mass.
inline constexpr uint16_t TILE_ROCK_DATA[16] = {
    0b1111111111111111,
    0b1000100010001000,
    0b1000100010001000,
    0b1111111111111111,
    0b0010001000100010,
    0b0010001000100010,
    0b1111111111111111,
    0b1000100010001000,
    0b1000100010001000,
    0b1111111111111111,
    0b0010001000100010,
    0b0010001000100010,
    0b1111111111111111,
    0b1000100010001000,
    0b1000100010001000,
    0b1111111111111111,
};

/// Rock with an arched hole punched through it — the classic cave mouth.
inline constexpr uint16_t TILE_CAVE_DATA[16] = {
    0b1111111111111111,
    0b1111111111111111,
    0b1111100000011111,
    0b1111000000001111,
    0b1110000000000111,
    0b1110000000000111,
    0b1100000000000011,
    0b1100000000000011,
    0b1100000000000011,
    0b1100000000000011,
    0b1100000000000011,
    0b1100000000000011,
    0b1100000000000011,
    0b1100000000000011,
    0b1100000000000011,
    0b1100000000000011,
};

/**
 * @brief The tileset itself, indexed by tile id.
 *
 * Every layer shares this one array, so a tile id means the same thing on
 * every layer and the collision map can be derived from ids alone.
 */
inline constexpr SceneSprite OVERWORLD_TILES[kTileCount] = {
    { TILE_EMPTY_DATA,  16, 16 },
    { TILE_GROUND_DATA, 16, 16 },
    { TILE_GRASS_DATA,  16, 16 },
    { TILE_TREE_DATA,   16, 16 },
    { TILE_ROCK_DATA,   16, 16 },
    { TILE_CAVE_DATA,   16, 16 },
};

} // namespace zelda_overworld
