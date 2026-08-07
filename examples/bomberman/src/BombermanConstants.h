#pragma once
#include <cstdint>
#include "gameplay/GridSpace.h"
#include "platforms/EngineConfig.h"

namespace bomberman {

/**
 * @file BombermanConstants.h
 * @brief Bomberman game configuration: geometry, fixed-step timing, entity
 *        ceiling.
 */

namespace gameplay = pixelroot32::gameplay;

/* Input button IDs */
constexpr std::uint8_t BTN_UP = 0;
constexpr std::uint8_t BTN_DOWN = 1;
constexpr std::uint8_t BTN_LEFT = 2;
constexpr std::uint8_t BTN_RIGHT = 3;
constexpr std::uint8_t BTN_BOMB = 4;
constexpr std::uint8_t BTN_RESTART = 5;

/* Board geometry: 13x11 cells at 16px each, horizontally centred on a
 * 240x240 display with a status band reserved above the playfield. */
constexpr int kCols = 13;
constexpr int kRows = 11;
constexpr int kCells = kCols * kRows;
constexpr int kCellSize = 16;
constexpr int kBoardWidth = kCols * kCellSize;    // 208
constexpr int kBoardHeight = kRows * kCellSize;   // 176
constexpr int kBoardOriginX = (DISPLAY_WIDTH - kBoardWidth) / 2;  // 16
constexpr int kStatusBandHeight = 48;
constexpr int kBoardOriginY = kStatusBandHeight;  // 48; bottom margin 240-48-176=16

/// Cell<->world grid for the board. The status band above the board is
/// expressed as originY, so containsCell() rejects that band from the
/// playable area without any rule needing to know it exists.
inline constexpr gameplay::GridSpec kBoardGrid{
    kBoardOriginX, kBoardOriginY, kCellSize, kCellSize, kCols, kRows};
static_assert(gameplay::gridSpecIsValid(kBoardGrid),
              "kBoardGrid exceeds Scalar's range or has an invalid cell size.");

/* Fixed logic step. deltaTime only ever accumulates into a step counter in
 * BombermanScene::update() — every rule function downstream of that
 * accumulator (movement, bombs, explosions, the level clock) takes zero
 * time arguments and reads a step count instead of milliseconds. This keeps
 * every duration below reproducible across two runs given the same seed and
 * the same per-step input sequence, independent of frame pacing. */
constexpr int kLogicStepMs = 20;            // exactly 50 steps/second
constexpr int kMaxLogicStepsPerFrame = 4;   // catch-up clamp: 80ms of backlog

constexpr int kPlayerStepsPerCell = 12;     // 240ms/cell
constexpr int kEnemyStepsPerCell = 20;      // 400ms/cell

/// Player walk-frame animation divisor. mv.progress / kPlayerAnimStepDiv % 3
/// yields the walk frame. 12/4 = 3 distinct frames per cell, matching the
/// 3-frame walk tables in kPlayerWalkDown/Up/Right.
constexpr int kPlayerAnimStepDiv = 4;

/// Enemy walk-frame animation divisor. mv.progress / kEnemyAnimStepDiv % 7
/// yields the walk frame. 20/4 = 5 distinct frames per cell out of the
/// 7-frame walk table in kEnemyBallomWalk.
constexpr int kEnemyAnimStepDiv = 4;
constexpr int kBombFuseSteps = 150;         // 3.00s
constexpr int kBombFlashSteps = 50;         // last 1.00s of the fuse
constexpr int kExplosionSteps = 25;         // 0.50s
constexpr int kStepsPerSecond = 50;

/* Entity ceiling. Scene::addEntity silently drops entities once the scene's
 * fixed array is full: `if (entityCount < MaxEntities) { ... }`, no else,
 * no assert, no return value to check. This static_assert turns "the
 * declared entity set exceeds the cap" into a compile error instead of a
 * silent runtime drop that would only show up as a missing enemy on screen. */
constexpr int kMaxEnemies = 8;
constexpr int kEnemyCount = 4;   // tuneable default; generateLevel() may spawn fewer
constexpr int kSceneEntities = 1 /* board renderer */ + 1 /* player */ + kMaxEnemies;
static_assert(kEnemyCount <= kMaxEnemies, "kEnemyCount exceeds the enemy pool.");
static_assert(kSceneEntities <= pixelroot32::platforms::config::MaxEntities,
              "Scene::addEntity silently drops entities past the cap; raise MAX_ENTITIES or cut actors.");

/* Board generation tuning. Both values are playability guesses, not fixed
 * decisions — a play-through is what settles them, not arithmetic. */
constexpr int kSoftWallPercent = 60;    // percent chance a free cell becomes a SoftWall
constexpr int kEnemySafeDistance = 6;   // Manhattan distance from the player's start cell

/* The player always starts in the top-left corner of the free interior,
 * cell (1,1) — the classic layout's anchor for the reserved start zone in
 * BombermanBoard.cpp. */
constexpr int kPlayerStartCellX = 1;
constexpr int kPlayerStartCellY = 1;

/* Bomb pool. Fixed-size, no dynamic allocation: 8 slots is headroom, not a
 * requirement — with exactly one Bomb power-up per level the reachable
 * maximum a single player can carry is small, but sizing the pool larger
 * than that gives the chain-reaction queue in BombermanBombs.cpp a bound
 * that stays correct if a later change adds more Bomb power-ups. */
constexpr int kMaxBombs = 8;

/* Player starting stats. Fire power is the blast arm length in cells;
 * max bombs is the number of simultaneously active bombs the player may
 * have out at once. Both are fixed constants in this slice — power-ups
 * that raise them are later work. */
constexpr int kDefaultFirePower = 2;
constexpr int kDefaultMaxBombs = 1;
constexpr int kStartingLives = 3;

/* Power-up increments. A Fire pickup adds this many cells to the blast arm
 * length; a Bomb pickup adds one to the simultaneous-bomb limit (that half
 * has no separate constant -- it is always exactly +1). */
constexpr int kFirePowerIncrement = 1;

/**
 * @brief Blast segment type for each exploded cell — written alongside
 *        blastSteps_ by resolveDetonations() and paintArm(), read by
 *        BoardRenderer to select the correct directional sprite.
 *
 * Direction is encoded in the Arm* values (HL/HR/VU/VD), so the renderer
 * can pick the right kARMBASE_<dir> / kARNEXT_<dir> / kTIP<dir> sprite
 * without inferring direction from the cell's coordinates. TipL/R/U/D
 * remain in the enum for backward compatibility with any soft-wall/chain
 * caller that historically wrote them, but the renderer's primary path is
 * now `isTip = (blastDist == blastRange)`; the Tip* values are only used
 * as a fallback when blastRange is unavailable.
 *
 * Observation-only (audit §8.5): never read by rule functions, so
 * logic determinism is unaffected by the contents of this array.
 */
enum class BlastShape : std::uint8_t {
    Center = 0,  ///< Bomb's own cell.
    ArmHL  = 1,  ///< Horizontal arm extending left from center.
    ArmHR  = 2,  ///< Horizontal arm extending right from center.
    ArmVU  = 3,  ///< Vertical arm extending up from center.
    ArmVD  = 4,  ///< Vertical arm extending down from center.
    TipL   = 5,  ///< Leftward-facing tip (legacy/fallback).
    TipR   = 6,  ///< Rightward-facing tip (legacy/fallback).
    TipU   = 7,  ///< Upward-facing tip (legacy/fallback).
    TipD   = 8   ///< Downward-facing tip (legacy/fallback).
};

/* Level countdown. Ticks once per logic step, inside the same pipeline
 * stage as the bomb fuses (BombermanScene::logicStep()); reaching zero
 * costs the player one life through the exact same path as an explosion or
 * an enemy contact -- there is no separate "time's up" state. 180 seconds
 * is a first estimate sized to comfortably cover destroying both
 * power-up-bearing walls, collecting both power-ups, clearing four enemies,
 * and reaching the exit at 240ms/cell; it is a playability guess, not a
 * measured value, since no live play session tuned it. */
constexpr std::uint16_t kInitialCountdownSeconds = 180;

}  // namespace bomberman
