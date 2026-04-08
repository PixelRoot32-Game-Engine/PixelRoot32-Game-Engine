#pragma once

#include "../GameConstants.h"
#include <cstdint>
#include <graphics/Renderer.h>

namespace spaceinvaders {

/**
 * @file AlienSprites.h
 * @brief Bitmap sprite data for alien types (Squid, Crab, Octopus).
 *
 * Sprites use uint16_t bitmaps: 1 = pixel on, 0 = off. Bit 0 is leftmost.
 * Crab uses MultiSprite for layered rendering (body + eyes).
 */

/** Squid (8x8) - Frame 1 */
static const uint16_t SQUID_F1_BITS[] = {
    0x0018, // ...XX...
    0x003C, // ..XXXX..
    0x007E, // .XXXXXX.
    0x00DB, // XX.XX.XX
    0x00FF, // XXXXXXXX
    0x0024, // ..X..X..
    0x005A, // .X.XX.X.
    0x00A5  // X.X..X.X
};
// Frame 2
static const uint16_t SQUID_F2_BITS[] = {
    0x0018, // ...XX...
    0x003C, // ..XXXX..
    0x007E, // .XXXXXX.
    0x00DB, // XX.XX.XX
    0x00FF, // XXXXXXXX
    0x005A, // .X.XX.X.
    0x0081, // X......X
    0x0042  // .X....X.
};

// Crab (11x8)
// Frame 1
static const uint16_t CRAB_F1_BITS[] = {
    0x00C0, // ..XX....... (padded to 11?) No, let's align right for simplicity or use specific bits.
            // Let's use visual bits: 00100000100 -> 0x104
    0x0208, // ..X.....X..
    0x0090, // ...X...X...
    0x03F8, // ..XXXXXXX..
    0x061C, // .XX.XXX.XX.
    0x07FE, // XXXXXXXXXXX
    0x061C, // .XX.XXX.XX.
    0x0208, // ..X.....X..
    0x0000  // ...........
};
// Wait, Crab is 11x8. Let's try again with accurate shapes.
// Crab F1:
// ..X.....X..
// X.........X
// X.XXXXXXX.X
// XXX.XXX.XXX
// XXXXXXXXXXX
// .X.XXXXX.X.
// X.X.....X.X
// ...     ... (bottom empty?)

// Let's use standard bitmaps found online for 2600.
// Crab F1:
// 00100000100 (0x104)
// 00010001000 (0x088)
// 00111111100 (0x1FC)
// 01101110110 (0x376)
// 11111111111 (0x7FF)
// 10111111101 (0x5FD)
// 10100000101 (0x505)
// 00000000000 

// Actually, I'll use a simpler "visual" approach that looks good enough.
// Crab F1 (11x8)
static const uint16_t CRAB_F1_REAL_BITS[] = {
    0x0104, // ..X.....X..
    0x0088, // ...X...X...
    0x01FC, // ..XXXXXXX..
    0x0376, // .XX.XXX.XX.
    0x07FF, // XXXXXXXXXXX
    0x01DD, // ..X.XXX.X..
    0x0208, // .X.......X.
    0x0000
};
// Crab F2
static const uint16_t CRAB_F2_REAL_BITS[] = {
    0x0104,
    0x03F8,
    0x01FC,
    0x0376,
    0x07FF,
    0x02AA, // .X.X...X.X.
    0x0404, // X.........X
    0x0000
};

// Octopus (12x8)
// Frame 1
static const uint16_t OCTOPUS_F1_BITS[] = {
    0x0000,
    0x01E0, // ....XXXX....
    0x07F8, // .XXXXXXXXXX.
    0x09F2, // X..XXXXXX..X
    0x0FFE, // XXXXXXXXXXXX
    0x05A0, // .X.XX..XX.X.
    0x0A50, // X.X......X.X
    0x0210  // ..X......X..
};
// Frame 2
static const uint16_t OCTOPUS_F2_BITS[] = {
    0x0000,
    0x01E0, // ....XXXX....
    0x07F8, // .XXXXXXXXXX.
    0x09F2, // X..XXXXXX..X
    0x0FFE, // XXXXXXXXXXXX
    0x02D0, // ..X.XXXX.X..
    0x0528, // .X.X....X.X.
    0x0000
};

// Sprite descriptors that can be reused by multiple actors.
static const pixelroot32::graphics::Sprite SQUID_F1 = { SQUID_F1_BITS, ALIEN_SQUID_SPRITE_W, ALIEN_SQUID_SPRITE_H };
static const pixelroot32::graphics::Sprite SQUID_F2 = { SQUID_F2_BITS, ALIEN_SQUID_SPRITE_W, ALIEN_SQUID_SPRITE_H };

static const pixelroot32::graphics::Sprite CRAB_F1_REAL = { CRAB_F1_REAL_BITS, ALIEN_CRAB_SPRITE_W, ALIEN_CRAB_SPRITE_H };
static const pixelroot32::graphics::Sprite CRAB_F2_REAL = { CRAB_F2_REAL_BITS, ALIEN_CRAB_SPRITE_W, ALIEN_CRAB_SPRITE_H };

static const pixelroot32::graphics::Sprite OCTOPUS_F1 = { OCTOPUS_F1_BITS, ALIEN_OCTOPUS_SPRITE_W, ALIEN_OCTOPUS_SPRITE_H };
static const pixelroot32::graphics::Sprite OCTOPUS_F2 = { OCTOPUS_F2_BITS, ALIEN_OCTOPUS_SPRITE_W, ALIEN_OCTOPUS_SPRITE_H };

// Example layered sprite using MultiSprite (body + eyes highlight).
// Eyes layer only sets a small subset of pixels, rendered in a second color.
static const uint16_t CRAB_EYES_BITS[] = {
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0240, // simple eye highlight pattern
    0x0000,
    0x0000
};

static const pixelroot32::graphics::SpriteLayer CRAB_LAYERS_F1[] = {
    { CRAB_F1_REAL_BITS, pixelroot32::graphics::Color::Orange },
    { CRAB_EYES_BITS,    pixelroot32::graphics::Color::White }
};

static const pixelroot32::graphics::MultiSprite CRAB_F1_MULTI = {
    ALIEN_CRAB_SPRITE_W,
    ALIEN_CRAB_SPRITE_H,
    CRAB_LAYERS_F1,
    2
};

}
