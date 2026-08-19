/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#pragma once

#include "platforms/PlatformDefaults.h"

#if PIXELROOT32_ENABLE_GAMEPLAY_GRID_SPACE

#include "gameplay/GridSpace.h"
#include "math/Vector2.h"

#if PIXELROOT32_ENABLE_PROJECTION
#include "gameplay/Projection.h"
#endif

/**
 * @file GridMotion.h
 * @brief One actor's position while it travels from one grid cell to the next.
 *
 * `GridMotion` holds the LOGICAL cell (the one every gameplay rule reads), the
 * cell currently being travelled to, and a sub-cell progress counter. The four
 * functions here advance that state and project it to a world position. That
 * is the whole capability.
 *
 * What is deliberately NOT here, because it diverges between any two actors
 * that move on a grid and would have to arrive as callbacks:
 *
 * - **Whether a cell may be entered.** A player checking a bomb it just
 *   dropped and an enemy checking the same bomb answer differently.
 * - **Where the next direction comes from.** Held input, an AI draw, a
 *   scripted push — unrelated sources.
 * - **What happens on arrival.** `tickStep()` reports the arrival edge; the
 *   caller decides what it means.
 * - **Input buffering.** No consumer buffers direction input: a grid actor
 *   samples its direction at rest and ignores it in flight, so a queue would
 *   be state nobody reads.
 *
 * Shares `PIXELROOT32_ENABLE_GAMEPLAY_GRID_SPACE` with GridSpace rather than
 * taking a flag of its own, and still does — but the original reason no longer
 * holds and is corrected here rather than left standing. That reason was
 * "interpolatedWorld() takes a `GridSpec`, so motion without space is not a
 * reachable configuration". Since the `ProjectionSpec` overload landed, motion
 * under a projection with no `GridSpec` in sight IS reachable in principle.
 *
 * The flag is deliberately not split anyway: a separate
 * `..._GAMEPLAY_GRID_MOTION` would widen the build matrix for a configuration
 * no consumer has asked for. The consequence is stated plainly instead — the
 * `ProjectionSpec` overload below is guarded on BOTH flags, so an isometric
 * game that wants `GridMotion` must enable the grid flag even if it never
 * declares a `GridSpec`.
 *
 * @code
 * // Per logic step, in the actor:
 * if (isMoving(mv)) {
 *     if (tickStep(mv, kStepsPerCell)) { onArrive(mv.cellX, mv.cellY); }
 * } else if (wantsToMove && canEnter(nx, ny)) {   // canEnter is game code
 *     beginStep(mv, nx, ny);
 * }
 * position = interpolatedWorld(mv, kStepsPerCell, kGrid);
 * @endcode
 */

namespace pixelroot32::gameplay {

/**
 * @struct GridMotion
 * @brief Plain five-`int` aggregate: logical cell, target cell, progress.
 *
 * At rest, `toX == cellX`, `toY == cellY` and `progress == 0`. In flight,
 * `progress` runs 1..stepsPerCell-1 and the logical cell still names the cell
 * being LEFT — it flips only on arrival. Gameplay rules that read the logical
 * cell therefore never see an actor occupying two cells, or none.
 */
struct GridMotion {
    int cellX = 0;     ///< Logical cell X — the one every gameplay rule reads.
    int cellY = 0;     ///< Logical cell Y — the one every gameplay rule reads.
    int toX = 0;       ///< Target cell X; equals cellX when at rest.
    int toY = 0;       ///< Target cell Y; equals cellY when at rest.
    int progress = 0;  ///< 0..stepsPerCell-1; 0 means "at rest in (cellX, cellY)".
};

/// True while a step is in flight, i.e. the sprite sits between two cells.
constexpr bool isMoving(const GridMotion& motion) {
    return motion.progress > 0;
}

/**
 * @brief Places the actor at rest in (cellX, cellY), cancelling any step.
 *
 * Use for spawn, respawn and teleport. Cancelling matters: leaving `progress`
 * non-zero would strand the actor mid-step toward a target it will never
 * reach.
 */
constexpr void placeAt(GridMotion& motion, int cellX, int cellY) {
    motion.cellX = cellX;
    motion.cellY = cellY;
    motion.toX = cellX;
    motion.toY = cellY;
    motion.progress = 0;
}

/**
 * @brief Starts a step toward (toCellX, toCellY).
 *
 * Arms the target and puts the actor at `progress == 1`, so the frame a step
 * starts is already the first frame of travel. The logical cell is untouched.
 *
 * @pre The caller has decided the target is enterable. This function performs
 *      no validation — that policy is game code (see the file comment).
 */
constexpr void beginStep(GridMotion& motion, int toCellX, int toCellY) {
    motion.toX = toCellX;
    motion.toY = toCellY;
    motion.progress = 1;
}

/**
 * @brief Advances an in-flight step by one frame.
 *
 * @param stepsPerCell Frames one cell of travel takes. MUST be >= 1.
 * @return `true` on exactly the frame the logical cell flips to the target,
 *         `false` otherwise — including every call made while at rest.
 *
 * The `true` return is an EDGE, not a state: it fires once per completed step,
 * which is what one-shot arrival reactions (a footstep sound, clearing a
 * pass-through exemption, picking the next direction) need. Calling this at
 * rest is a harmless no-op, so callers need not guard it.
 */
constexpr bool tickStep(GridMotion& motion, int stepsPerCell) {
    if (motion.progress == 0) {
        return false;
    }

    ++motion.progress;
    if (motion.progress < stepsPerCell) {
        return false;
    }

    motion.cellX = motion.toX;
    motion.cellY = motion.toY;
    motion.progress = 0;
    return true;
}

/**
 * @brief World position of the actor, interpolated across the current step.
 *
 * Exact at both endpoints: at rest it equals `cellToWorld(cellX, cellY)`, and
 * because the logical cell flips at the same instant `progress` returns to 0,
 * an arrival lands exactly on the destination cell with no accumulated
 * residue.
 *
 * The lerp truncates toward zero rather than flooring, and that is deliberate
 * — it is NOT the negative-coordinate hazard `worldToCellX()` guards against.
 * Truncation makes the drawn position lag the exact position by under one
 * pixel *symmetrically* in both directions of travel; flooring would make
 * rightward moves lag while leftward moves lead.
 *
 * Costs one integer division per axis per call (`stepsPerCell` is a runtime
 * value, so it cannot be strength-reduced the way GridSpace's division-free
 * conversions are). Callers that need it cheaper can call it once per frame
 * and cache, which is what an actor's update already does.
 *
 * @pre `stepsPerCell >= 1` and `spec` satisfies gridSpecIsValid().
 */
inline math::Vector2 interpolatedWorld(const GridMotion& motion, int stepsPerCell,
                                       const GridSpec& spec) {
    const int fromPx = cellToWorldX(motion.cellX, spec);
    const int fromPy = cellToWorldY(motion.cellY, spec);
    const int toPx = cellToWorldX(motion.toX, spec);
    const int toPy = cellToWorldY(motion.toY, spec);

    const int x = fromPx + (toPx - fromPx) * motion.progress / stepsPerCell;
    const int y = fromPy + (toPy - fromPy) * motion.progress / stepsPerCell;

    return math::Vector2(x, y);
}

#if PIXELROOT32_ENABLE_PROJECTION

/**
 * @brief Same interpolation, under an arbitrary projection instead of an
 *        axis-aligned grid.
 *
 * Projects both endpoints through the `ProjectionSpec` and lerps between them.
 * The `GridMotion` state is used unchanged — `placeAt`, `beginStep`,
 * `tickStep` and `isMoving` know nothing about projections and need no
 * variant. This is the whole reason an isometric game does not reimplement
 * cell-to-cell navigation.
 *
 * Under a non-identity basis a step along one cell axis moves BOTH screen
 * axes, which the `GridSpec` overload cannot express.
 *
 * The lerp truncates toward zero, exactly like the `GridSpec` overload above,
 * and for exactly the same reason: the drawn position lags the exact position
 * by under one pixel *symmetrically* in both directions of travel. That is
 * deliberate and projection-independent — do NOT "fix" it to the flooring
 * division `Projection.h` uses for its inverse mapping. Those solve different
 * problems.
 *
 * Costs one integer division per axis per call, same as the `GridSpec`
 * overload (`stepsPerCell` is a runtime value and cannot be strength-reduced).
 *
 * @pre `stepsPerCell >= 1` and `spec` satisfies projectionSpecIsValid().
 */
inline math::Vector2 interpolatedWorld(const GridMotion& motion, int stepsPerCell,
                                       const ProjectionSpec& spec) {
    const int fromPx = cellToScreenX(motion.cellX, motion.cellY, spec);
    const int fromPy = cellToScreenY(motion.cellX, motion.cellY, spec);
    const int toPx = cellToScreenX(motion.toX, motion.toY, spec);
    const int toPy = cellToScreenY(motion.toX, motion.toY, spec);

    const int x = fromPx + (toPx - fromPx) * motion.progress / stepsPerCell;
    const int y = fromPy + (toPy - fromPy) * motion.progress / stepsPerCell;

    return math::Vector2(x, y);
}

#endif  // PIXELROOT32_ENABLE_PROJECTION

}  // namespace pixelroot32::gameplay

#endif  // PIXELROOT32_ENABLE_GAMEPLAY_GRID_SPACE
