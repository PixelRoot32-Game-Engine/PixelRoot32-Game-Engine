#pragma once

#include "TileFormat.h"

/**
 * @file LinkSprites.h
 * @brief 16x16 player sprites, one per facing.
 *
 * Three bitmaps cover four directions: the west-facing pose is the east-facing
 * one drawn with flipX, exactly as the NES does it. Same 1bpp format and same
 * bit order as the tileset — see OverworldTiles.h.
 *
 * Zero-bit pixels are transparent, so the gaps in the front-facing pose read as
 * eyes and the gap between the legs shows the ground through it.
 */
namespace zelda_overworld {

/// Facing south — toward the camera. Eyes visible.
inline constexpr uint16_t PLAYER_DOWN_DATA[16] = {
    0b0000111111110000,
    0b0001111111111000,
    0b0011111111111100,
    0b0011000000001100,
    0b0011011001101100,
    0b0011000000001100,
    0b0001111111111000,
    0b0000011111100000,
    0b0001111111111000,
    0b0011111111111100,
    0b0111111111111110,
    0b0111111111111110,
    0b0011111111111100,
    0b0001111111111000,
    0b0001100000011000,
    0b0011100000011100,
};

/// Facing north — back of the head, no face.
inline constexpr uint16_t PLAYER_UP_DATA[16] = {
    0b0000111111110000,
    0b0001111111111000,
    0b0011111111111100,
    0b0011111111111100,
    0b0011111111111100,
    0b0011111111111100,
    0b0001111111111000,
    0b0000011111100000,
    0b0001111111111000,
    0b0011111111111100,
    0b0111111111111110,
    0b0111111111111110,
    0b0011111111111100,
    0b0001111111111000,
    0b0001100000011000,
    0b0011100000011100,
};

/// Facing east. Drawn with flipX for west.
inline constexpr uint16_t PLAYER_SIDE_DATA[16] = {
    0b0000111111100000,
    0b0001111111110000,
    0b0011111111111000,
    0b0011111100011000,
    0b0011111100111000,
    0b0011111111111000,
    0b0001111111110000,
    0b0000111111100000,
    0b0001111111110000,
    0b0011111111111000,
    0b0011111111111100,
    0b0011111111111100,
    0b0001111111111000,
    0b0001111111110000,
    0b0001110001110000,
    0b0011110011110000,
};

inline constexpr SceneSprite PLAYER_DOWN = { PLAYER_DOWN_DATA, 16, 16 };
inline constexpr SceneSprite PLAYER_UP   = { PLAYER_UP_DATA,   16, 16 };
inline constexpr SceneSprite PLAYER_SIDE = { PLAYER_SIDE_DATA, 16, 16 };

} // namespace zelda_overworld
