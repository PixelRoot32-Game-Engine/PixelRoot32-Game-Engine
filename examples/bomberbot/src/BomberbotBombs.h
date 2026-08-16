#pragma once
#include <cstdint>
#include "BomberbotBoard.h"
#include "BomberbotConstants.h"

namespace bomberbot {

/**
 * @file BomberbotBombs.h
 * @brief Bomb pool, fuse ticking, and cross-shaped chain-reaction
 *        detonation.
 *
 * Pure data and pure functions over plain arrays — nothing here touches an
 * Actor or a Scene, so this whole file is callable and inspectable without
 * a live game, exactly like BomberbotBoard.h's generateLevel().
 *
 * All function bodies are inline in this header (Phase 2 refactor per
 * audit §8.5) so unit tests can include and call them directly without
 * needing to link against a separate translation unit.
 */

/// One pool slot. `range` is snapshotted from the placing player's fire
/// power at placement time and never re-read at detonation, so a Fire
/// pickup collected after a bomb is dropped does not retroactively change
/// that bomb's blast radius.
struct Bomb {
    uint8_t cellX = 0;
    uint8_t cellY = 0;
    uint8_t fuseSteps = 0;
    uint8_t range = 0;
    bool active = false;
};
static_assert(sizeof(Bomb) == 5,
              "Bomb must stay a tight 5-byte record; the 8-slot pool below assumes this.");

/// True iff an active bomb currently occupies (cellX, cellY). Correctly
/// reports false for a cell whose bomb already detonated (active == false),
/// even while that cell's blastSteps entry is still counting down.
inline bool bombAt(const Bomb (&bombs)[kMaxBombs], int cellX, int cellY) {
    for (int i = 0; i < kMaxBombs; ++i) {
        if (bombs[i].active && bombs[i].cellX == cellX && bombs[i].cellY == cellY) {
            return true;
        }
    }
    return false;
}

/// Number of currently active (not yet detonated) bombs in the pool.
inline int activeBombCount(const Bomb (&bombs)[kMaxBombs]) {
    int count = 0;
    for (int i = 0; i < kMaxBombs; ++i) {
        if (bombs[i].active) {
            ++count;
        }
    }
    return count;
}

/// Writes a new bomb into the first free pool slot at (cellX, cellY) with
/// the given blast range and a full fuse. Returns false, changing nothing,
/// if the pool has no free slot or a bomb is already active at that cell.
/// This function knows nothing about any player's own max-simultaneous-bomb
/// limit — that is caller-side policy (see PlayerActor::tryPlaceBomb), kept
/// separate so this pool operation stays reusable by any placer.
inline bool placeBomb(Bomb (&bombs)[kMaxBombs], int cellX, int cellY, int range) {
    if (bombAt(bombs, cellX, cellY)) {
        return false;
    }
    for (int i = 0; i < kMaxBombs; ++i) {
        if (!bombs[i].active) {
            bombs[i].cellX = static_cast<uint8_t>(cellX);
            bombs[i].cellY = static_cast<uint8_t>(cellY);
            bombs[i].fuseSteps = static_cast<uint8_t>(kBombFuseSteps);
            bombs[i].range = static_cast<uint8_t>(range);
            bombs[i].active = true;
            return true;
        }
    }
    return false;  // pool full
}

/// Advances every active bomb's fuse by one step. A bomb whose fuse reaches
/// zero this call is deactivated and its pool index appended to
/// `detonationQueue` — the seed for resolveDetonations() below. This is the
/// only place a bomb's OWN timer enqueues it, and it clears `active` in the
/// same statement that appends the index, which is half of the
/// never-enqueued-twice invariant resolveDetonations() depends on.
/// `queueTail` should be 0 on entry (a fresh call always starts a fresh
/// detonation batch — nothing in this codebase carries a queue across two
/// calls), and every write into `detonationQueue` is bounded against
/// kMaxBombs at the write site, not merely trusted from the caller.
inline void tickFuses(Bomb (&bombs)[kMaxBombs], uint8_t (&detonationQueue)[kMaxBombs], int& queueTail) {
    for (int i = 0; i < kMaxBombs; ++i) {
        if (!bombs[i].active) {
            continue;
        }
        if (bombs[i].fuseSteps > 0) {
            --bombs[i].fuseSteps;
        }
        if (bombs[i].fuseSteps == 0) {
            bombs[i].active = false;
            if (queueTail < kMaxBombs) {  // bounded at the write site
                detonationQueue[queueTail++] = static_cast<uint8_t>(i);
            }
        }
    }
}

namespace detail {

/// Walks one direction from (cx, cy) out to `range` cells, painting
/// blastSteps and applying destruction/chain-triggering per the header's
/// propagation rules. `tail` is bounded against kMaxBombs at the write
/// site — never assumed safe purely because the caller promised it.
///
/// Direction-aware shape (Phase 2+): each painted cell records the
/// directionally-specific arm shape (ArmHL/HR/VU/VD) and writes its
/// `blastDist` (distance from center, 1-based for arm cells) and
/// `blastRange` (the bomb's range). The renderer uses
/// `isTip = (blastDist == blastRange)` to select the Tip sprite
/// instead of trusting the blastShape to be TipL/R/U/D.
///
/// blastShape (Phase 2): each cell that receives blastSteps also records
/// its blast segment type (Center, ArmHL/HR/VU/VD, TipL/R/U/D) so the renderer
/// can select the correct directional explosion sprite. Observation-only
/// per audit §8.5.
inline void paintArm(int cx, int cy, int dx, int dy, int range,
                     TileType (&board)[kCells], uint8_t (&blastSteps)[kCells],
                     uint8_t (&blastShape)[kCells],
                     uint8_t (&blastDist)[kCells],
                     uint8_t (&blastRange)[kCells],
                     uint8_t (&detonationQueue)[kMaxBombs], int& tail,
                     Bomb (&bombs)[kMaxBombs], TileType hiddenPowerUp) {
    int x = cx;
    int y = cy;
    int lastIdx = -1;
    const uint8_t armShape = (dx == 0)
        ? static_cast<uint8_t>(dy < 0 ? BlastShape::ArmVU : BlastShape::ArmVD)
        : static_cast<uint8_t>(dx < 0 ? BlastShape::ArmHL : BlastShape::ArmHR);
    const uint8_t tipShape = (dx == 0)
        ? static_cast<uint8_t>(dy < 0 ? BlastShape::TipU : BlastShape::TipD)
        : static_cast<uint8_t>(dx < 0 ? BlastShape::TipL : BlastShape::TipR);
    const uint8_t rangeU8 = static_cast<uint8_t>(range);

    for (int step = 0; step < range; ++step) {
        x += dx;
        y += dy;
        if (!gameplay::containsCell(x, y, kBoardGrid)) {
            return;  // the border is always HardWall; unreachable in practice
        }
        const int idx = cellIndex(x, y);
        const TileType t = board[idx];

        if (t == TileType::HardWall) {
            return;  // stops the arm; the cell is untouched; no upgrade
        }
        if (isSoftWall(t)) {
            board[idx] = destroyedInto(t, hiddenPowerUp);
            blastSteps[idx] = static_cast<uint8_t>(kExplosionSteps);
            blastShape[idx] = tipShape;  // soft wall cell IS the tip
            blastDist[idx] = static_cast<uint8_t>(step + 1);
            blastRange[idx] = rangeU8;
            return;
        }

        bool chained = false;
        for (int i = 0; i < kMaxBombs; ++i) {
            if (bombs[i].active && bombs[i].cellX == x && bombs[i].cellY == y) {
                bombs[i].active = false;
                if (tail < kMaxBombs) {  // bounded at the write site
                    detonationQueue[tail++] = static_cast<uint8_t>(i);
                }
                chained = true;
                break;
            }
        }
        blastSteps[idx] = static_cast<uint8_t>(kExplosionSteps);
        blastShape[idx] = chained ? tipShape : armShape;  // chain cell IS the tip
        blastDist[idx] = static_cast<uint8_t>(step + 1);
        blastRange[idx] = rangeU8;
        lastIdx = idx;
        if (chained) {
            // The chained bomb produces its own independent cross on a
            // later iteration of the drain loop; this arm stops here so
            // the two crosses never repaint the same cells against each
            // other.
            return;
        }
        // Otherwise: Empty, or a cell whose bomb already detonated earlier
        // in this same drain — paint it and let the arm continue.
    }
    // Out-of-range: upgrade the last painted cell from Arm to Tip.
    if (lastIdx >= 0) {
        blastShape[lastIdx] = tipShape;
    }
}

}  // namespace detail

/// Drains `detonationQueue` (already seeded by tickFuses) to a fixed point,
/// painting each detonating bomb's cross into `board`/`blastSteps` and
/// chain-triggering any OTHER active bomb an arm reaches. Returns the total
/// number of bombs that detonated this call.
///
/// blastShape (Phase 2): written alongside blastSteps. Each cell records
/// its segment type (Center / ArmHL/HR/VU/VD / TipL/R/U/D).
/// blastDist (Phase 2+): 1-based distance from the bomb cell for arm
/// cells, 0 for center cells.
/// blastRange (Phase 2+): the bomb's range, written to every cell the
/// arm touches so the renderer can compute `isTip = (dist == range)`.
/// All three arrays are observation-only; never read by rule functions
/// (audit §8.5).
///
/// TERMINATION ARGUMENT. A pool slot enters detonationQueue in exactly two
/// places in this file: in tickFuses (a fuse reaching zero) and inside this
/// function's arm walk (a chain trigger) — and both do so only in the same
/// statement that clears that slot's own `active` flag, and only when it
/// was previously active. Once `active` is false, neither site will enqueue
/// that slot again, so each of the kMaxBombs pool slots can enter the queue
/// at most once across the whole call. Therefore the queue's write cursor
/// never exceeds kMaxBombs, and the `while (head < tail)` drain loop below
/// runs at most kMaxBombs iterations for ANY topology — including a ring of
/// kMaxBombs mutually-adjacent bombs, where every slot chains into exactly
/// one neighbour and the queue still fills at most once per slot. No
/// recursion, no unbounded growth, no bomb ever processed twice.
inline int resolveDetonations(uint8_t (&detonationQueue)[kMaxBombs], int queueTail,
                               Bomb (&bombs)[kMaxBombs],
                               TileType (&board)[kCells],
                               uint8_t (&blastSteps)[kCells],
                               uint8_t (&blastShape)[kCells],
                               uint8_t (&blastDist)[kCells],
                               uint8_t (&blastRange)[kCells],
                               TileType hiddenPowerUp) {
    int head = 0;
    int tail = (queueTail < kMaxBombs) ? queueTail : kMaxBombs;  // bounded at the write site

    static constexpr int kArmDX[4] = {0, 0, -1, 1};
    static constexpr int kArmDY[4] = {-1, 1, 0, 0};

    while (head < tail) {
        const Bomb b = bombs[detonationQueue[head++]];  // copy: paintArm mutates other slots
        const int idx = cellIndex(b.cellX, b.cellY);
        blastSteps[idx] = static_cast<uint8_t>(kExplosionSteps);
        blastShape[idx] = static_cast<uint8_t>(BlastShape::Center);
        blastDist[idx] = 0;
        blastRange[idx] = b.range;
        for (int d = 0; d < 4; ++d) {
            detail::paintArm(b.cellX, b.cellY, kArmDX[d], kArmDY[d], b.range,
                             board, blastSteps, blastShape,
                             blastDist, blastRange,
                             detonationQueue, tail, bombs, hiddenPowerUp);
        }
    }
    return tail;
}

/// Decrements every nonzero blastSteps cell by one step. Cells reaching
/// zero simply stop being lethal/drawn from the next call onward — the
/// tile itself was already finalized by resolveDetonations() at
/// propagation time (destroyedInto() runs once, when an arm hits the cell,
/// not when blastSteps expires).
inline void tickExplosions(uint8_t (&blastSteps)[kCells]) {
    for (int i = 0; i < kCells; ++i) {
        if (blastSteps[i] > 0) {
            --blastSteps[i];
        }
    }
}

}  // namespace bomberbot
