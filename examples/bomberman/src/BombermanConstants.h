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

}  // namespace bomberman
