#include "BombermanBoard.h"
#include "math/MathUtil.h"

namespace bomberman {

namespace math = pixelroot32::math;

LevelLayout generateLevel(uint32_t seed, int requestedEnemies, int softWallPercent) {
    math::set_seed(seed);

    LevelLayout layout{};

    // 1. Skeleton: border cells and interior pillars (even x AND even y)
    //    are permanently HardWall (44 + 20 = 64 cells, fixed for every
    //    seed); everything else starts Empty (79 cells). Pillars sit only
    //    at even/even, so every cell with an odd x or an odd y is free in
    //    the skeleton and those cells form one connected corridor lattice —
    //    soft walls sit on top of that lattice but are all destructible, so
    //    no region can ever be permanently unreachable, and generation
    //    needs no separate reachability check.
    for (int y = 0; y < kRows; ++y) {
        for (int x = 0; x < kCols; ++x) {
            const bool isBorder = (x == 0 || y == 0 || x == kCols - 1 || y == kRows - 1);
            const bool isPillar = (x % 2 == 0 && y % 2 == 0);
            layout.board[cellIndex(x, y)] =
                (isBorder || isPillar) ? TileType::HardWall : TileType::Empty;
        }
    }

    // 2. Reserved start zone: the player's start cell (1,1), the classic L
    //    {(2,1), (1,2)}, and the two non-border, non-pillar cells
    //    orthogonally adjacent to the L: (3,1) and (1,3). Exactly 5 cells —
    //    a literal 3x3 block centred on (1,1) is not realizable on this
    //    skeleton, since it would include border cells and the (2,2)
    //    pillar. None of these 5 cells is ever a SoftWall.
    constexpr int kReservedCount = 5;
    constexpr int kReservedX[kReservedCount] = {1, 2, 1, 3, 1};
    constexpr int kReservedY[kReservedCount] = {1, 1, 2, 1, 3};
    bool reserved[kCells] = {};
    for (int i = 0; i < kReservedCount; ++i) {
        reserved[cellIndex(kReservedX[i], kReservedY[i])] = true;
    }

    // 3. Soft walls: roll each remaining free, non-reserved cell once.
    //    Cells that don't roll a SoftWall stay as fallback candidates for
    //    step 4, so the whole pass makes exactly one rand_int() call per
    //    eligible cell regardless of outcome.
    uint8_t softWalls[kCells];
    int softCount = 0;
    uint8_t fallbackCandidates[kCells];
    int fallbackCount = 0;
    for (int i = 0; i < kCells; ++i) {
        if (layout.board[i] != TileType::Empty || reserved[i]) {
            continue;
        }
        if (math::rand_int(0, 99) < softWallPercent) {
            layout.board[i] = TileType::SoftWall;
            softWalls[softCount++] = static_cast<uint8_t>(i);
        } else {
            fallbackCandidates[fallbackCount++] = static_cast<uint8_t>(i);
        }
    }

    // 4. Guarantee at least 2 soft walls by promoting fallback candidates in
    //    scan order — deterministic, cheap, and removes an entire class of
    //    "one unlucky seed bricks the level" bug (no exit/power-up could be
    //    placed with fewer than 2 soft walls).
    for (int i = 0; softCount < 2 && i < fallbackCount; ++i) {
        const uint8_t cell = fallbackCandidates[i];
        layout.board[cell] = TileType::SoftWall;
        softWalls[softCount++] = cell;
    }

    // 5. Exit and power-up: a fixed-draw-count distinct pair over the soft
    //    wall list. This always makes exactly 2 rand_int() calls, so the
    //    number of PRNG draws never depends on the seed.
    const int32_t exitIndex = math::rand_int(0, softCount - 1);
    int32_t powerUpIndex = math::rand_int(0, softCount - 2);
    if (powerUpIndex >= exitIndex) {
        ++powerUpIndex;
    }
    layout.board[softWalls[exitIndex]] = TileType::SoftWallHidingExit;
    layout.board[softWalls[powerUpIndex]] = TileType::SoftWallHidingPowerUp;
    layout.hiddenPowerUp =
        (math::rand_int(0, 1) == 0) ? TileType::PowerUpFire : TileType::PowerUpBomb;

    // 6. Enemies: collect Empty cells at Manhattan distance >=
    //    kEnemySafeDistance from the player's start (1,1), then draw
    //    distinct candidates via a bounded partial shuffle — no rejection
    //    loop, so the draw count is fixed by
    //    min(requestedEnemies, candidateCount). If fewer candidates exist
    //    than requested, spawn as many as exist and record the real count;
    //    never loop looking for a cell that isn't there.
    uint8_t enemyCandidates[kCells];
    int enemyCandidateCount = 0;
    for (int i = 0; i < kCells; ++i) {
        if (layout.board[i] != TileType::Empty) {
            continue;
        }
        const int x = i % kCols;
        const int y = i / kCols;
        const int dx = (x > 1) ? (x - 1) : (1 - x);
        const int dy = (y > 1) ? (y - 1) : (1 - y);
        if (dx + dy >= kEnemySafeDistance) {
            enemyCandidates[enemyCandidateCount++] = static_cast<uint8_t>(i);
        }
    }

    const int toSpawn =
        (requestedEnemies < enemyCandidateCount) ? requestedEnemies : enemyCandidateCount;
    for (int i = 0; i < toSpawn; ++i) {
        const int32_t j = math::rand_int(i, enemyCandidateCount - 1);
        const uint8_t picked = enemyCandidates[j];
        enemyCandidates[j] = enemyCandidates[i];
        enemyCandidates[i] = picked;
        layout.enemyCells[i] = picked;
    }
    layout.enemyCount = static_cast<uint8_t>(toSpawn);

    return layout;
}

}  // namespace bomberman
