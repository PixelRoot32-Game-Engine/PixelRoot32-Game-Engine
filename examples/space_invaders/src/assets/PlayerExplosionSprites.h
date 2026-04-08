#pragma once
#include <cstdint>
#include <graphics/Renderer.h>

namespace spaceinvaders {

/**
 * @file PlayerExplosionSprites.h
 * @brief 8x8 explosion frames for the player death animation.
 *
 * Small frames for efficient rendering on resource-constrained platforms.
 */
static const uint16_t PLAYER_EXPLOSION_F1_BITS[] = {
    0b00011000,
    0b00111100,
    0b01111110,
    0b11111111,
    0b11111111,
    0b01111110,
    0b00111100,
    0b00011000
};

static const uint16_t PLAYER_EXPLOSION_F2_BITS[] = {
    0b00100100,
    0b01011010,
    0b10111101,
    0b11111111,
    0b11111111,
    0b10111101,
    0b01011010,
    0b00100100
};

static const uint16_t PLAYER_EXPLOSION_F3_BITS[] = {
    0b00000000,
    0b00100100,
    0b01000010,
    0b10011001,
    0b10011001,
    0b01000010,
    0b00100100,
    0b00000000
};

static const pixelroot32::graphics::Sprite PLAYER_EXPLOSION_F1 = { PLAYER_EXPLOSION_F1_BITS, 8, 8 };
static const pixelroot32::graphics::Sprite PLAYER_EXPLOSION_F2 = { PLAYER_EXPLOSION_F2_BITS, 8, 8 };
static const pixelroot32::graphics::Sprite PLAYER_EXPLOSION_F3 = { PLAYER_EXPLOSION_F3_BITS, 8, 8 };

static const pixelroot32::graphics::SpriteAnimationFrame PLAYER_EXPLOSION_FRAMES[] = {
    { &PLAYER_EXPLOSION_F1, nullptr },
    { &PLAYER_EXPLOSION_F2, nullptr },
    { &PLAYER_EXPLOSION_F3, nullptr }
};

} // namespace spaceinvaders
