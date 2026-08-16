#pragma once
#include <cstdint>

#include "assets/OceanTileMap.h"

namespace midway_clone {

// ---------------------------------------------------------------------------
// Screen layout
// ---------------------------------------------------------------------------
// The playfield is anchored at screen y = 0 with the HUD below it, so world y
// maps straight to screen y and the camera offset is the only transform in
// play. A HUD on top would push a constant offset into every world coordinate
// and hit test in the example.

/// Display size in pixels (square 240x240, same panel as every other example).
inline constexpr int kDisplayWidth  = 240;
inline constexpr int kDisplayHeight = 240;

/// Tile size in pixels. Eight, like the machine this is imitating.
inline constexpr int kTileSize = ocean::TILE_SIZE;

/// HUD strip below the playfield.
inline constexpr int kHudHeight = 32;

/// Playfield in pixels — also the camera viewport.
inline constexpr int kPlayfieldWidth  = kDisplayWidth;                 // 240
inline constexpr int kPlayfieldHeight = kDisplayHeight - kHudHeight;   // 208
inline constexpr int kHudY            = kPlayfieldHeight;              // 208

// ---------------------------------------------------------------------------
// World
// ---------------------------------------------------------------------------

inline constexpr int kWorldCols    = ocean::MAP_WIDTH;               // 30
inline constexpr int kWorldRows    = ocean::MAP_HEIGHT;              // 200
inline constexpr int kWorldWidthPx = kWorldCols * kTileSize;         // 240
inline constexpr int kWorldHeightPx = kWorldRows * kTileSize;        // 1600

static_assert(kWorldWidthPx == kPlayfieldWidth,
              "The map is exactly one screen wide. Nothing in this example "
              "scrolls horizontally, and a wider map would silently hide "
              "columns rather than fail.");

/**
 * Camera y at the start of the stage — the bottom of the map.
 *
 * The camera climbs from here toward 0, so the player meets the highest row
 * numbers first and the carrier at rows 10-24 last.
 */
inline constexpr int kCameraStartY = kWorldHeightPx - kPlayfieldHeight;  // 1392

/**
 * Scroll rate in pixels per second.
 *
 * At 32 px/s the 1,392 px of travel takes 43.5 seconds, which is about the
 * length of one NES stage. It is also slow enough that at 30 FPS the world
 * advances one pixel per frame — the scroll never skips, so the whitecaps read
 * as moving water rather than as a jittering pattern.
 */
inline constexpr int kScrollSpeedPxPerSec = 32;

// ---------------------------------------------------------------------------
// Player
// ---------------------------------------------------------------------------

inline constexpr int kPlayerSize = 16;

/**
 * Hitbox inset, per side, in pixels.
 *
 * The player's collision box is deliberately smaller than the sprite: 16x16 of
 * P-38 is mostly wing, and a shmup that kills you for a wingtip graze feels
 * broken rather than hard. Every arcade game of this kind cheats here.
 */
inline constexpr int kPlayerHitboxInset = 4;

inline constexpr int kPlayerSpeedPxPerSec = 110;

/// Player spawn, in playfield-relative pixels.
inline constexpr int kPlayerStartX = (kPlayfieldWidth - kPlayerSize) / 2;
inline constexpr int kPlayerStartY = kPlayfieldHeight - kPlayerSize - 24;

/// Milliseconds between shots while the fire button is held.
inline constexpr unsigned long kPlayerFireIntervalMs = 170;

/**
 * How long one propeller frame is held, in milliseconds.
 *
 * Fast enough to read as a blur, slow enough to survive a 30 FPS frame budget:
 * at 60 ms a frame is visible for about two rendered frames.
 */
inline constexpr unsigned long kPropFrameMs = 60;

/**
 * How long the aircraft holds a banked frame after the stick centres, in ms.
 *
 * Without it the bank flickers on and off as the player taps left and right.
 * The NES holds the roll for a few frames the same way.
 */
inline constexpr unsigned long kBankHoldMs = 90;

inline constexpr int kPlayerLives = 3;

/// Invulnerability after respawning, in milliseconds.
inline constexpr unsigned long kPlayerInvulnMs = 1500;

/// Blink period while invulnerable. The sprite is hidden for half of each.
inline constexpr unsigned long kPlayerBlinkMs = 120;

// ---------------------------------------------------------------------------
// Projectiles
// ---------------------------------------------------------------------------

inline constexpr int kBulletSize = 8;
inline constexpr int kPlayerBulletSpeedPxPerSec = 300;
inline constexpr int kEnemyBulletSpeedPxPerSec  = 130;

// ---------------------------------------------------------------------------
// Enemies
// ---------------------------------------------------------------------------

inline constexpr int kEnemySize = 16;

/**
 * Enemy descent rate in WORLD pixels per second.
 *
 * On screen an enemy travels this plus the scroll, because the world is moving
 * under it too: 60 + 32 = 92 px/s, so it crosses the 208 px playfield in about
 * 2.3 seconds. Tuning this number without remembering the scroll is added to it
 * is the easiest way to end up with enemies that flash past unhittably.
 */
inline constexpr int kEnemySpeedPxPerSec = 60;

/// Horizontal sway amplitude and period for a weaving enemy.
inline constexpr int kEnemySwayAmplitudePx = 28;
inline constexpr unsigned long kEnemySwayPeriodMs = 1400;

/// Milliseconds between an enemy's shots. Zero disables its gun.
inline constexpr unsigned long kEnemyFireIntervalMs = 1100;

inline constexpr int kEnemyScore = 100;

// ---------------------------------------------------------------------------
// Explosions
// ---------------------------------------------------------------------------

/// Milliseconds each explosion frame is held. Three frames, so 3x this total.
inline constexpr unsigned long kExplosionFrameMs = 70;

// ---------------------------------------------------------------------------
// Pool sizes
// ---------------------------------------------------------------------------
// These are compile-time and deliberately small. Every slot is storage in .bss
// whether or not it is ever used, and every live slot is an AABB test and a
// blit inside a 33 ms budget that already spends 23 ms pushing the frame.
//
// The bullet pools are sized off the fire rate, not guessed: a player bullet
// crosses the 208 px playfield in 208/300 s = 693 ms, and one leaves every
// 170 ms, so at most 5 are ever in flight. Eight leaves room for a faster gun
// without a silent cap.

inline constexpr uint16_t kMaxPlayerBullets = 8;
inline constexpr uint16_t kMaxEnemyBullets  = 12;
inline constexpr uint16_t kMaxEnemies       = 10;
inline constexpr uint16_t kMaxExplosions    = 6;

// ---------------------------------------------------------------------------
// Input
// ---------------------------------------------------------------------------

/// Button indices matching the InputConfig order (Up, Down, Left, Right, A, B).
inline constexpr std::uint8_t BTN_UP    = 0;
inline constexpr std::uint8_t BTN_DOWN  = 1;
inline constexpr std::uint8_t BTN_LEFT  = 2;
inline constexpr std::uint8_t BTN_RIGHT = 3;
inline constexpr std::uint8_t BTN_A     = 4;  ///< Gun, while flying.
inline constexpr std::uint8_t BTN_B     = 5;  ///< Restart, once the run has ended.

// ---------------------------------------------------------------------------
// Hit testing
// ---------------------------------------------------------------------------

/**
 * @struct Box
 * @brief An integer AABB in world pixels.
 *
 * Not `core::Rect`. Rect carries Scalar coordinates, which every hit test here
 * would have to convert into and back out of, and its intersects() compares
 * with `<` rather than `<=` — so two boxes that merely touch along an edge are
 * reported as overlapping. A shmup runs this test a few hundred times a frame
 * and wants it exact and cheap, which for integer positions means writing it
 * out.
 */
struct Box {
    int x, y, w, h;
};

/// True when two boxes share at least one pixel. Touching edges do not count.
inline bool overlaps(const Box& a, const Box& b) {
    return a.x < b.x + b.w && b.x < a.x + a.w &&
           a.y < b.y + b.h && b.y < a.y + a.h;
}

// ---------------------------------------------------------------------------
// Sub-pixel motion
// ---------------------------------------------------------------------------

/**
 * @brief Advances an integer position at a rate in px/s, carrying the
 *        sub-pixel remainder between frames.
 *
 * Speeds are in px/s and frames arrive in ms, so a frame is worth a fraction of
 * a pixel. Dropping that fraction every frame makes everything measurably
 * slower than its stated speed — at 30 FPS and 32 px/s the scroll would lose
 * about 3% of its travel. Carrying it in an integer keeps the rate exact
 * without putting a float on the hot path, which matters on the non-FPU
 * targets the engine also builds for.
 *
 * @param accumulator Carried remainder, in px*ms. One per moving thing.
 * @param speedPxPerSec Rate. May be negative.
 * @param deltaTime Milliseconds since the last frame.
 * @return Whole pixels to move this frame.
 */
inline int advancePixels(int& accumulator, int speedPxPerSec, unsigned long deltaTime) {
    accumulator += static_cast<int>(deltaTime) * speedPxPerSec;
    // Truncation toward zero is what we want on both signs: the remainder keeps
    // its sign and the next frame carries it, so a negative rate loses no
    // travel either.
    const int steps = accumulator / 1000;
    accumulator -= steps * 1000;
    return steps;
}

} // namespace midway_clone
