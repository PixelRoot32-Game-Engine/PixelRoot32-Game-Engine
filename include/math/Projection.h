/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#pragma once

#include "platforms/PlatformDefaults.h"

#if PIXELROOT32_ENABLE_PROJECTION

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
 * inline constexpr pixelroot32::math::ProjectionSpec kIso{0, 0, 16, 8, -16, 8};
 * static_assert(pixelroot32::math::projectionSpecIsValid(kIso, kCols, kRows),
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

namespace pixelroot32::math {

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

/**
 * @brief True when plain row-major cell iteration is already a correct
 *        back-to-front paint order under this projection.
 *
 * A caller that walks cells `for y, for x` and draws them with no depth sort
 * at all is not taking a shortcut -- but it does rest on a property of the
 * projection that nothing checks unless it asks this predicate.
 *
 * The proof is two lines. Row-major always draws `(x-1, y)` and `(x, y-1)`
 * before `(x, y)`, and those two are the only neighbours whose sprites a tile
 * at `(x, y)` can overlap. So if a `+1` step along EITHER cell axis moves a
 * tile strictly FORWARD on screen, every tile is drawn after everything it
 * can cover.
 *
 * Strict, not `>= 0`, and the difference is the whole point. A zero
 * component puts two neighbours at EQUAL screen depth, and row-major then
 * resolves that tie by array order rather than by geometry. Harmless for a
 * tileset whose sprites fill exactly one cell and never overlap -- and wrong
 * for extruded blocks. A 40 px wall on a 16 px cell stride has no correct
 * arbitrary order. Flip either sign and row-major paints back to front: a
 * tile that should be covered ends up drawn last, which reads as a sprite
 * bug rather than as a projection one.
 *
 * **Sufficient, not necessary.** `false` does not mean the order is wrong --
 * only that it is unproven. Two of the four layouts in this header's own
 * `ProjectionSpec` doc table return `false` here: Orthogonal `{0, 0, 16, 0, 0,
 * 16}` and Oblique `{0, 0, 16, 0, 8, 16}`, both because `axisXy == 0`. Both
 * are correct row-major in practice, because their art fills its cell and
 * overlaps nothing. A reader who takes `false` to mean "invalid spec" and
 * adds a guard that refuses those two layouts has misread this predicate.
 *
 * Enforcement is caller-side, the same shape as projectionSpecIsValid():
 *
 * @code
 * inline constexpr ProjectionSpec kIso{0, 0, 16, 8, -16, 8};
 * static_assert(rowMajorIsPainterOrder(kIso),
 *               "kIso no longer makes row-major a back-to-front order.");
 * @endcode
 */
constexpr bool rowMajorIsPainterOrder(const ProjectionSpec& spec) {
    return spec.axisXy > 0 && spec.axisYy > 0;
}

/**
 * @struct CellRange
 * @brief Half-open cell-space window `[startCol, endCol) x [startRow, endRow)`
 *        covering a screen rectangle, under a given ProjectionSpec.
 *
 * Field names are deliberately identical to
 * `TilemapDirtyTrackingHelper`'s (`include/graphics/Renderer.h`), so the
 * eventual renderer wiring is a rename-free assignment rather than a mapping.
 * A default-constructed `CellRange` is empty (`start == end == 0` on both
 * axes), matching "nothing to draw".
 */
struct CellRange {
    int startCol = 0;
    int endCol   = 0;
    int startRow = 0;
    int endRow   = 0;
};

/**
 * @brief Cell-space window covering a screen rectangle, for any
 *        ProjectionSpec (orthogonal, isometric, or oblique).
 *
 * The rectangle is given half-open as `(screenX, screenY, screenW, screenH)`
 * and inverted at its FOUR corners using its inclusive last pixel
 * (`screenX + screenW - 1`, `screenY + screenH - 1`), never the exclusive
 * edge. This is not a rounding nicety: the correct question is "which cells
 * contain a pixel of this rectangle", and the last pixel is the last pixel.
 * For an orthogonal spec whose screen extent is an exact multiple of the tile
 * size, the exclusive-edge form overcounts by one cell on that edge (see the
 * `endCol == 15` regression case for a 240px-wide, 16px-tile map: the
 * exclusive form yields 16). This exactly reproduces the shipped culling in
 * `computeTilemapDirtyTracking()` (`include/graphics/Renderer.h`), including
 * its `+ tileWidth - 1` ceiling term, once expressed through
 * `screenToCellX`/`screenToCellY` instead of a hand-derived orthogonal
 * formula.
 *
 * The four-corner min/max is not a heuristic: each of `screenToCellX`/
 * `screenToCellY` is affine in the screen coordinates (Cramer's rule), so it
 * is a linear functional; a linear functional over a convex polygon attains
 * its extrema at a vertex, and the rectangle's four corners are exactly the
 * vertices bounding its interior. `floor` (see `detail::projectionFloorDiv`)
 * is monotone non-decreasing, so composing it with a linear functional
 * preserves "extrema at the vertices". The min/max of the four corner
 * inversions therefore equals the min/max over every point in the rectangle:
 * the returned window is exact and tight, never a loose over-approximation.
 *
 * Under a non-orthogonal basis the set of cells whose parallelogram touches
 * the rectangle is itself a parallelogram, not a rectangle, so an
 * axis-aligned bounding box in cell space necessarily includes cells that
 * are not actually on screen. This is intended overdraw, not a defect: for
 * the documented 2:1 isometric layout on a 240x240 screen the measured
 * factor is about 2.25x (see the AC-5 test), and each rejected cell costs one
 * screen-bounds compare, far cheaper than a sprite decode.
 *
 * **Documented limitation**: this is the window of cells whose parallelogram
 * intersects the rectangle, not the window of cells whose *sprite* overlaps
 * it. Orthogonal tiles exactly fill their cell, so the shipped code needs no
 * padding; isometric tile art commonly overhangs its cell (a tall wall or
 * tree sprite drawn from a half-height diamond footprint). Padding the range
 * by the sprite extent is therefore the caller's responsibility, deliberately
 * not addressed here.
 *
 * @param spec Projection basis. The caller composes the effective origin
 *        (e.g. `spec.originX + viewOriginX`) before calling — this function
 *        takes no separate camera/view origin, so there is exactly one
 *        origin to reason about (see the header's `ProjectionSpec` docs).
 * @param screenX Left edge of the screen rectangle, in pixels.
 * @param screenY Top edge of the screen rectangle, in pixels.
 * @param screenW Width of the screen rectangle, in pixels.
 * @param screenH Height of the screen rectangle, in pixels.
 * @param cols Map width in cells. `endCol` is clamped high to this; `startCol`
 *        has no upper clamp (matches the shipped asymmetry: a start past the
 *        map only makes the range empty once compared against `endCol`).
 * @param rows Map height in cells. Same asymmetric clamp as `cols`/`endCol`.
 * @return An empty `CellRange{}` for any degenerate input (see the table
 *         below), otherwise the half-open window `[startCol, endCol) x
 *         [startRow, endRow)`.
 *
 * | Input | Result |
 * |---|---|
 * | `projectionDet(spec) < 1` | Empty. Never divides — see `projectionFloorDiv`'s precondition. |
 * | `cols <= 0` or `rows <= 0` | Empty. |
 * | `screenW <= 0` or `screenH <= 0` | Empty. |
 * | Rectangle entirely off the map | Empty, after clamping. |
 */
[[nodiscard]] constexpr CellRange cellRangeForScreenRect(const ProjectionSpec& spec,
                                                          int screenX, int screenY,
                                                          int screenW, int screenH,
                                                          int cols, int rows) {
    if (projectionDet(spec) < 1 || cols <= 0 || rows <= 0 || screenW <= 0 || screenH <= 0) {
        return CellRange{};
    }

    const int leftX   = screenX;
    const int rightX  = screenX + screenW - 1;  // Inclusive last pixel.
    const int topY    = screenY;
    const int bottomY = screenY + screenH - 1;  // Inclusive last pixel.

    const int cellX0 = screenToCellX(leftX, topY, spec);
    const int cellX1 = screenToCellX(rightX, topY, spec);
    const int cellX2 = screenToCellX(leftX, bottomY, spec);
    const int cellX3 = screenToCellX(rightX, bottomY, spec);

    const int cellY0 = screenToCellY(leftX, topY, spec);
    const int cellY1 = screenToCellY(rightX, topY, spec);
    const int cellY2 = screenToCellY(leftX, bottomY, spec);
    const int cellY3 = screenToCellY(rightX, bottomY, spec);

    int minCol = cellX0;
    if (cellX1 < minCol) minCol = cellX1;
    if (cellX2 < minCol) minCol = cellX2;
    if (cellX3 < minCol) minCol = cellX3;

    int maxCol = cellX0;
    if (cellX1 > maxCol) maxCol = cellX1;
    if (cellX2 > maxCol) maxCol = cellX2;
    if (cellX3 > maxCol) maxCol = cellX3;

    int minRow = cellY0;
    if (cellY1 < minRow) minRow = cellY1;
    if (cellY2 < minRow) minRow = cellY2;
    if (cellY3 < minRow) minRow = cellY3;

    int maxRow = cellY0;
    if (cellY1 > maxRow) maxRow = cellY1;
    if (cellY2 > maxRow) maxRow = cellY2;
    if (cellY3 > maxRow) maxRow = cellY3;

    CellRange range{};
    range.startCol = minCol;
    range.endCol   = maxCol + 1;  // Half-open.
    range.startRow = minRow;
    range.endRow   = maxRow + 1;  // Half-open.

    // Clamp asymmetrically, mirroring the shipped culling exactly: start
    // clamps low only, end clamps high only. No upper clamp on start, no
    // lower clamp on end — both remain sufficient to make the range empty
    // when the rectangle sits fully off the map (e.g. an unclamped start far
    // above `cols`/`rows` still yields an empty `[start, end)` once `end`
    // stays at its clamped value).
    if (range.startCol < 0) range.startCol = 0;
    if (range.endCol > cols) range.endCol = cols;
    if (range.startRow < 0) range.startRow = 0;
    if (range.endRow > rows) range.endRow = rows;

    return range;
}

}  // namespace pixelroot32::math

#endif  // PIXELROOT32_ENABLE_PROJECTION
