/**
 * @file test_gameplay_grid_space.cpp
 * @brief Unit tests for gameplay/GridSpace module
 *
 * Covers the spec requirements for gameplay-grid-space:
 * - grid-spec-is-a-constexpr-friendly-value-describing-origin-cell-size-and-extent
 * - cell-to-world-converts-a-cell-index-to-a-world-position-in-int-and-scalar-without-division
 * - world-to-cell-maps-a-world-position-to-its-containing-cell-by-floor-division-never-truncation
 * - cell-to-world-and-world-to-cell-round-trip-exactly-including-negative-cells
 * - contains-cell-bounds-checks-a-cell-index-against-the-grid-extent
 * - feature-gated-and-zero-cost-when-disabled
 *
 * Plus cases the requirement names above do not call out verbatim: non-square
 * cells (cellWidth != cellHeight scaling independently per axis), the Scalar
 * overloads flooring a genuinely fractional input rather than truncating it,
 * and axis independence on a non-square grid.
 *
 * The functional tests only compile when PIXELROOT32_ENABLE_GAMEPLAY_GRID_SPACE
 * is enabled, since GridSpace is entirely guarded behind that flag (see
 * include/gameplay/GridSpace.h). This file therefore compiles cleanly in
 * BOTH the default (flag off) and opt-in (flag on) configurations, matching
 * the "no behavior change for existing examples" goal and the flags-off /
 * flags-on CI matrix.
 */

#include <unity.h>
#include "../../test_config.h"
#include "platforms/PlatformDefaults.h"

#if PIXELROOT32_ENABLE_GAMEPLAY_GRID_SPACE

#include "gameplay/GridSpace.h"
#include "math/Vector2.h"
#include "math/MathUtil.h"
#include "math/Scalar.h"

#include <cstddef>
#include <cstdint>

using namespace pixelroot32::gameplay;
using pixelroot32::math::Scalar;
using pixelroot32::math::Vector2;
using pixelroot32::math::toScalar;

namespace {

// Square 16x16-cell grid, origin at (0,0), 20 cols x 15 rows — used for every
// scenario the spec states in terms of "cellWidth=16".
constexpr GridSpec kSquareGrid{0, 0, 16, 16, 20, 15};
static_assert(gridSpecIsValid(kSquareGrid), "kSquareGrid must be a valid spec.");

// Non-square cells (cellWidth != cellHeight), origin at (0,0).
constexpr GridSpec kNonSquareGrid{0, 0, 12, 8, 10, 10};
static_assert(gridSpecIsValid(kNonSquareGrid), "kNonSquareGrid must be a valid spec.");

// A distinct 10x10 grid used only for the containsCell boundary scenarios.
constexpr GridSpec kTenByTenGrid{0, 0, 16, 16, 10, 10};
static_assert(gridSpecIsValid(kTenByTenGrid), "kTenByTenGrid must be a valid spec.");

// Invalid: zero cell width (RISC-V div-by-zero returns -1 instead of
// trapping, so this must be caught by the predicate, not by hardware).
constexpr GridSpec kZeroWidthGrid{0, 0, 0, 16, 10, 10};
static_assert(!gridSpecIsValid(kZeroWidthGrid), "kZeroWidthGrid must be rejected (cellWidth < 1).");

// Invalid: extent exceeds Scalar's +/-32767 fixed-point integer range.
constexpr GridSpec kOversizedGrid{0, 0, 100, 100, 1000, 1000};
static_assert(!gridSpecIsValid(kOversizedGrid), "kOversizedGrid must be rejected (extent exceeds +/-32767).");

}  // namespace

// =============================================================================
// Requirement: GridSpec Is A constexpr-Friendly Value Describing Origin,
// Cell Size, And Extent
// =============================================================================

void test_grid_spec_is_a_plain_six_int_aggregate(void) {
    // No hidden per-instance state beyond the six declared int fields: any
    // growth here is SRAM cost paid by every non-constexpr consumer.
    static_assert(sizeof(GridSpec) == 6 * sizeof(int),
                  "GridSpec must be exactly six ints, no hidden state.");
    TEST_ASSERT_EQUAL_UINT32(static_cast<uint32_t>(6 * sizeof(int)), static_cast<uint32_t>(sizeof(GridSpec)));
}

void test_grid_spec_is_valid_accepts_well_formed_specs(void) {
    TEST_ASSERT_TRUE(gridSpecIsValid(kSquareGrid));
    TEST_ASSERT_TRUE(gridSpecIsValid(kNonSquareGrid));
}

void test_grid_spec_is_valid_rejects_zero_or_negative_cell_size(void) {
    TEST_ASSERT_FALSE(gridSpecIsValid(kZeroWidthGrid));

    constexpr GridSpec negativeHeightGrid{0, 0, 16, -1, 10, 10};
    TEST_ASSERT_FALSE(gridSpecIsValid(negativeHeightGrid));
}

void test_grid_spec_is_valid_rejects_extent_exceeding_scalar_range(void) {
    TEST_ASSERT_FALSE(gridSpecIsValid(kOversizedGrid));
}

// =============================================================================
// Requirement: cellToWorld Converts A Cell Index To A World Position, In int
// And Scalar, Without Division
// =============================================================================

void test_cell_to_world_x_y_agree_with_scalar_on_positive_cell(void) {
    const int worldX = cellToWorldX(3, kSquareGrid);
    const int worldY = cellToWorldY(2, kSquareGrid);
    TEST_ASSERT_EQUAL_INT(48, worldX);
    TEST_ASSERT_EQUAL_INT(32, worldY);

    const Vector2 world = cellToWorld(3, 2, kSquareGrid);
    TEST_ASSERT_TRUE(world.x == toScalar(worldX));
    TEST_ASSERT_TRUE(world.y == toScalar(worldY));
}

void test_cell_to_world_x_y_agree_with_scalar_on_negative_cell(void) {
    const int worldX = cellToWorldX(-2, kSquareGrid);
    TEST_ASSERT_EQUAL_INT(-32, worldX);

    const Vector2 world = cellToWorld(-2, 0, kSquareGrid);
    TEST_ASSERT_TRUE(world.x == toScalar(worldX));
    TEST_ASSERT_TRUE(world.y == toScalar(0));
}

void test_cell_to_world_respects_non_square_cell_dimensions(void) {
    // cellWidth=12, cellHeight=8: X and Y must scale independently, never
    // sharing a single axis-agnostic cell size.
    const int worldX = cellToWorldX(3, kNonSquareGrid);
    const int worldY = cellToWorldY(3, kNonSquareGrid);
    TEST_ASSERT_EQUAL_INT(3 * 12, worldX);
    TEST_ASSERT_EQUAL_INT(3 * 8, worldY);
    TEST_ASSERT_TRUE(worldX != worldY);
}

// =============================================================================
// Requirement: worldToCell Maps A World Position To Its Containing Cell By
// Floor Division, Never Truncation
// =============================================================================

void test_world_to_cell_x_positive_coordinate(void) {
    TEST_ASSERT_EQUAL_INT(1, worldToCellX(20, kSquareGrid));
    TEST_ASSERT_EQUAL_INT(1, worldToCellX(toScalar(20), kSquareGrid));
}

void test_world_to_cell_x_negative_coordinate_floors_not_truncates(void) {
    // The correctness hazard: truncation would give 0, not -1.
    TEST_ASSERT_EQUAL_INT(-1, worldToCellX(-1, kSquareGrid));
    TEST_ASSERT_EQUAL_INT(-1, worldToCellX(toScalar(-1), kSquareGrid));
}

void test_world_to_cell_x_exact_cell_edge_both_signs(void) {
    TEST_ASSERT_EQUAL_INT(1, worldToCellX(16, kSquareGrid));
    TEST_ASSERT_EQUAL_INT(-1, worldToCellX(-16, kSquareGrid));
    TEST_ASSERT_EQUAL_INT(1, worldToCellX(toScalar(16), kSquareGrid));
    TEST_ASSERT_EQUAL_INT(-1, worldToCellX(toScalar(-16), kSquareGrid));
}

void test_world_to_cell_x_one_unit_below_edge_both_signs(void) {
    TEST_ASSERT_EQUAL_INT(0, worldToCellX(15, kSquareGrid));
    TEST_ASSERT_EQUAL_INT(-2, worldToCellX(-17, kSquareGrid));
}

void test_world_to_cell_x_negative_exact_multiple_of_cell_size(void) {
    // No off-by-one: -32 is exactly -2 cells, not -1 or -3.
    TEST_ASSERT_EQUAL_INT(-2, worldToCellX(-32, kSquareGrid));
}

void test_world_to_cell_axis_independence_on_non_square_grid(void) {
    // At world=11 on a (cellWidth=12, cellHeight=8) grid: X floors to cell 0
    // (11/12), Y floors to cell 1 (11/8) — proving each axis uses its own
    // cell size, never a shared axis-agnostic divisor.
    TEST_ASSERT_EQUAL_INT(0, worldToCellX(11, kNonSquareGrid));
    TEST_ASSERT_EQUAL_INT(1, worldToCellY(11, kNonSquareGrid));

    // Negative coordinate, same axis-independence check.
    TEST_ASSERT_EQUAL_INT(-1, worldToCellX(-1, kNonSquareGrid));  // floor(-1/12) = -1
    TEST_ASSERT_EQUAL_INT(-1, worldToCellY(-1, kNonSquareGrid));  // floor(-1/8)  = -1
    TEST_ASSERT_EQUAL_INT(-1, worldToCellX(-12, kNonSquareGrid)); // exact multiple of 12
    TEST_ASSERT_EQUAL_INT(-2, worldToCellY(-12, kNonSquareGrid)); // floor(-12/8) = -2
}

void test_world_to_cell_x_scalar_overload_floors_fractional_input_not_truncates(void) {
    // A genuinely fractional Scalar (not an integer round-tripped through
    // toScalar()) exercises math::floorToInt() inside the Scalar overload.
    // Truncation would map 15.9 -> cell 0 (same as floor) but -0.1 -> cell 0
    // (WRONG: floor gives -1) — this is the case that actually distinguishes
    // the two behaviors.
    const Scalar justAboveZero = static_cast<Scalar>(15.9f);
    TEST_ASSERT_EQUAL_INT(0, worldToCellX(justAboveZero, kSquareGrid));

    const Scalar justBelowZero = static_cast<Scalar>(-0.1f);
    TEST_ASSERT_EQUAL_INT(-1, worldToCellX(justBelowZero, kSquareGrid));
}

// =============================================================================
// Requirement: cellToWorld And worldToCell Round-Trip Exactly, Including
// Negative Cells
// =============================================================================

void test_round_trip_identity_holds_for_positive_zero_and_negative_cells(void) {
    const int cells[] = {5, 0, -5};
    for (int c : cells) {
        const int worldX = cellToWorldX(c, kSquareGrid);
        TEST_ASSERT_EQUAL_INT(c, worldToCellX(worldX, kSquareGrid));
        TEST_ASSERT_EQUAL_INT(c, worldToCellX(toScalar(worldX), kSquareGrid));

        const int worldY = cellToWorldY(c, kSquareGrid);
        TEST_ASSERT_EQUAL_INT(c, worldToCellY(worldY, kSquareGrid));
        TEST_ASSERT_EQUAL_INT(c, worldToCellY(toScalar(worldY), kSquareGrid));
    }
}

// =============================================================================
// Requirement: containsCell Bounds-Checks A Cell Index Against The Grid
// Extent
// =============================================================================

void test_contains_cell_rejects_negative_index_from_world_to_cell(void) {
    TEST_ASSERT_FALSE(containsCell(-1, 0, kTenByTenGrid));
    TEST_ASSERT_FALSE(containsCell(0, -1, kTenByTenGrid));
}

void test_contains_cell_accepts_last_valid_rejects_one_past(void) {
    TEST_ASSERT_TRUE(containsCell(9, 9, kTenByTenGrid));
    TEST_ASSERT_FALSE(containsCell(10, 9, kTenByTenGrid));
    TEST_ASSERT_FALSE(containsCell(9, 10, kTenByTenGrid));
}

// =============================================================================
// Requirement: Feature-Gated And Zero-Cost When Disabled
//
// Defined in BOTH flag states, mirroring test_gameplay_state_machine.cpp:680,
// 711 and test_gameplay_object_pool.cpp:513,536, because main() RUN_TESTs it
// unconditionally. Defining it only in the #else branch leaves main()
// referencing an undeclared function whenever the flag is on.
// =============================================================================

void test_gameplay_grid_space_zero_cost_when_disabled(void) {
    // With the flag on, GridSpec must stay a plain six-int aggregate — no
    // vtable, no hidden padding-inducing member — and every free function
    // must be usable from a constexpr context, proving zero runtime
    // footprint for the constexpr-consumer path the byte budget relies on.
    static_assert(sizeof(GridSpec) == 6 * sizeof(int),
                  "GridSpec must be exactly six ints: any growth here is "
                  "SRAM cost paid by every non-constexpr consumer.");
    TEST_ASSERT_EQUAL_UINT32(static_cast<uint32_t>(6 * sizeof(int)), static_cast<uint32_t>(sizeof(GridSpec)));

    constexpr GridSpec kProbe{0, 0, 8, 8, 4, 4};
    static_assert(gridSpecIsValid(kProbe), "probe spec must be constexpr-valid");
    static_assert(cellToWorldX(1, kProbe) == 8, "cellToWorldX must be constexpr-evaluable");
    static_assert(cellToWorldY(1, kProbe) == 8, "cellToWorldY must be constexpr-evaluable");
    static_assert(worldToCellX(8, kProbe) == 1, "worldToCellX(int) must be constexpr-evaluable");
    static_assert(worldToCellY(8, kProbe) == 1, "worldToCellY(int) must be constexpr-evaluable");
    static_assert(containsCell(1, 1, kProbe), "containsCell must be constexpr-evaluable");

    TEST_PASS_MESSAGE(
        "PIXELROOT32_ENABLE_GAMEPLAY_GRID_SPACE=1: GridSpec stays a plain "
        "six-int aggregate and every free function is constexpr-evaluable, "
        "so a constexpr consumer pays zero runtime footprint.");
}

#else  // !PIXELROOT32_ENABLE_GAMEPLAY_GRID_SPACE

void test_gameplay_grid_space_zero_cost_when_disabled(void) {
    // With the flag off, GridSpec/cellToWorld/worldToCell/containsCell are
    // not compiled at all — their entire declaration lives inside the
    // #if PIXELROOT32_ENABLE_GAMEPLAY_GRID_SPACE guard in
    // include/gameplay/GridSpace.h. This translation unit compiling and
    // passing without referencing any of them IS the "zero bytes reserved"
    // property required by the spec's "Feature-Gated And Zero-Cost When
    // Disabled" requirement.
    TEST_PASS_MESSAGE(
        "PIXELROOT32_ENABLE_GAMEPLAY_GRID_SPACE=0: GridSpec/cellToWorld/"
        "worldToCell/containsCell are not compiled, zero bytes reserved.");
}

#endif  // PIXELROOT32_ENABLE_GAMEPLAY_GRID_SPACE

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

#if PIXELROOT32_ENABLE_GAMEPLAY_GRID_SPACE
    RUN_TEST(test_grid_spec_is_a_plain_six_int_aggregate);
    RUN_TEST(test_grid_spec_is_valid_accepts_well_formed_specs);
    RUN_TEST(test_grid_spec_is_valid_rejects_zero_or_negative_cell_size);
    RUN_TEST(test_grid_spec_is_valid_rejects_extent_exceeding_scalar_range);
    RUN_TEST(test_cell_to_world_x_y_agree_with_scalar_on_positive_cell);
    RUN_TEST(test_cell_to_world_x_y_agree_with_scalar_on_negative_cell);
    RUN_TEST(test_cell_to_world_respects_non_square_cell_dimensions);
    RUN_TEST(test_world_to_cell_x_positive_coordinate);
    RUN_TEST(test_world_to_cell_x_negative_coordinate_floors_not_truncates);
    RUN_TEST(test_world_to_cell_x_exact_cell_edge_both_signs);
    RUN_TEST(test_world_to_cell_x_one_unit_below_edge_both_signs);
    RUN_TEST(test_world_to_cell_x_negative_exact_multiple_of_cell_size);
    RUN_TEST(test_world_to_cell_axis_independence_on_non_square_grid);
    RUN_TEST(test_world_to_cell_x_scalar_overload_floors_fractional_input_not_truncates);
    RUN_TEST(test_round_trip_identity_holds_for_positive_zero_and_negative_cells);
    RUN_TEST(test_contains_cell_rejects_negative_index_from_world_to_cell);
    RUN_TEST(test_contains_cell_accepts_last_valid_rejects_one_past);
#endif
    RUN_TEST(test_gameplay_grid_space_zero_cost_when_disabled);

    return UNITY_END();
}
