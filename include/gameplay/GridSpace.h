/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#pragma once

#include "platforms/PlatformDefaults.h"

#if PIXELROOT32_ENABLE_GAMEPLAY_GRID_SPACE

#include "math/Vector2.h"
#include "math/MathUtil.h"

/**
 * @file GridSpace.h
 * @brief Conversion math between cell indices and world positions for a
 *        uniform grid, without ever performing a division on the hot path.
 *
 * `GridSpec` describes a grid's origin, per-axis cell size, and extent as a
 * plain, `constexpr`-constructible aggregate — there is deliberately no
 * `snapToGrid` and no runtime "grid" object: this header is conversion math
 * only, composed on demand by callers (`cellToWorld(worldToCell(p))`).
 *
 * `worldToCellX`/`worldToCellY` always use FLOOR semantics (round toward
 * negative infinity), never C++'s truncate-toward-zero integer division —
 * the negative-coordinate hazard a naive `/` would get silently wrong. The
 * Scalar overloads floor first via `math::floorToInt()` and then reuse the
 * exact same int primitive: no `Fixed16::operator/` is reachable from any
 * path in this header.
 */

namespace pixelroot32::gameplay {

/**
 * @struct GridSpec
 * @brief Plain six-`int` aggregate describing a grid's origin, per-axis cell
 *        size, and extent (columns/rows). No member functions, no
 *        per-instance runtime state beyond these fields — a `constexpr
 *        GridSpec` costs zero SRAM.
 */
struct GridSpec {
    int originX    = 0;  ///< World-space X of cell (0, 0)'s top-left corner, in pixels.
    int originY    = 0;  ///< World-space Y of cell (0, 0)'s top-left corner, in pixels.
    int cellWidth  = 1;  ///< Cell width in pixels. MUST be >= 1 (see gridSpecIsValid()).
    int cellHeight = 1;  ///< Cell height in pixels. MUST be >= 1 (see gridSpecIsValid()).
    int cols       = 0;  ///< Number of columns, for containsCell() bounds-checking.
    int rows       = 0;  ///< Number of rows, for containsCell() bounds-checking.
};

/**
 * @brief Validates a GridSpec's invariants at compile time or runtime.
 *
 * Checks BOTH that `cellWidth`/`cellHeight` are at least `1` (a RISC-V
 * `div` by zero returns `-1` instead of trapping, so this must be caught
 * here rather than relying on hardware) AND that every corner of the grid's
 * extent stays within Scalar's +/-32767 fixed-point integer range.
 * Enforcement is caller-side, via a `static_assert` at the
 * grid's declaration site — a plain aggregate cannot self-assert its own
 * constructor arguments under `-fno-exceptions`.
 *
 * @code
 * inline constexpr GridSpec kMyGrid{0, 0, 16, 16, 20, 15};
 * static_assert(gridSpecIsValid(kMyGrid), "kMyGrid exceeds Scalar's range or has an invalid cell size.");
 * @endcode
 */
constexpr bool gridSpecIsValid(const GridSpec& spec) {
    constexpr int kScalarMin = -32767;
    constexpr int kScalarMax = 32767;

    if (spec.cellWidth < 1 || spec.cellHeight < 1) {
        return false;
    }

    const int maxX = spec.originX + spec.cols * spec.cellWidth;
    const int maxY = spec.originY + spec.rows * spec.cellHeight;

    if (spec.originX < kScalarMin || spec.originX > kScalarMax) return false;
    if (spec.originY < kScalarMin || spec.originY > kScalarMax) return false;
    if (maxX < kScalarMin || maxX > kScalarMax) return false;
    if (maxY < kScalarMin || maxY > kScalarMax) return false;

    return true;
}

namespace detail {

/**
 * @brief Floor division: `floor(value / divisor)`, for `divisor >= 1`.
 *
 * Branchless (a ternary selecting a bias, not an `if`), exactly ONE `div`,
 * no `rem`. Rejected alternatives: a `rem`-based
 * `q + (r >> 31)` form (RISC-V has no combined div/rem instruction, so that
 * still costs two multi-cycle ops, not one) and `value >> 31` in place of
 * the ternary (signed right shift is implementation-defined in C++17
 * [expr.shift]/3).
 *
 * @pre `divisor >= 1`. Callers only ever pass a validated GridSpec's
 *      `cellWidth`/`cellHeight` (see gridSpecIsValid()), so this is never
 *      checked here.
 */
constexpr int gridFloorDiv(int value, int divisor) {
    const int bias = (value < 0) ? (divisor - 1) : 0;
    return (value - bias) / divisor;
}

}  // namespace detail

/// Converts a cell X index to a world-space X pixel position. Multiply/add
/// only — never divides.
constexpr int cellToWorldX(int cellX, const GridSpec& spec) {
    return spec.originX + cellX * spec.cellWidth;
}

/// Converts a cell Y index to a world-space Y pixel position. Multiply/add
/// only — never divides.
constexpr int cellToWorldY(int cellY, const GridSpec& spec) {
    return spec.originY + cellY * spec.cellHeight;
}

/// Converts a cell index (cellX, cellY) to a world-space position as a
/// `math::Vector2` (Scalar domain, e.g. for `Actor::position`). Composed
/// from cellToWorldX()/cellToWorldY() — never divides.
constexpr math::Vector2 cellToWorld(int cellX, int cellY, const GridSpec& spec) {
    return math::Vector2(cellToWorldX(cellX, spec), cellToWorldY(cellY, spec));
}

/// Maps a world-space X pixel position to its containing cell's X index,
/// using FLOOR semantics (round toward negative infinity) — never
/// C++'s truncate-toward-zero division.
constexpr int worldToCellX(int worldX, const GridSpec& spec) {
    return detail::gridFloorDiv(worldX - spec.originX, spec.cellWidth);
}

/// Maps a world-space Y pixel position to its containing cell's Y index,
/// using FLOOR semantics (round toward negative infinity) — never
/// C++'s truncate-toward-zero division.
constexpr int worldToCellY(int worldY, const GridSpec& spec) {
    return detail::gridFloorDiv(worldY - spec.originY, spec.cellHeight);
}

/// Scalar overload: floors `worldX` via `math::floorToInt()` first, then
/// reuses the exact int primitive above. Exact, not
/// approximate — `floor(floor(x)/n) == floor(x/n)` for integer `n > 0` and
/// an integer origin commutes with floor. `Fixed16::operator/` is never
/// reached from this path. Not `constexpr`: `std::floor` is not `constexpr`
/// in C++17, so `math::floorToInt()` is not either.
inline int worldToCellX(math::Scalar worldX, const GridSpec& spec) {
    return worldToCellX(math::floorToInt(worldX), spec);
}

/// Scalar overload: floors `worldY` via `math::floorToInt()` first, then
/// reuses the exact int primitive above. See worldToCellX()
/// for the full rationale.
inline int worldToCellY(math::Scalar worldY, const GridSpec& spec) {
    return worldToCellY(math::floorToInt(worldY), spec);
}

/// Deleted: `double` is ambiguous against the int and Scalar overloads on
/// native (Scalar == float) but silently resolves to the int overload on
/// the C3 (a user-defined conversion loses to a standard one). Deleting it
/// makes that a hard error identically on both targets.
int worldToCellX(double, const GridSpec&) = delete;

/// Deleted: see worldToCellX(double, const GridSpec&) above.
int worldToCellY(double, const GridSpec&) = delete;

/**
 * @brief Returns true iff (cellX, cellY) is within [0, cols) x [0, rows).
 *
 * Returns `false` for negative indices even though `worldToCellX`/
 * `worldToCellY` can legally produce them — a cell outside the declared
 * extent is out of bounds regardless of sign.
 */
constexpr bool containsCell(int cellX, int cellY, const GridSpec& spec) {
    return cellX >= 0 && cellX < spec.cols && cellY >= 0 && cellY < spec.rows;
}

}  // namespace pixelroot32::gameplay

#endif  // PIXELROOT32_ENABLE_GAMEPLAY_GRID_SPACE
