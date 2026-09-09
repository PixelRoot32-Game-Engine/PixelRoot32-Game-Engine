/**
 * @file test_math_projection.cpp
 * @brief Unit tests for the canonical math/Projection module and its
 *        gameplay:: alias identity (WU-2b: relocate ProjectionSpec).
 *
 * Covers:
 * - pixelroot32::math::ProjectionSpec is the canonical home of the aggregate
 *   and its free functions (relocated from pixelroot32::gameplay).
 * - pixelroot32::gameplay::ProjectionSpec and its free functions are TRUE
 *   aliases of the math:: entities, not copies — proven at both compile time
 *   (static_assert on type identity, restated here) and by function-ADDRESS
 *   identity via casted function pointers, which a type-level `is_same_v`
 *   cannot catch (a duplicate function with an identical signature would
 *   still pass `is_same_v` on the type, but never on the function address).
 *
 * This file only exercises pixelroot32::math:: functional behavior when
 * PIXELROOT32_ENABLE_PROJECTION is enabled, mirroring the flags-off / flags-on
 * matrix used by test_gameplay_projection.cpp. Functional coverage
 * (round-trips, floor semantics, validity checks) already lives in that file;
 * this suite exists to prove the RELOCATION and the ALIAS, not to duplicate
 * the functional matrix.
 */

#include <unity.h>
#include "../../test_config.h"
#include "platforms/PlatformDefaults.h"

#if PIXELROOT32_ENABLE_PROJECTION

#include "math/Projection.h"
#include "gameplay/Projection.h"

#include <cstdint>
#include <type_traits>

namespace math = pixelroot32::math;
namespace gameplay = pixelroot32::gameplay;

namespace {

constexpr math::ProjectionSpec kIso2to1{0, 0, 16, 8, -16, 8};
static_assert(math::projectionSpecIsValid(kIso2to1, 32, 32), "kIso2to1 must be valid.");

}  // namespace

// ===========================================================================
// Canonical home: pixelroot32::math::ProjectionSpec
// ===========================================================================

void test_math_projection_spec_is_a_plain_six_int_aggregate(void) {
    TEST_ASSERT_EQUAL_UINT32(static_cast<uint32_t>(6 * sizeof(int)),
                             static_cast<uint32_t>(sizeof(math::ProjectionSpec)));
}

void test_math_cell_to_screen_and_screen_to_cell_round_trip(void) {
    const int sx = math::cellToScreenX(3, 2, kIso2to1);
    const int sy = math::cellToScreenY(3, 2, kIso2to1);
    TEST_ASSERT_EQUAL_INT(3, math::screenToCellX(sx, sy, kIso2to1));
    TEST_ASSERT_EQUAL_INT(2, math::screenToCellY(sx, sy, kIso2to1));
}

void test_math_projection_floor_div_floors_toward_negative_infinity(void) {
    // -8 / 16 truncates to 0; floor must give -1 (matches the gameplay
    // capability's documented hazard case).
    TEST_ASSERT_EQUAL_INT(-1, math::detail::projectionFloorDiv(-8, 16));
    TEST_ASSERT_EQUAL_INT(0, math::detail::projectionFloorDiv(0, 16));
    TEST_ASSERT_EQUAL_INT(1, math::detail::projectionFloorDiv(16, 16));
}

// ===========================================================================
// gameplay:: is a TRUE alias of math::, never a duplicate
// ===========================================================================

void test_gameplay_projection_spec_is_the_same_type_as_math_projection_spec(void) {
    // Redundant with the forwarder's own static_assert (which already gates
    // every TU before any test runs), restated here as a runtime-visible
    // assertion so a reviewer sees the identity proven in test output too.
    static_assert(std::is_same_v<gameplay::ProjectionSpec, math::ProjectionSpec>,
                  "gameplay::ProjectionSpec must alias math::ProjectionSpec.");
    TEST_PASS_MESSAGE("gameplay::ProjectionSpec is math::ProjectionSpec (same type).");
}

void test_gameplay_cell_to_screen_x_is_the_same_function_as_math(void) {
    // Function-ADDRESS identity: a type-level is_same_v cannot catch a
    // duplicate free function with an identical signature. Casting both
    // names to the same function-pointer type and comparing addresses can.
    using Fn = int (*)(int, int, const math::ProjectionSpec&);
    const Fn mathFn = &math::cellToScreenX;
    const Fn gameplayFn = &gameplay::cellToScreenX;
    TEST_ASSERT_EQUAL_PTR(reinterpret_cast<void*>(mathFn), reinterpret_cast<void*>(gameplayFn));
}

void test_gameplay_cell_to_screen_y_is_the_same_function_as_math(void) {
    using Fn = int (*)(int, int, const math::ProjectionSpec&);
    const Fn mathFn = &math::cellToScreenY;
    const Fn gameplayFn = &gameplay::cellToScreenY;
    TEST_ASSERT_EQUAL_PTR(reinterpret_cast<void*>(mathFn), reinterpret_cast<void*>(gameplayFn));
}

void test_gameplay_screen_to_cell_x_is_the_same_function_as_math(void) {
    using Fn = int (*)(int, int, const math::ProjectionSpec&);
    const Fn mathFn = &math::screenToCellX;
    const Fn gameplayFn = &gameplay::screenToCellX;
    TEST_ASSERT_EQUAL_PTR(reinterpret_cast<void*>(mathFn), reinterpret_cast<void*>(gameplayFn));
}

void test_gameplay_screen_to_cell_y_is_the_same_function_as_math(void) {
    using Fn = int (*)(int, int, const math::ProjectionSpec&);
    const Fn mathFn = &math::screenToCellY;
    const Fn gameplayFn = &gameplay::screenToCellY;
    TEST_ASSERT_EQUAL_PTR(reinterpret_cast<void*>(mathFn), reinterpret_cast<void*>(gameplayFn));
}

void test_gameplay_projection_det_is_the_same_function_as_math(void) {
    using Fn = int (*)(const math::ProjectionSpec&);
    const Fn mathFn = &math::projectionDet;
    const Fn gameplayFn = &gameplay::projectionDet;
    TEST_ASSERT_EQUAL_PTR(reinterpret_cast<void*>(mathFn), reinterpret_cast<void*>(gameplayFn));
}

void test_gameplay_projection_spec_is_valid_is_the_same_function_as_math(void) {
    using Fn = bool (*)(const math::ProjectionSpec&, int, int);
    const Fn mathFn = &math::projectionSpecIsValid;
    const Fn gameplayFn = &gameplay::projectionSpecIsValid;
    TEST_ASSERT_EQUAL_PTR(reinterpret_cast<void*>(mathFn), reinterpret_cast<void*>(gameplayFn));
}

void test_gameplay_detail_projection_floor_div_is_the_same_function_as_math(void) {
    using Fn = int (*)(int, int);
    const Fn mathFn = &math::detail::projectionFloorDiv;
    const Fn gameplayFn = &gameplay::detail::projectionFloorDiv;
    TEST_ASSERT_EQUAL_PTR(reinterpret_cast<void*>(mathFn), reinterpret_cast<void*>(gameplayFn));
}

// ===========================================================================
// rowMajorIsPainterOrder — sufficient-not-necessary paint-order predicate
// ===========================================================================

namespace {

constexpr math::ProjectionSpec kIso1to1{0, 0, 16, 16, -16, 16};

// AC-3: constexpr-usable. Only the TRUE case is pinned here — a FALSE
// static_assert against a stub would be a build break indistinguishable from
// compile-red, voiding the two-stage TDD requirement (see this file's
// strict-TDD rationale in the header comment).
static_assert(math::rowMajorIsPainterOrder(kIso2to1),
              "kIso2to1 must make row-major iteration a back-to-front paint order.");

}  // namespace

// AC-1: all four ProjectionSpec doc-table layouts, asserted explicitly —
// including the two that are false, so the truth table lives in the test
// file rather than only in prose.
void test_math_row_major_is_painter_order_orthogonal_layout_is_false(void) {
    constexpr math::ProjectionSpec kOrthogonal{0, 0, 16, 0, 0, 16};
    TEST_ASSERT_FALSE(math::rowMajorIsPainterOrder(kOrthogonal));
}

void test_math_row_major_is_painter_order_iso_2to1_layout_is_true(void) {
    TEST_ASSERT_TRUE(math::rowMajorIsPainterOrder(kIso2to1));
}

void test_math_row_major_is_painter_order_iso_1to1_layout_is_true(void) {
    TEST_ASSERT_TRUE(math::rowMajorIsPainterOrder(kIso1to1));
}

void test_math_row_major_is_painter_order_oblique_layout_is_false(void) {
    constexpr math::ProjectionSpec kOblique{0, 0, 16, 0, 8, 16};
    TEST_ASSERT_FALSE(math::rowMajorIsPainterOrder(kOblique));
}

// A negative axis component moves a tile BACKWARD on screen and must never
// read as painter-correct, regardless of the other axis's sign.
void test_math_row_major_is_painter_order_flipped_axis_yy_is_false(void) {
    constexpr math::ProjectionSpec kFlippedYy{0, 0, 16, 8, -16, -8};
    TEST_ASSERT_FALSE(math::rowMajorIsPainterOrder(kFlippedYy));
}

void test_math_row_major_is_painter_order_flipped_axis_xy_is_false(void) {
    constexpr math::ProjectionSpec kFlippedXy{0, 0, 16, -8, -16, 8};
    TEST_ASSERT_FALSE(math::rowMajorIsPainterOrder(kFlippedXy));
}

// AC-1's both-zero case, doubling as AC-2's named pin against the `>= 0`
// mistake: a non-negative predicate would wrongly accept a basis whose axes
// carry zero screen-Y motion (0 >= 0 && 0 >= 0), collapsing every row and
// every column onto the same screen depth with no correct tie-break.
void test_math_row_major_is_painter_order_both_axes_zero_rejects_the_ge_zero_mistake(void) {
    constexpr math::ProjectionSpec kBothZero{0, 0, 16, 0, 0, 0};
    TEST_ASSERT_FALSE(math::rowMajorIsPainterOrder(kBothZero));
}

// AC-2: axisXy == 0 alone (axisYy stays positive) must still be false.
void test_math_row_major_is_painter_order_axis_xy_zero_alone_is_false(void) {
    constexpr math::ProjectionSpec kAxisXyZero{0, 0, 16, 0, -16, 8};
    TEST_ASSERT_FALSE(math::rowMajorIsPainterOrder(kAxisXyZero));
}

// AC-2: axisYy == 0 alone (axisXy stays positive) must still be false.
void test_math_row_major_is_painter_order_axis_yy_zero_alone_is_false(void) {
    constexpr math::ProjectionSpec kAxisYyZero{0, 0, 0, 16, 16, 0};
    TEST_ASSERT_FALSE(math::rowMajorIsPainterOrder(kAxisYyZero));
}

// AC-4: gameplay::rowMajorIsPainterOrder is the SAME function as
// math::rowMajorIsPainterOrder, not a duplicate — proved by function-ADDRESS
// identity, the idiom this file already uses for the other six forwarded names.
void test_gameplay_row_major_is_painter_order_is_the_same_function_as_math(void) {
    using Fn = bool (*)(const math::ProjectionSpec&);
    const Fn mathFn = &math::rowMajorIsPainterOrder;
    const Fn gameplayFn = &gameplay::rowMajorIsPainterOrder;
    TEST_ASSERT_EQUAL_PTR(reinterpret_cast<void*>(mathFn), reinterpret_cast<void*>(gameplayFn));
}

void test_math_projection_zero_cost_when_disabled(void) {
    // Flag ON: the capability is a constexpr aggregate plus free functions, so
    // a constexpr consumer pays zero runtime footprint. The mirror of this
    // test in the #else branch asserts the flag-off property.
    constexpr math::ProjectionSpec kProbe{0, 0, 8, 4, -8, 4};
    static_assert(math::projectionSpecIsValid(kProbe, 4, 4), "probe spec must be constexpr-valid");
    static_assert(math::cellToScreenX(1, 0, kProbe) == 8, "cellToScreenX must be constexpr-evaluable");
    static_assert(math::cellRangeForScreenRect(kProbe, 0, 0, 32, 32, 4, 4).startCol == 0,
                  "cellRangeForScreenRect must be constexpr-evaluable");

    TEST_PASS_MESSAGE(
        "PIXELROOT32_ENABLE_PROJECTION=1: math::ProjectionSpec stays a plain "
        "six-int aggregate and every free function, including "
        "cellRangeForScreenRect, is constexpr-evaluable, so a constexpr "
        "consumer pays zero runtime footprint.");
}

// ===========================================================================
// cellRangeForScreenRect — characterization against the shipped culling
// ===========================================================================

namespace {

/**
 * @brief Oracle reproducing the shipped viewport culling verbatim.
 *
 * Copied character-for-character from `computeTilemapDirtyTracking()`
 * (include/graphics/Renderer.h:1348-1361), substituting local parameter names
 * for the original `h.*`/`map.*` member accesses. This is the only correct
 * way to characterize the new function against the old one: re-deriving the
 * expressions from the doc comment would risk encoding today's understanding
 * of the old code rather than the old code itself.
 */
struct OrthogonalCullOracle {
    int startCol = 0;
    int endCol   = 0;
    int startRow = 0;
    int endRow   = 0;
};

OrthogonalCullOracle orthogonalCullOracle(int viewOriginX, int viewOriginY, int tileWidth,
                                          int tileHeight, int mapWidth, int mapHeight,
                                          int logicalWidth, int logicalHeight) {
    OrthogonalCullOracle h{};

    // Verbatim from include/graphics/Renderer.h:1348-1361 ("Viewport culling").
    h.startCol = (viewOriginX < 0) ? (-viewOriginX / tileWidth) : 0;
    h.endCol   = (viewOriginX + mapWidth * tileWidth > logicalWidth)
                     ? ((logicalWidth - viewOriginX + tileWidth - 1) / tileWidth)
                     : mapWidth;
    h.startRow = (viewOriginY < 0) ? (-viewOriginY / tileHeight) : 0;
    h.endRow   = (viewOriginY + mapHeight * tileHeight > logicalHeight)
                     ? ((logicalHeight - viewOriginY + tileHeight - 1) / tileHeight)
                     : mapHeight;

    if (h.startCol < 0) h.startCol = 0;
    if (h.endCol > mapWidth) h.endCol = mapWidth;
    if (h.startRow < 0) h.startRow = 0;
    if (h.endRow > mapHeight) h.endRow = mapHeight;

    return h;
}

/**
 * @brief Asserts one axis of a returned range against the oracle, per AC-2.
 *
 * When the oracle's iteration range is non-empty, raw integer equality MUST
 * hold. When the oracle's range is empty, only the new range's emptiness is
 * asserted — C++ truncation-toward-zero (old code) versus this function's
 * corner-inversion ceiling can produce different negative integers that are
 * both "empty" (see the two dedicated divergence tests below), and asserting
 * raw equality there would be wrong per the proposal's AC-2.
 */
void assertAxisRangeMatchesOracle(int oracleStart, int oracleEnd, int actualStart,
                                  int actualEnd, const char* what) {
    if (oracleStart < oracleEnd) {
        TEST_ASSERT_EQUAL_INT_MESSAGE(oracleStart, actualStart, what);
        TEST_ASSERT_EQUAL_INT_MESSAGE(oracleEnd, actualEnd, what);
    } else {
        TEST_ASSERT_TRUE_MESSAGE(actualStart >= actualEnd, what);
    }
}

struct CharacterizationCase {
    const char* name;
    int viewOriginX;
    int viewOriginY;
    int tileWidth;
    int tileHeight;
    int mapWidth;
    int mapHeight;
    int logicalWidth;
    int logicalHeight;
};

// AC-4 coverage, per axis: origin zero/positive/negative; screen extent an
// exact multiple of the tile size and not; map smaller/equal/larger than the
// screen; map entirely off-screen left and right.
constexpr CharacterizationCase kCharacterizationCases[] = {
    {"origin x zero",           0,   0, 16, 16, 20, 20, 240, 240},
    {"origin x positive",      40,   0, 16, 16, 20, 20, 240, 240},
    {"origin x negative",     -40,   0, 16, 16, 20, 20, 240, 240},
    {"origin y zero",           0,   0, 16, 16, 20, 20, 240, 240},
    {"origin y positive",       0,  40, 16, 16, 20, 20, 240, 240},
    {"origin y negative",       0, -40, 16, 16, 20, 20, 240, 240},
    {"extent exact multiple",   0,   0, 16, 16, 20, 20, 240, 240},
    {"extent not multiple",     0,   0, 16, 16, 20, 20, 241, 241},
    {"map smaller than screen", 0,   0, 16, 16,  5,  5, 240, 240},
    {"map equal to screen",     0,   0, 16, 16, 15, 15, 240, 240},
    {"map larger than screen",  0,   0, 16, 16, 40, 40, 240, 240},
    {"map off-screen left", -1000,   0, 16, 16, 10, 10, 240, 240},
    {"map off-screen right", 1000,   0, 16, 16, 10, 10, 240, 240},
};

}  // namespace

void test_math_cell_range_for_screen_rect_matches_the_shipped_orthogonal_culling(void) {
    for (const CharacterizationCase& c : kCharacterizationCases) {
        const math::ProjectionSpec spec{c.viewOriginX, c.viewOriginY, c.tileWidth, 0, 0,
                                        c.tileHeight};
        const OrthogonalCullOracle oracle =
            orthogonalCullOracle(c.viewOriginX, c.viewOriginY, c.tileWidth, c.tileHeight,
                                 c.mapWidth, c.mapHeight, c.logicalWidth, c.logicalHeight);
        const math::CellRange r = math::cellRangeForScreenRect(
            spec, 0, 0, c.logicalWidth, c.logicalHeight, c.mapWidth, c.mapHeight);

        assertAxisRangeMatchesOracle(oracle.startCol, oracle.endCol, r.startCol, r.endCol, c.name);
        assertAxisRangeMatchesOracle(oracle.startRow, oracle.endRow, r.startRow, r.endRow, c.name);
    }
}

void test_math_cell_range_for_screen_rect_orthogonal_edge_uses_inclusive_last_pixel(void) {
    // AC-3 regression guard: the screen rect is inverted at its inclusive
    // last pixel (screenW - 1), never the exclusive edge. For a 240px-wide
    // screen tiled at 16px, the exclusive-edge form would yield endCol == 16
    // (240 / 16 exactly); the correct, last-pixel answer is endCol == 15,
    // because pixel 239 (the last pixel) falls in cell 14, so the half-open
    // end is 15. This is the specific defect this unit exists to prevent.
    constexpr math::ProjectionSpec spec{0, 0, 16, 0, 0, 16};
    const math::CellRange r = math::cellRangeForScreenRect(spec, 0, 0, 240, 240, 20, 20);
    TEST_ASSERT_EQUAL_INT(15, r.endCol);
    TEST_ASSERT_EQUAL_INT(15, r.endRow);
}

void test_math_cell_range_for_screen_rect_end_col_diverges_from_oracle_but_both_stay_empty(void) {
    // AC-2 divergence case 1: logicalWidth=100, viewOriginX=145, tileWidth=16
    // gives a = logicalWidth - viewOriginX = -45. C++ truncation toward zero
    // (the old, oracle code) and this function's ceil-via-corner-inversion
    // diverge for a negative dividend: oracle endCol == -1, new endCol == -2.
    // Both produce an EMPTY [startCol, endCol) range (0 >= -1 and 0 >= -2), so
    // behaviour is identical; do NOT assert raw equality here.
    constexpr int viewOriginX = 145;
    constexpr int tileWidth = 16;
    constexpr int mapWidth = 20;
    constexpr int logicalWidth = 100;

    const OrthogonalCullOracle oracle = orthogonalCullOracle(
        viewOriginX, 0, tileWidth, tileWidth, mapWidth, mapWidth, logicalWidth, logicalWidth);
    TEST_ASSERT_EQUAL_INT_MESSAGE(-1, oracle.endCol, "oracle must reproduce the documented -1");
    TEST_ASSERT_TRUE_MESSAGE(oracle.startCol >= oracle.endCol, "oracle range must be empty");

    constexpr math::ProjectionSpec spec{viewOriginX, 0, tileWidth, 0, 0, tileWidth};
    const math::CellRange r = math::cellRangeForScreenRect(spec, 0, 0, logicalWidth, logicalWidth,
                                                            mapWidth, mapWidth);
    TEST_ASSERT_EQUAL_INT_MESSAGE(-2, r.endCol, "new function must reproduce the documented -2");
    TEST_ASSERT_TRUE_MESSAGE(r.startCol >= r.endCol, "new range must be empty too");
}

void test_math_cell_range_for_screen_rect_start_col_has_no_upper_clamp_but_range_stays_empty(void) {
    // AC-2 divergence case 2: viewOriginX=-1000, tileWidth=16, cols=10. The
    // oracle's startCol (62) has NO upper clamp against cols (the shipped
    // code, and this function, clamp `start` low-only and `end` high-only —
    // see the asymmetric clamp), so it can exceed cols entirely. The range is
    // still empty once compared against endCol (10): emptiness comes from the
    // comparison, not from clamping. Assert emptiness, not a specific raw pair.
    constexpr int viewOriginX = -1000;
    constexpr int tileWidth = 16;
    constexpr int mapWidth = 10;
    constexpr int logicalWidth = 240;

    const OrthogonalCullOracle oracle = orthogonalCullOracle(
        viewOriginX, 0, tileWidth, tileWidth, mapWidth, mapWidth, logicalWidth, logicalWidth);
    TEST_ASSERT_EQUAL_INT_MESSAGE(62, oracle.startCol, "oracle must reproduce the documented 62");
    TEST_ASSERT_TRUE_MESSAGE(oracle.startCol >= oracle.endCol, "oracle range must be empty");

    constexpr math::ProjectionSpec spec{viewOriginX, 0, tileWidth, 0, 0, tileWidth};
    const math::CellRange r = math::cellRangeForScreenRect(spec, 0, 0, logicalWidth, logicalWidth,
                                                            mapWidth, mapWidth);
    TEST_ASSERT_TRUE_MESSAGE(r.startCol >= r.endCol, "new range must be empty too");
}

// ===========================================================================
// AC-5: non-orthogonal (isometric) — a tight superset of the visible cells
// ===========================================================================

void test_math_cell_range_for_screen_rect_iso_is_a_tight_superset_of_the_visible_cells(void) {
    // kIso2to1 on a 240x240 screen: every cell whose anchor projects inside
    // the rect must be inside the returned range (superset), and the range
    // must be a parallelogram-vs-bounding-box overdraw, not an arbitrary one.
    const math::CellRange r = math::cellRangeForScreenRect(kIso2to1, 0, 0, 240, 240, 32, 32);

    int visibleCount = 0;
    for (int row = 0; row < 32; ++row) {
        for (int col = 0; col < 32; ++col) {
            const int sx = math::cellToScreenX(col, row, kIso2to1);
            const int sy = math::cellToScreenY(col, row, kIso2to1);
            if (sx >= 0 && sx < 240 && sy >= 0 && sy < 240) {
                ++visibleCount;
                TEST_ASSERT_TRUE_MESSAGE(col >= r.startCol && col < r.endCol,
                                         "visible col must be inside the returned range");
                TEST_ASSERT_TRUE_MESSAGE(row >= r.startRow && row < r.endRow,
                                         "visible row must be inside the returned range");
            }
        }
    }

    TEST_ASSERT_TRUE_MESSAGE(visibleCount > 0, "the visible set must be non-empty for this fixture");

    // Measured overdraw factor for THIS fixture, recorded as a fact, not a
    // guess. The proposal's ~2.25x estimate is the unclamped
    // bounding-box-area-over-diamond-area ratio for an infinite map; here the
    // 32x32 map clamps the returned range to [0, 32) on both axes, which
    // trims part of that bounding box back down. The actually measured ratio
    // for a 32x32 kIso2to1 map on a 240x240 screen is ~1.96x.
    const int returnedCells = (r.endCol - r.startCol) * (r.endRow - r.startRow);
    const float overdraw = static_cast<float>(returnedCells) / static_cast<float>(visibleCount);
    TEST_ASSERT_FLOAT_WITHIN(0.05f, 1.96f, overdraw);
}

// ===========================================================================
// AC-6: degenerate inputs — one test per proposal-§4 row
// ===========================================================================

void test_math_cell_range_for_screen_rect_degenerate_determinant_returns_empty(void) {
    // det == 0: collinear basis, no inverse. Must never divide.
    constexpr math::ProjectionSpec kCollinear{0, 0, 16, 0, 32, 0};
    const math::CellRange r = math::cellRangeForScreenRect(kCollinear, 0, 0, 240, 240, 20, 20);
    TEST_ASSERT_EQUAL_INT(0, r.startCol);
    TEST_ASSERT_EQUAL_INT(0, r.endCol);
    TEST_ASSERT_EQUAL_INT(0, r.startRow);
    TEST_ASSERT_EQUAL_INT(0, r.endRow);
}

void test_math_cell_range_for_screen_rect_non_positive_cols_or_rows_returns_empty(void) {
    constexpr math::ProjectionSpec spec{0, 0, 16, 0, 0, 16};
    const math::CellRange zeroCols = math::cellRangeForScreenRect(spec, 0, 0, 240, 240, 0, 20);
    const math::CellRange zeroRows = math::cellRangeForScreenRect(spec, 0, 0, 240, 240, 20, 0);
    const math::CellRange negativeCols = math::cellRangeForScreenRect(spec, 0, 0, 240, 240, -5, 20);

    TEST_ASSERT_EQUAL_INT(0, zeroCols.endCol);
    TEST_ASSERT_EQUAL_INT(0, zeroRows.endRow);
    TEST_ASSERT_EQUAL_INT(0, negativeCols.endCol);
}

void test_math_cell_range_for_screen_rect_non_positive_screen_extent_returns_empty(void) {
    constexpr math::ProjectionSpec spec{0, 0, 16, 0, 0, 16};
    const math::CellRange zeroWidth = math::cellRangeForScreenRect(spec, 0, 0, 0, 240, 20, 20);
    const math::CellRange zeroHeight = math::cellRangeForScreenRect(spec, 0, 0, 240, 0, 20, 20);

    TEST_ASSERT_EQUAL_INT(0, zeroWidth.endCol);
    TEST_ASSERT_EQUAL_INT(0, zeroHeight.endRow);
}

void test_math_cell_range_for_screen_rect_entirely_off_map_returns_empty(void) {
    // Map anchored far outside the screen rect: no corner inversion lands
    // inside [0, cols) x [0, rows) once clamped.
    constexpr math::ProjectionSpec spec{2000, 2000, 16, 0, 0, 16};
    const math::CellRange r = math::cellRangeForScreenRect(spec, 0, 0, 240, 240, 20, 20);
    TEST_ASSERT_TRUE(r.startCol >= r.endCol);
    TEST_ASSERT_TRUE(r.startRow >= r.endRow);
}

// ===========================================================================
// AC-7: constexpr — a full non-empty range computed at compile time
// ===========================================================================

namespace {

constexpr math::ProjectionSpec kStaticProbeSpec{0, 0, 16, 0, 0, 16};
constexpr math::CellRange kStaticRange =
    math::cellRangeForScreenRect(kStaticProbeSpec, 0, 0, 240, 240, 20, 20);

// The same orthogonal case pinned by the AC-3 regression test above, proven
// here to be a compile-time constant, not merely runtime-correct.
static_assert(kStaticRange.startCol == 0, "cellRangeForScreenRect must be constexpr: startCol");
static_assert(kStaticRange.endCol == 15, "cellRangeForScreenRect must be constexpr: endCol");
static_assert(kStaticRange.startRow == 0, "cellRangeForScreenRect must be constexpr: startRow");
static_assert(kStaticRange.endRow == 15, "cellRangeForScreenRect must be constexpr: endRow");

}  // namespace

void test_math_cell_range_for_screen_rect_is_constexpr_evaluable(void) {
    TEST_PASS_MESSAGE(
        "cellRangeForScreenRect computed a full non-empty range at compile "
        "time (see the static_assert block above this test).");
}

#else  // !PIXELROOT32_ENABLE_PROJECTION

void test_math_projection_zero_cost_when_disabled(void) {
    // With the flag off, math::ProjectionSpec/projectionDet/
    // projectionSpecIsValid/cellToScreen*/screenToCell*/CellRange/
    // cellRangeForScreenRect are not compiled at all — their entire
    // declaration lives inside the #if PIXELROOT32_ENABLE_PROJECTION guard in
    // include/math/Projection.h. This translation unit compiling and passing
    // without referencing any of them IS the "zero bytes reserved" property
    // required by the spec's "Flags-off means not compiled" requirement.
    TEST_PASS_MESSAGE(
        "PIXELROOT32_ENABLE_PROJECTION=0: math::ProjectionSpec/cellToScreen/"
        "screenToCell/projectionSpecIsValid/CellRange/cellRangeForScreenRect "
        "are not compiled, zero bytes reserved.");
}

#endif  // PIXELROOT32_ENABLE_PROJECTION

void setUp(void) {
    test_setup();
}

void tearDown(void) {
    test_teardown();
}

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;
    UNITY_BEGIN();

#if PIXELROOT32_ENABLE_PROJECTION
    RUN_TEST(test_math_projection_spec_is_a_plain_six_int_aggregate);
    RUN_TEST(test_math_cell_to_screen_and_screen_to_cell_round_trip);
    RUN_TEST(test_math_projection_floor_div_floors_toward_negative_infinity);
    RUN_TEST(test_gameplay_projection_spec_is_the_same_type_as_math_projection_spec);
    RUN_TEST(test_gameplay_cell_to_screen_x_is_the_same_function_as_math);
    RUN_TEST(test_gameplay_cell_to_screen_y_is_the_same_function_as_math);
    RUN_TEST(test_gameplay_screen_to_cell_x_is_the_same_function_as_math);
    RUN_TEST(test_gameplay_screen_to_cell_y_is_the_same_function_as_math);
    RUN_TEST(test_gameplay_projection_det_is_the_same_function_as_math);
    RUN_TEST(test_gameplay_projection_spec_is_valid_is_the_same_function_as_math);
    RUN_TEST(test_gameplay_detail_projection_floor_div_is_the_same_function_as_math);
    RUN_TEST(test_math_row_major_is_painter_order_orthogonal_layout_is_false);
    RUN_TEST(test_math_row_major_is_painter_order_iso_2to1_layout_is_true);
    RUN_TEST(test_math_row_major_is_painter_order_iso_1to1_layout_is_true);
    RUN_TEST(test_math_row_major_is_painter_order_oblique_layout_is_false);
    RUN_TEST(test_math_row_major_is_painter_order_flipped_axis_yy_is_false);
    RUN_TEST(test_math_row_major_is_painter_order_flipped_axis_xy_is_false);
    RUN_TEST(test_math_row_major_is_painter_order_both_axes_zero_rejects_the_ge_zero_mistake);
    RUN_TEST(test_math_row_major_is_painter_order_axis_xy_zero_alone_is_false);
    RUN_TEST(test_math_row_major_is_painter_order_axis_yy_zero_alone_is_false);
    RUN_TEST(test_gameplay_row_major_is_painter_order_is_the_same_function_as_math);
    RUN_TEST(test_math_cell_range_for_screen_rect_matches_the_shipped_orthogonal_culling);
    RUN_TEST(test_math_cell_range_for_screen_rect_orthogonal_edge_uses_inclusive_last_pixel);
    RUN_TEST(test_math_cell_range_for_screen_rect_end_col_diverges_from_oracle_but_both_stay_empty);
    RUN_TEST(test_math_cell_range_for_screen_rect_start_col_has_no_upper_clamp_but_range_stays_empty);
    RUN_TEST(test_math_cell_range_for_screen_rect_iso_is_a_tight_superset_of_the_visible_cells);
    RUN_TEST(test_math_cell_range_for_screen_rect_degenerate_determinant_returns_empty);
    RUN_TEST(test_math_cell_range_for_screen_rect_non_positive_cols_or_rows_returns_empty);
    RUN_TEST(test_math_cell_range_for_screen_rect_non_positive_screen_extent_returns_empty);
    RUN_TEST(test_math_cell_range_for_screen_rect_entirely_off_map_returns_empty);
    RUN_TEST(test_math_cell_range_for_screen_rect_is_constexpr_evaluable);
#endif
    RUN_TEST(test_math_projection_zero_cost_when_disabled);

    return UNITY_END();
}
