#pragma once
#include <cstdint>
#include "../GameConstants.h"

#include <graphics/Renderer.h>

namespace spaceinvaders {

/**
 * @file PlayerSprites.h
 * @brief Bitmap sprite data for the player ship.
 *
 * 16-bit rows, 1 = draw, 0 = transparent. Dimensions: 11x8 pixels.
 */
static const uint16_t PLAYER_SHIP_BITS[] = {
    0x0020,  // Row 0:     □□□□■□□□□□□
    0x0070,  // Row 1:     □□■■■□□□□□□
    0x00F8,  // Row 2:     □■■■■■□□□□□
    0x01FC,  // Row 3:     ■■■■■■■□□□□
    0x03DE,  // Row 4:     ■■■■■□■■□□□
    0x03FE,  // Row 5:     ■■■■■■■■□□□
    0x0124,  // Row 6:     □■□□■□□■□□□
    0x0124   // Row 7:     □■□□■□□■□□□
};

static const pixelroot32::graphics::Sprite PLAYER_SHIP_SPRITE = { PLAYER_SHIP_BITS, PLAYER_SPRITE_W, PLAYER_SPRITE_H };

}