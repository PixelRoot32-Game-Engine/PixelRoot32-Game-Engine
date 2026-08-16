#pragma once
#include <stdint.h>

/**
 * @file PickupSprites.h
 * @brief 4bpp collectible orb, two frames, drawn from the player's palette.
 *
 * ## Why these colour indices
 *
 * The scene runs in dual-palette mode, and Scene::draw() selects the palette
 * context from the entity's render layer: layer 0 gets the Background palette,
 * every other layer gets the Sprite one. The orb is an ordinary actor on the
 * default layer 1, so its colours resolve through PLAYER_SPRITE_PALETTE_RGB565
 * (assets/PlayerPalette.h) — the same table the player sprite uses.
 *
 * That table defines indices 0-7 and leaves 8-15 as 0x0000. A `Color` enum
 * value outside 1..7 therefore renders BLACK here, however sensible its name
 * looks in the PR32 palette (`Color::Yellow` is index 8). The orb sticks to
 * indices 4-7 so it is guaranteed to land on a real colour and, as a bonus,
 * cannot clash with the player's art — it is drawn from the player's own
 * ramp:
 *
 *   4 = RGB(204,  66,  94)  pink/magenta   — shadow ring
 *   5 = RGB(234,  98,  98)  salmon red     — outline
 *   6 = RGB(252, 239, 141)  peach yellow   — bright core
 *   7 = RGB(255, 184, 121)  light peach    — inner ring
 *
 * ## Format
 *
 * 8x8, 4bpp, row stride `(width * 4 + 7) / 8` = 4 bytes. Within a byte the LOW
 * nibble is the even (left) pixel and the HIGH nibble the odd (right) one —
 * see Renderer::drawSpriteInternal(). Index 0 is transparent, never drawn.
 *
 * Declared as `uint8_t` rather than the `uint16_t` the Sprite Compiler emits:
 * `Sprite4bpp::data` is a `const uint8_t*`, so this needs no reinterpret_cast
 * and makes no assumption about byte order.
 */

namespace metroidvania {

// Frame 0 — full glow.
//   . . . 5 5 . . .
//   . . 5 7 7 5 . .
//   . 5 7 6 6 7 5 .
//   5 7 6 6 6 6 7 5
//   5 7 6 6 6 6 7 5
//   . 5 7 6 6 7 5 .
//   . . 5 7 7 5 . .
//   . . . 5 5 . . .
static const uint8_t PICKUP_ORB_SPRITE_0_4BPP[] = {
    0x00, 0x50, 0x05, 0x00,
    0x00, 0x75, 0x57, 0x00,
    0x50, 0x67, 0x76, 0x05,
    0x75, 0x66, 0x66, 0x57,
    0x75, 0x66, 0x66, 0x57,
    0x50, 0x67, 0x76, 0x05,
    0x00, 0x75, 0x57, 0x00,
    0x00, 0x50, 0x05, 0x00,
};

// Frame 1 — the glow pulls in by one ring, so the orb pulses in place.
//   . . . 5 5 . . .
//   . . 5 4 4 5 . .
//   . 5 4 7 7 4 5 .
//   5 4 7 6 6 7 4 5
//   5 4 7 6 6 7 4 5
//   . 5 4 7 7 4 5 .
//   . . 5 4 4 5 . .
//   . . . 5 5 . . .
static const uint8_t PICKUP_ORB_SPRITE_1_4BPP[] = {
    0x00, 0x50, 0x05, 0x00,
    0x00, 0x45, 0x54, 0x00,
    0x50, 0x74, 0x47, 0x05,
    0x45, 0x67, 0x76, 0x54,
    0x45, 0x67, 0x76, 0x54,
    0x50, 0x74, 0x47, 0x05,
    0x00, 0x45, 0x54, 0x00,
    0x00, 0x50, 0x05, 0x00,
};

/// Milliseconds each frame is held. Slow enough to read as a pulse, not a flicker.
static constexpr unsigned long PICKUP_FRAME_DURATION_MS = 300;

} // namespace metroidvania
