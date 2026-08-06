#pragma once
#include <cstdint>
#include "BombermanBoard.h"
#include "BombermanConstants.h"

namespace bomberman {

/**
 * @file BombermanBombs.h
 * @brief Bomb pool, fuse ticking, and cross-shaped chain-reaction
 *        detonation.
 *
 * Pure data and pure functions over plain arrays — nothing here touches an
 * Actor or a Scene, so this whole file is callable and inspectable without
 * a live game, exactly like BombermanBoard.h's generateLevel().
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
bool bombAt(const Bomb (&bombs)[kMaxBombs], int cellX, int cellY);

/// Number of currently active (not yet detonated) bombs in the pool.
int activeBombCount(const Bomb (&bombs)[kMaxBombs]);

/// Writes a new bomb into the first free pool slot at (cellX, cellY) with
/// the given blast range and a full fuse. Returns false, changing nothing,
/// if the pool has no free slot or a bomb is already active at that cell.
/// This function knows nothing about any player's own max-simultaneous-bomb
/// limit — that is caller-side policy (see PlayerActor::tryPlaceBomb), kept
/// separate so this pool operation stays reusable by any placer.
bool placeBomb(Bomb (&bombs)[kMaxBombs], int cellX, int cellY, int range);

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
void tickFuses(Bomb (&bombs)[kMaxBombs], uint8_t (&detonationQueue)[kMaxBombs], int& queueTail);

/// Drains `detonationQueue` (already seeded by tickFuses) to a fixed point,
/// painting each detonating bomb's cross into `board`/`blastSteps` and
/// chain-triggering any OTHER active bomb an arm reaches. Returns the total
/// number of bombs that detonated this call.
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
int resolveDetonations(uint8_t (&detonationQueue)[kMaxBombs], int queueTail,
                        Bomb (&bombs)[kMaxBombs],
                        TileType (&board)[kCells],
                        uint8_t (&blastSteps)[kCells],
                        TileType hiddenPowerUp);

/// Decrements every nonzero blastSteps cell by one step. Cells reaching
/// zero simply stop being lethal/drawn from the next call onward — the
/// tile itself was already finalized by resolveDetonations() at
/// propagation time (destroyedInto() runs once, when an arm hits the cell,
/// not when blastSteps expires).
void tickExplosions(uint8_t (&blastSteps)[kCells]);

}  // namespace bomberman
