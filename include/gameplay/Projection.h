/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#pragma once

#include "platforms/PlatformDefaults.h"

#if PIXELROOT32_ENABLE_GAMEPLAY_PROJECTION

/**
 * @file Projection.h
 * @brief Conversion math between cell indices and screen positions for an
 *        arbitrary integer 2x2 basis.
 *
 * `ProjectionSpec` describes where cell (0, 0) lands on screen and how far one
 * step along each cell axis moves. That is the whole model. Orthogonal,
 * isometric 2:1, isometric 1:1, oblique and mirrored layouts are all *values*
 * of this one type — there is deliberately no isometric-specific function,
 * enumeration or template parameter, because a general integer basis costs the
 * identical arithmetic as a hardcoded diamond (four multiplies, two adds) and
 * does not need a new API the first time a game wants a different ratio.
 *
 * Like `GridSpace.h`, this header is conversion math only: a plain,
 * `constexpr`-constructible aggregate plus free functions, no runtime
 * "projection" object, no per-instance state, zero SRAM for a `constexpr` spec.
 *
 * @code
 * // Isometric 2:1 with 32x16 diamond tiles.
 * inline constexpr pixelroot32::gameplay::ProjectionSpec kIso{0, 0, 16, 8, -16, 8};
 * static_assert(pixelroot32::gameplay::projectionSpecIsValid(kIso, kCols, kRows),
 *               "kIso exceeds Scalar's fixed-point range.");
 * @endcode
 *
 * ### Cost, stated precisely
 *
 * `cellToScreenX`/`cellToScreenY` are multiply/add only and never divide.
 * `screenToCellX`/`screenToCellY` invert the basis by Cramer's rule and perform
 * **exactly one integer division per axis** — no remainder, no branch, and
 * never `Fixed16::operator/`.
 *
 * "One integer div" is the honest invariant, and it matches `GridSpace.h`
 * rather than improving on it: `detail::gridFloorDiv` there also performs
 * exactly one `div`. What both headers avoid is `Fixed16::operator/`, which is
 * a 64-bit shift plus a 64-bit divide — a libgcc `__divdi3` call on a 32-bit
 * core. A precomputed Q16 reciprocal of the determinant would remove the `div`
 * but is exact only when the determinant is a power of two, and would be
 * derived state that a hand-written aggregate initializer could desynchronise
 * from the four axis fields. It is deliberately not provided.
 *
 * With a `constexpr` spec the determinant is a compile-time constant, so GCC
 * strength-reduces the division away entirely (for the documented layouts it
 * is 256 or 512, i.e. a shift). No `div` instruction survives in such a build.
 *
 * ### Dependencies
 *
 * This header includes nothing but `PlatformDefaults.h`. In particular it does
 * NOT depend on `PIXELROOT32_ENABLE_GAMEPLAY_GRID_SPACE`: a game may project
 * without ever declaring a `GridSpec`. The one place the two capabilities meet
 * is the `ProjectionSpec` overload of `interpolatedWorld()`, which lives in
 * `GridMotion.h` and is guarded on both flags there.
 */

namespace pixelroot32::gameplay {

/**
 * @struct ProjectionSpec
 * @brief Plain six-`int` aggregate: the screen anchor of cell (0, 0) plus the
 *        two screen-space axis vectors of the cell grid.
 *
 * The two axis vectors form the columns of a 2x2 integer matrix. No member
 * functions, no derived fields, no per-instance runtime state — a `constexpr
 * ProjectionSpec` costs zero SRAM. The determinant is deliberately NOT a
 * field: see projectionDet().
 *
 * Defaults form the identity basis, so a default-constructed spec maps every
 * cell to itself.
 *
 * | Layout | Tile | Value | det |
 * |---|---|---|---|
 * | Orthogonal | 16x16 | `{0, 0, 16, 0, 0, 16}` | 256 |
 * | Isometric 2:1 | 32x16 | `{0, 0, 16, 8, -16, 8}` | 256 |
 * | Isometric 1:1 | 32x32 | `{0, 0, 16, 16, -16, 16}` | 512 |
 * | Oblique | 16x16 | `{0, 0, 16, 0, 8, 16}` | 256 |
 */
struct ProjectionSpec {
    int originX = 0;  ///< Screen X of cell (0, 0)'s anchor, in pixels.
    int originY = 0;  ///< Screen Y of cell (0, 0)'s anchor, in pixels.
    int axisXx  = 1;  ///< Screen X delta per +1 cellX.
    int axisXy  = 0;  ///< Screen Y delta per +1 cellX.
    int axisYx  = 0;  ///< Screen X delta per +1 cellY.
    int axisYy  = 1;  ///< Screen Y delta per +1 cellY.
};

/**
 * @brief Determinant of the basis: `axisXx * axisYy - axisXy * axisYx`.
 *
 * Computed, never stored. A seventh field would be derived state that
 * aggregate initialization — the only construction path — could set
 * inconsistently with the four fields it comes from, and there is no
 * constructor in which to maintain the invariant. Two multiplies and a
 * subtract, folded to a constant for any `constexpr` spec.
 */
constexpr int projectionDet(const ProjectionSpec& spec) {
    return spec.axisXx * spec.axisYy - spec.axisXy * spec.axisYx;
}

namespace detail {

/**
 * @brief Floor division: `floor(value / divisor)`, for `divisor >= 1`.
 *
 * Branchless (a ternary selecting a bias, not an `if`), exactly ONE `div`, no
 * `rem`. The ternary is deliberately NOT written as `value >> 31`: signed
 * right shift is implementation-defined in C++17 [expr.shift]/3, and GCC
 * lowers the ternary to the same instruction pair anyway.
 *
 * @pre `divisor >= 1`. Callers only ever pass a validated spec's determinant
 *      (see projectionSpecIsValid()), so this is never checked here.
 *
 * @note This is a deliberate byte-for-byte duplicate of
 *       `GridSpace.h`'s `detail::gridFloorDiv`, including its rationale.
 *       It cannot simply call that one: `gridFloorDiv` lives inside
 *       `#if PIXELROOT32_ENABLE_GAMEPLAY_GRID_SPACE`, and this capability must
 *       stay usable with the grid flag off. Three duplicated lines beat
 *       coupling two independent opt-in flags. A unit test asserts the two
 *       agree over the full floor table whenever both flags are on — if you
 *       change one, that test is what catches the other.
 */
constexpr int projectionFloorDiv(int value, int divisor) {
    const int bias = (value < 0) ? (divisor - 1) : 0;
    return (value - bias) / divisor;
}

}  // namespace detail

/**
 * @brief Screen X of a cell's anchor. Multiply/add only — never divides.
 *
 * Both cell coordinates are required: under any non-identity basis the screen
 * X of a cell depends on `cellY` as well. A one-coordinate signature would be
 * silently wrong for exactly the case this header exists to serve.
 */
constexpr int cellToScreenX(int cellX, int cellY, const ProjectionSpec& spec) {
    return spec.originX + cellX * spec.axisXx + cellY * spec.axisYx;
}

/// Screen Y of a cell's anchor. Multiply/add only — never divides.
/// See cellToScreenX() for why both cell coordinates are required.
constexpr int cellToScreenY(int cellX, int cellY, const ProjectionSpec& spec) {
    return spec.originY + cellX * spec.axisXy + cellY * spec.axisYy;
}

/**
 * @brief Maps a screen position to the cell X index whose parallelogram
 *        contains it, using FLOOR semantics (round toward negative infinity).
 *
 * Cramer's rule over the basis, then one floor division by the determinant.
 * Exactly one integer `div`; never a `Fixed16` division.
 *
 * Flooring is not a detail. For a screen point one pixel left of cell (0, 0)'s
 * anchor under the isometric 2:1 spec the numerator is `-8`; flooring gives
 * `-1`, while C++'s truncate-toward-zero division would give `0` and place a
 * point outside the map inside cell (0, 0). Touch picking hits this on its
 * first call.
 *
 * @note The numerator stays far inside `int` range: with the projected extent
 *       validated against +/-32767 (see projectionSpecIsValid()) and axis
 *       components being tile half-sizes, the products are bounded by roughly
 *       2^22 and their difference by 2^23.
 */
constexpr int screenToCellX(int screenX, int screenY, const ProjectionSpec& spec) {
    const int dx = screenX - spec.originX;
    const int dy = screenY - spec.originY;
    return detail::projectionFloorDiv(dx * spec.axisYy - dy * spec.axisYx,
                                      projectionDet(spec));
}

/// Maps a screen position to the cell Y index whose parallelogram contains it.
/// See screenToCellX() for the full rationale — same rule, other row.
constexpr int screenToCellY(int screenX, int screenY, const ProjectionSpec& spec) {
    const int dx = screenX - spec.originX;
    const int dy = screenY - spec.originY;
    return detail::projectionFloorDiv(dy * spec.axisXx - dx * spec.axisXy,
                                      projectionDet(spec));
}

/// Deleted: `double` is ambiguous against the int overload on native but
/// silently resolves to it on the C3 (a standard conversion beats a
/// user-defined one), discarding the fractional part without a diagnostic.
/// Deleting it makes such a call a hard error identically on both targets.
int cellToScreenX(double, double, const ProjectionSpec&) = delete;

/// Deleted: see cellToScreenX(double, double, const ProjectionSpec&) above.
int cellToScreenY(double, double, const ProjectionSpec&) = delete;

/// Deleted: see cellToScreenX(double, double, const ProjectionSpec&) above.
int screenToCellX(double, double, const ProjectionSpec&) = delete;

/// Deleted: see cellToScreenX(double, double, const ProjectionSpec&) above.
int screenToCellY(double, double, const ProjectionSpec&) = delete;

/**
 * @brief Validates a ProjectionSpec's invariants against a given extent, at
 *        compile time or runtime.
 *
 * Checks two things:
 *
 * 1. `projectionDet(spec) >= 1`. A determinant of `0` is a degenerate
 *    collinear basis with no inverse — and a RISC-V `div` by zero returns
 *    `-1` instead of trapping, so it must be caught here rather than by
 *    hardware. A *negative* determinant is a left-handed basis: it is
 *    invertible, but it would call detail::projectionFloorDiv() with a
 *    negative divisor, violating its precondition and silently inverting the
 *    floor direction. Express a left-handed layout by swapping the two axis
 *    columns instead; that is exact and free.
 *
 * 2. Every one of the FOUR projected corners `(0,0)`, `(cols,0)`, `(0,rows)`
 *    and `(cols,rows)` stays within Scalar's +/-32767 fixed-point integer
 *    range. Checking `originX + cols * axisXx` the way `gridSpecIsValid()`
 *    does would be wrong here: that form is the maximum only for a
 *    non-negative basis. With `axisYx = -16` the minimum projected X sits at
 *    `(0, rows)`, so the naive form accepts specs whose far edge silently
 *    wraps.
 *
 * `cols`/`rows` are parameters rather than fields because they describe the
 * map's extent, not the projection: one spec can serve several maps.
 *
 * Enforcement is caller-side, via a `static_assert` at the spec's declaration
 * site — a plain aggregate cannot self-assert its own initializers under
 * `-fno-exceptions`.
 *
 * @code
 * inline constexpr ProjectionSpec kIso{0, 0, 16, 8, -16, 8};
 * static_assert(projectionSpecIsValid(kIso, 32, 32), "kIso exceeds Scalar's range.");
 * @endcode
 */
constexpr bool projectionSpecIsValid(const ProjectionSpec& spec, int cols, int rows) {
    constexpr int kScalarMin = -32767;
    constexpr int kScalarMax = 32767;

    if (projectionDet(spec) < 1) {
        return false;
    }
    if (cols < 0 || rows < 0) {
        return false;
    }

    const int cornerX[4] = {
        cellToScreenX(0,    0,    spec),
        cellToScreenX(cols, 0,    spec),
        cellToScreenX(0,    rows, spec),
        cellToScreenX(cols, rows, spec),
    };
    const int cornerY[4] = {
        cellToScreenY(0,    0,    spec),
        cellToScreenY(cols, 0,    spec),
        cellToScreenY(0,    rows, spec),
        cellToScreenY(cols, rows, spec),
    };

    for (int i = 0; i < 4; ++i) {
        if (cornerX[i] < kScalarMin || cornerX[i] > kScalarMax) return false;
        if (cornerY[i] < kScalarMin || cornerY[i] > kScalarMax) return false;
    }

    return true;
}

}  // namespace pixelroot32::gameplay

#endif  // PIXELROOT32_ENABLE_GAMEPLAY_PROJECTION
