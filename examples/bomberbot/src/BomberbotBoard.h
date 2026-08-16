#pragma once
#include <cstdint>
#include "BomberbotConstants.h"

namespace bomberbot {

/**
 * @file BomberbotBoard.h
 * @brief The board's tile type, destruction rule, and deterministic
 *        generation. Pure data and pure functions — nothing here touches an
 *        Actor or a Scene, so generateLevel() is callable and inspectable
 *        without a live game.
 */

/// A cell's contents. Hidden content (the exit, the power-up) lives IN the
/// enum rather than in a parallel array: the three soft-wall variants below
/// are indistinguishable to every rule and to the renderer, which is
/// exactly what "hidden" means, and there is only ever one array that can
/// be wrong.
enum class TileType : uint8_t {
    Empty = 0,
    HardWall,               // border + interior pillar; never changes
    SoftWall,                // destructible, hides nothing
    SoftWallHidingExit,      // destructible, reveals Exit
    SoftWallHidingPowerUp,   // destructible, reveals the level's one power-up
    Exit,                    // revealed; inert until every enemy is dead
    PowerUpFire,
    PowerUpBomb
};

/// True for any of the three soft-wall variants. One range test over the
/// enum's declared order — the renderer draws all three identically, and
/// every blocking/propagation rule treats them identically until destroyed.
constexpr bool isSoftWall(TileType t) {
    return t >= TileType::SoftWall && t <= TileType::SoftWallHidingPowerUp;
}

/// Total function over every TileType value: what a tile becomes once an
/// explosion destroys it. `hiddenPowerUp` supplies the concrete type for
/// SoftWallHidingPowerUp (the level's single power-up, fixed at
/// generation). Every enumerator is handled explicitly so a future addition
/// to TileType fails to compile here instead of silently falling through a
/// default case.
constexpr TileType destroyedInto(TileType t, TileType hiddenPowerUp) {
    switch (t) {
        case TileType::Empty:                  return TileType::Empty;
        case TileType::HardWall:               return TileType::HardWall;
        case TileType::SoftWall:               return TileType::Empty;
        case TileType::SoftWallHidingExit:     return TileType::Exit;
        case TileType::SoftWallHidingPowerUp:  return hiddenPowerUp;
        case TileType::Exit:                   return TileType::Exit;
        case TileType::PowerUpFire:             return TileType::PowerUpFire;
        case TileType::PowerUpBomb:             return TileType::PowerUpBomb;
    }
    return t;  // unreachable for a valid TileType; keeps -Wreturn-type quiet
}

/// Flat-array cell indexing. Every per-cell array over the board (this
/// example's `board[]`, and Phase 3's `blastSteps[]`) uses this one
/// formula — never a second hand-rolled index anywhere else.
constexpr int cellIndex(int x, int y) { return y * kCols + x; }

/// Output of level generation: the board plus everything a caller needs to
/// finish spawning actors.
struct LevelLayout {
    TileType board[kCells] = {};
    TileType hiddenPowerUp = TileType::PowerUpFire;
    uint8_t enemyCells[kMaxEnemies] = {};
    uint8_t enemyCount = 0;
};

/// Deterministic level generation: a pure function of (seed,
/// requestedEnemies, softWallPercent). Calls `math::set_seed(seed)` exactly
/// once and every subsequent random choice comes from `math::rand_int` — no
/// other entropy source is consulted anywhere in this function, and the
/// number of draws it makes never depends on how the dice landed, only on
/// the fixed board geometry and `requestedEnemies`/candidate counts. Two
/// calls with the same seed produce byte-identical output.
LevelLayout generateLevel(uint32_t seed, int requestedEnemies, int softWallPercent);

}  // namespace bomberbot
