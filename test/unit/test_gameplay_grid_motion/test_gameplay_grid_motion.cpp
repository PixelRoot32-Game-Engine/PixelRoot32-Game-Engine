/**
 * @file test_gameplay_grid_motion.cpp
 * @brief Unit tests for the gameplay/GridMotion module.
 *
 * GridMotion is the policy-free half of cell-to-cell movement: it owns the
 * logical cell, the in-flight target, and the sub-cell progress counter, and
 * nothing else. Which cells are enterable, where the next direction comes
 * from, and what happens on arrival all stay in game code, because those
 * diverge between any two actors that move on a grid.
 *
 * The functional tests only compile when PIXELROOT32_ENABLE_GAMEPLAY_GRID_SPACE
 * is enabled, since GridMotion is entirely guarded behind that flag (it shares
 * GridSpace's flag: interpolatedWorld() takes a GridSpec, so motion without
 * space is not a reachable configuration). This file therefore compiles
 * cleanly in BOTH the default (flag off) and opt-in (flag on) configurations.
 */

#include <unity.h>
#include "../../test_config.h"
#include "platforms/PlatformDefaults.h"

#if PIXELROOT32_ENABLE_GAMEPLAY_GRID_SPACE

#include "gameplay/GridMotion.h"
#include "gameplay/GridSpace.h"
#include "math/Vector2.h"
#include "math/MathUtil.h"
#include "math/Scalar.h"

using namespace pixelroot32::gameplay;
using pixelroot32::math::Scalar;
using pixelroot32::math::Vector2;
using pixelroot32::math::toScalar;

namespace {

// Square 16x16-cell grid, origin at (0,0), 20 cols x 15 rows.
constexpr GridSpec kSquareGrid{0, 0, 16, 16, 20, 15};
static_assert(gridSpecIsValid(kSquareGrid), "kSquareGrid must be a valid spec.");

// Non-square cells, to prove the two axes interpolate independently.
constexpr GridSpec kNonSquareGrid{0, 0, 12, 8, 10, 10};
static_assert(gridSpecIsValid(kNonSquareGrid), "kNonSquareGrid must be a valid spec.");

// Non-zero origin, to prove interpolation is anchored to the spec's origin
// rather than to world (0,0).
constexpr GridSpec kOffsetGrid{40, 24, 16, 16, 10, 10};
static_assert(gridSpecIsValid(kOffsetGrid), "kOffsetGrid must be a valid spec.");

/// Short step budget so a whole traversal fits in an explicit, readable
/// sequence of calls rather than a loop the reader has to simulate.
constexpr int kSteps = 4;

/// Matches bomberbot's player cadence, used where the test needs a realistic
/// number of in-flight frames.
constexpr int kLongSteps = 12;

int worldX(const GridMotion& m, int stepsPerCell, const GridSpec& spec) {
    return pixelroot32::math::floorToInt(interpolatedWorld(m, stepsPerCell, spec).x);
}

int worldY(const GridMotion& m, int stepsPerCell, const GridSpec& spec) {
    return pixelroot32::math::floorToInt(interpolatedWorld(m, stepsPerCell, spec).y);
}

}  // namespace

// --- Value shape and rest state -------------------------------------------

void test_default_constructed_motion_is_at_rest_in_cell_zero_zero(void) {
    GridMotion m;

    TEST_ASSERT_EQUAL_INT(0, m.cellX);
    TEST_ASSERT_EQUAL_INT(0, m.cellY);
    TEST_ASSERT_EQUAL_INT(0, m.toX);
    TEST_ASSERT_EQUAL_INT(0, m.toY);
    TEST_ASSERT_EQUAL_INT(0, m.progress);
    TEST_ASSERT_FALSE(isMoving(m));
}

void test_grid_motion_is_a_plain_five_int_aggregate(void) {
    // Aggregate initialization must remain available: GridMotion is a value,
    // not an object with invariants to protect. A constexpr instance costs
    // zero SRAM, exactly like GridSpec.
    constexpr GridMotion m{3, 4, 3, 4, 0};

    static_assert(m.cellX == 3, "GridMotion must be a constexpr-friendly aggregate.");
    static_assert(sizeof(GridMotion) == 5 * sizeof(int),
                  "GridMotion must stay five ints with no hidden members.");

    TEST_ASSERT_EQUAL_INT(4, m.cellY);
    TEST_ASSERT_FALSE(isMoving(m));
}

void test_place_at_sets_logical_cell_target_and_clears_progress(void) {
    GridMotion m;
    placeAt(m, 7, 5);

    TEST_ASSERT_EQUAL_INT(7, m.cellX);
    TEST_ASSERT_EQUAL_INT(5, m.cellY);
    TEST_ASSERT_EQUAL_INT(7, m.toX);
    TEST_ASSERT_EQUAL_INT(5, m.toY);
    TEST_ASSERT_EQUAL_INT(0, m.progress);
    TEST_ASSERT_FALSE(isMoving(m));
}

void test_place_at_cancels_an_in_flight_step(void) {
    GridMotion m;
    placeAt(m, 2, 2);
    beginStep(m, 3, 2);
    tickStep(m, kLongSteps);
    TEST_ASSERT_TRUE(isMoving(m));

    // A respawn/teleport must not leave the actor half-way into a step it
    // will never finish.
    placeAt(m, 9, 1);

    TEST_ASSERT_FALSE(isMoving(m));
    TEST_ASSERT_EQUAL_INT(9, m.cellX);
    TEST_ASSERT_EQUAL_INT(9, m.toX);
    TEST_ASSERT_EQUAL_INT(1, m.cellY);
    TEST_ASSERT_EQUAL_INT(1, m.toY);
    TEST_ASSERT_EQUAL_INT(0, m.progress);
}

// --- Starting a step -------------------------------------------------------

void test_begin_step_arms_the_target_without_moving_the_logical_cell(void) {
    GridMotion m;
    placeAt(m, 4, 4);

    beginStep(m, 5, 4);

    // The logical cell is what every gameplay rule reads (collision, bomb
    // placement, AI). It must NOT change until the step completes.
    TEST_ASSERT_EQUAL_INT(4, m.cellX);
    TEST_ASSERT_EQUAL_INT(4, m.cellY);
    TEST_ASSERT_EQUAL_INT(5, m.toX);
    TEST_ASSERT_EQUAL_INT(4, m.toY);
    TEST_ASSERT_EQUAL_INT(1, m.progress);
    TEST_ASSERT_TRUE(isMoving(m));
}

// --- Ticking a step --------------------------------------------------------

void test_tick_step_at_rest_is_a_no_op_and_reports_no_arrival(void) {
    GridMotion m;
    placeAt(m, 6, 6);

    const bool arrived = tickStep(m, kSteps);

    TEST_ASSERT_FALSE(arrived);
    TEST_ASSERT_EQUAL_INT(0, m.progress);
    TEST_ASSERT_EQUAL_INT(6, m.cellX);
    TEST_ASSERT_EQUAL_INT(6, m.cellY);
}

void test_tick_step_advances_progress_without_arriving_mid_flight(void) {
    GridMotion m;
    placeAt(m, 0, 0);
    beginStep(m, 1, 0);

    TEST_ASSERT_FALSE(tickStep(m, kSteps));
    TEST_ASSERT_EQUAL_INT(2, m.progress);
    TEST_ASSERT_EQUAL_INT(0, m.cellX);

    TEST_ASSERT_FALSE(tickStep(m, kSteps));
    TEST_ASSERT_EQUAL_INT(3, m.progress);
    TEST_ASSERT_EQUAL_INT(0, m.cellX);
}

void test_tick_step_commits_the_logical_cell_exactly_on_the_arrival_tick(void) {
    GridMotion m;
    placeAt(m, 0, 0);
    beginStep(m, 1, 0);

    tickStep(m, kSteps);  // progress 2
    tickStep(m, kSteps);  // progress 3
    const bool arrived = tickStep(m, kSteps);  // progress 4 -> commit

    TEST_ASSERT_TRUE(arrived);
    TEST_ASSERT_EQUAL_INT(1, m.cellX);
    TEST_ASSERT_EQUAL_INT(0, m.cellY);
    TEST_ASSERT_EQUAL_INT(0, m.progress);
    TEST_ASSERT_FALSE(isMoving(m));
}

void test_tick_step_reports_arrival_once_not_on_subsequent_ticks(void) {
    GridMotion m;
    placeAt(m, 0, 0);
    beginStep(m, 0, 1);
    tickStep(m, kSteps);
    tickStep(m, kSteps);
    TEST_ASSERT_TRUE(tickStep(m, kSteps));

    // The arrival edge drives one-shot game reactions (footstep SFX, clearing
    // a pass-through exemption). Firing it twice would double them.
    TEST_ASSERT_FALSE(tickStep(m, kSteps));
    TEST_ASSERT_FALSE(tickStep(m, kSteps));
    TEST_ASSERT_EQUAL_INT(1, m.cellY);
}

void test_a_full_traversal_takes_exactly_steps_per_cell_frames(void) {
    // beginStep() puts the actor at progress 1 on the frame the step starts,
    // so the whole traversal occupies stepsPerCell frames: one at begin, then
    // stepsPerCell-1 ticks, the last of which arrives.
    GridMotion m;
    placeAt(m, 0, 0);
    beginStep(m, 1, 0);

    int ticks = 0;
    while (isMoving(m)) {
        tickStep(m, kLongSteps);
        ++ticks;
    }

    TEST_ASSERT_EQUAL_INT(kLongSteps - 1, ticks);
    TEST_ASSERT_EQUAL_INT(1, m.cellX);
}

void test_consecutive_steps_chain_without_drift(void) {
    GridMotion m;
    placeAt(m, 2, 3);

    for (int i = 0; i < 3; ++i) {
        beginStep(m, m.cellX + 1, m.cellY);
        while (isMoving(m)) {
            tickStep(m, kSteps);
        }
    }

    TEST_ASSERT_EQUAL_INT(5, m.cellX);
    TEST_ASSERT_EQUAL_INT(3, m.cellY);
    TEST_ASSERT_EQUAL_INT(5, m.toX);
    TEST_ASSERT_EQUAL_INT(0, m.progress);
}

// --- Interpolated world position ------------------------------------------

void test_interpolated_world_at_rest_equals_cell_to_world(void) {
    GridMotion m;
    placeAt(m, 3, 2);

    TEST_ASSERT_EQUAL_INT(cellToWorldX(3, kSquareGrid), worldX(m, kSteps, kSquareGrid));
    TEST_ASSERT_EQUAL_INT(cellToWorldY(2, kSquareGrid), worldY(m, kSteps, kSquareGrid));
}

void test_interpolated_world_is_exact_at_both_endpoints(void) {
    GridMotion m;
    placeAt(m, 1, 1);
    const int fromPx = cellToWorldX(1, kSquareGrid);
    const int toPx = cellToWorldX(2, kSquareGrid);

    TEST_ASSERT_EQUAL_INT(fromPx, worldX(m, kSteps, kSquareGrid));

    beginStep(m, 2, 1);
    while (isMoving(m)) {
        tickStep(m, kSteps);
    }

    // After the commit the actor sits exactly on the destination cell — no
    // accumulated fractional residue.
    TEST_ASSERT_EQUAL_INT(toPx, worldX(m, kSteps, kSquareGrid));
}

void test_interpolated_world_advances_monotonically_and_stays_between_endpoints(void) {
    GridMotion m;
    placeAt(m, 0, 0);
    const int fromPx = cellToWorldX(0, kSquareGrid);
    const int toPx = cellToWorldX(1, kSquareGrid);

    beginStep(m, 1, 0);
    int previous = fromPx;
    while (isMoving(m)) {
        const int current = worldX(m, kLongSteps, kSquareGrid);
        TEST_ASSERT_TRUE(current >= previous);
        TEST_ASSERT_TRUE(current >= fromPx);
        TEST_ASSERT_TRUE(current <= toPx);
        previous = current;
        tickStep(m, kLongSteps);
    }

    TEST_ASSERT_EQUAL_INT(toPx, worldX(m, kLongSteps, kSquareGrid));
}

void test_interpolated_world_never_leads_the_true_position_in_either_direction(void) {
    // The lerp truncates toward zero rather than flooring. That is deliberate
    // and is NOT the negative-coordinate hazard GridSpace's worldToCell
    // guards against: here it means the drawn position always LAGS the exact
    // position by less than a pixel, symmetrically for both directions of
    // travel. Flooring would make rightward moves lag and leftward moves lead.
    GridMotion right;
    placeAt(right, 1, 0);
    beginStep(right, 2, 0);

    GridMotion left;
    placeAt(left, 1, 0);
    beginStep(left, 0, 0);

    const int origin = cellToWorldX(1, kSquareGrid);

    while (isMoving(right)) {
        const int rightOffset = worldX(right, kLongSteps, kSquareGrid) - origin;
        const int leftOffset = origin - worldX(left, kLongSteps, kSquareGrid);

        // Same magnitude at the same progress: neither direction is favoured.
        TEST_ASSERT_EQUAL_INT(rightOffset, leftOffset);

        tickStep(right, kLongSteps);
        tickStep(left, kLongSteps);
    }
}

void test_interpolated_world_moves_backwards_for_a_negative_direction_step(void) {
    GridMotion m;
    placeAt(m, 5, 5);
    const int fromPx = cellToWorldX(5, kSquareGrid);

    beginStep(m, 4, 5);
    tickStep(m, kLongSteps);

    const int current = worldX(m, kLongSteps, kSquareGrid);
    TEST_ASSERT_TRUE(current < fromPx);
    TEST_ASSERT_TRUE(current > cellToWorldX(4, kSquareGrid));

    // The logical cell has not flipped yet, even though the sprite has left it.
    TEST_ASSERT_EQUAL_INT(5, m.cellX);
}

void test_interpolated_world_handles_negative_cells(void) {
    GridMotion m;
    placeAt(m, -3, -2);

    TEST_ASSERT_EQUAL_INT(cellToWorldX(-3, kSquareGrid), worldX(m, kSteps, kSquareGrid));
    TEST_ASSERT_EQUAL_INT(cellToWorldY(-2, kSquareGrid), worldY(m, kSteps, kSquareGrid));

    beginStep(m, -2, -2);
    while (isMoving(m)) {
        tickStep(m, kSteps);
    }

    TEST_ASSERT_EQUAL_INT(cellToWorldX(-2, kSquareGrid), worldX(m, kSteps, kSquareGrid));
}

void test_interpolated_world_respects_a_non_zero_grid_origin(void) {
    GridMotion m;
    placeAt(m, 2, 3);

    TEST_ASSERT_EQUAL_INT(cellToWorldX(2, kOffsetGrid), worldX(m, kSteps, kOffsetGrid));
    TEST_ASSERT_EQUAL_INT(cellToWorldY(3, kOffsetGrid), worldY(m, kSteps, kOffsetGrid));

    beginStep(m, 2, 4);
    tickStep(m, kSteps);

    // Only the moving axis changes; the anchored axis stays on the origin-
    // offset column.
    TEST_ASSERT_EQUAL_INT(cellToWorldX(2, kOffsetGrid), worldX(m, kSteps, kOffsetGrid));
    TEST_ASSERT_TRUE(worldY(m, kSteps, kOffsetGrid) > cellToWorldY(3, kOffsetGrid));
}

void test_interpolated_world_scales_each_axis_by_its_own_cell_size(void) {
    // Half-way through a diagonal-looking pair of steps on a 12x8 grid, the X
    // offset must be driven by cellWidth and the Y offset by cellHeight.
    GridMotion horizontal;
    placeAt(horizontal, 0, 0);
    beginStep(horizontal, 1, 0);

    GridMotion vertical;
    placeAt(vertical, 0, 0);
    beginStep(vertical, 0, 1);

    tickStep(horizontal, kSteps);  // progress 2 of 4 -> half-way
    tickStep(vertical, kSteps);

    TEST_ASSERT_EQUAL_INT(kNonSquareGrid.cellWidth / 2, worldX(horizontal, kSteps, kNonSquareGrid));
    TEST_ASSERT_EQUAL_INT(kNonSquareGrid.cellHeight / 2, worldY(vertical, kSteps, kNonSquareGrid));

    // The idle axis of each motion has not drifted.
    TEST_ASSERT_EQUAL_INT(0, worldY(horizontal, kSteps, kNonSquareGrid));
    TEST_ASSERT_EQUAL_INT(0, worldX(vertical, kSteps, kNonSquareGrid));
}

void test_interpolated_world_agrees_with_grid_space_round_trip_after_arrival(void) {
    // The whole point of keeping the logical cell separate is that grid
    // queries stay exact. Landing on a cell and mapping the drawn position
    // back through worldToCell must return that same cell.
    GridMotion m;
    placeAt(m, 4, 6);
    beginStep(m, 4, 7);
    while (isMoving(m)) {
        tickStep(m, kLongSteps);
    }

    const Vector2 world = interpolatedWorld(m, kLongSteps, kSquareGrid);

    TEST_ASSERT_EQUAL_INT(4, worldToCellX(world.x, kSquareGrid));
    TEST_ASSERT_EQUAL_INT(7, worldToCellY(world.y, kSquareGrid));
    TEST_ASSERT_TRUE(containsCell(m.cellX, m.cellY, kSquareGrid));
}

// --- Under a projection ----------------------------------------------------
//
// The ProjectionSpec overload of interpolatedWorld() exists so an isometric
// game reuses GridMotion's stepping state instead of reimplementing cell-to-
// cell navigation. It is guarded on BOTH flags: the function lives in this
// header (so it needs GRID_SPACE) and its body needs PROJECTION.

#if PIXELROOT32_ENABLE_PROJECTION

namespace {

/// Isometric 2:1 with 32x16 diamond tiles. Both screen axes move for a step
/// along either cell axis, which is the property an axis-aligned GridSpec
/// cannot express.
constexpr ProjectionSpec kIso2to1{0, 0, 16, 8, -16, 8};
static_assert(projectionSpecIsValid(kIso2to1, 32, 32), "kIso2to1 must be a valid spec.");

int screenX(const GridMotion& m, int stepsPerCell, const ProjectionSpec& spec) {
    return pixelroot32::math::floorToInt(interpolatedWorld(m, stepsPerCell, spec).x);
}

int screenY(const GridMotion& m, int stepsPerCell, const ProjectionSpec& spec) {
    return pixelroot32::math::floorToInt(interpolatedWorld(m, stepsPerCell, spec).y);
}

}  // namespace

void test_interpolated_world_projection_endpoints_match_cell_to_screen(void) {
    GridMotion m;
    placeAt(m, 1, 1);

    // At rest: exactly the projected anchor of the logical cell.
    TEST_ASSERT_EQUAL_INT(cellToScreenX(1, 1, kIso2to1), screenX(m, kSteps, kIso2to1));
    TEST_ASSERT_EQUAL_INT(cellToScreenY(1, 1, kIso2to1), screenY(m, kSteps, kIso2to1));

    beginStep(m, 2, 1);
    while (isMoving(m)) {
        tickStep(m, kSteps);
    }

    // After the commit: exactly the destination anchor, no fractional residue.
    TEST_ASSERT_EQUAL_INT(cellToScreenX(2, 1, kIso2to1), screenX(m, kSteps, kIso2to1));
    TEST_ASSERT_EQUAL_INT(cellToScreenY(2, 1, kIso2to1), screenY(m, kSteps, kIso2to1));
}

void test_interpolated_world_projection_moves_on_both_screen_axes(void) {
    // A step along cell X moves the sprite right AND down under an isometric
    // basis. With a GridSpec the Y would not move at all — this is the whole
    // reason the overload exists.
    GridMotion m;
    placeAt(m, 1, 1);
    beginStep(m, 2, 1);

    const int fromX = cellToScreenX(1, 1, kIso2to1);  // 0
    const int fromY = cellToScreenY(1, 1, kIso2to1);  // 16
    const int toX = cellToScreenX(2, 1, kIso2to1);    // 16
    const int toY = cellToScreenY(2, 1, kIso2to1);    // 24

    int previousX = fromX;
    int previousY = fromY;

    while (isMoving(m)) {
        tickStep(m, kSteps);
        const int x = screenX(m, kSteps, kIso2to1);
        const int y = screenY(m, kSteps, kIso2to1);

        // Monotone on both axes, never past either endpoint.
        TEST_ASSERT_TRUE(x >= previousX);
        TEST_ASSERT_TRUE(y >= previousY);
        TEST_ASSERT_TRUE(x >= fromX && x <= toX);
        TEST_ASSERT_TRUE(y >= fromY && y <= toY);

        previousX = x;
        previousY = y;
    }

    TEST_ASSERT_EQUAL_INT(toX, previousX);
    TEST_ASSERT_EQUAL_INT(toY, previousY);
}

void test_interpolated_world_projection_handles_negative_cells(void) {
    GridMotion m;
    placeAt(m, -2, -3);

    TEST_ASSERT_EQUAL_INT(cellToScreenX(-2, -3, kIso2to1), screenX(m, kSteps, kIso2to1));
    TEST_ASSERT_EQUAL_INT(cellToScreenY(-2, -3, kIso2to1), screenY(m, kSteps, kIso2to1));

    beginStep(m, -1, -3);
    while (isMoving(m)) {
        tickStep(m, kSteps);
    }

    TEST_ASSERT_EQUAL_INT(cellToScreenX(-1, -3, kIso2to1), screenX(m, kSteps, kIso2to1));
    TEST_ASSERT_EQUAL_INT(cellToScreenY(-1, -3, kIso2to1), screenY(m, kSteps, kIso2to1));
}

void test_interpolated_world_grid_spec_overload_is_unaffected(void) {
    // The new overload must not change which one an existing GridSpec call
    // resolves to, nor what it returns. Same motion, same steps, both specs:
    // the GridSpec call keeps producing GridSpace's axis-aligned answer.
    GridMotion m;
    placeAt(m, 1, 1);
    beginStep(m, 2, 1);
    tickStep(m, kSteps);

    TEST_ASSERT_EQUAL_INT(cellToWorldX(1, kSquareGrid) +
                              (cellToWorldX(2, kSquareGrid) - cellToWorldX(1, kSquareGrid)) *
                                  m.progress / kSteps,
                          worldX(m, kSteps, kSquareGrid));

    // A step along cell X leaves world Y untouched under a GridSpec — the
    // behaviour that must survive the addition of the projection overload.
    TEST_ASSERT_EQUAL_INT(cellToWorldY(1, kSquareGrid), worldY(m, kSteps, kSquareGrid));
}

#endif  // PIXELROOT32_ENABLE_PROJECTION

#endif  // PIXELROOT32_ENABLE_GAMEPLAY_GRID_SPACE

void test_gameplay_grid_motion_zero_cost_when_disabled(void) {
#if PIXELROOT32_ENABLE_GAMEPLAY_GRID_SPACE
    TEST_PASS_MESSAGE(
        "PIXELROOT32_ENABLE_GAMEPLAY_GRID_SPACE=1: GridMotion compiled and "
        "exercised by the tests above.");
#else
    // With the flag off, GridMotion and every function operating on it are
    // not compiled at all — their entire declaration lives inside the
    // #if PIXELROOT32_ENABLE_GAMEPLAY_GRID_SPACE guard in
    // include/gameplay/GridMotion.h. This translation unit compiling and
    // passing without referencing any of them IS the "zero bytes reserved"
    // property.
    TEST_PASS_MESSAGE(
        "PIXELROOT32_ENABLE_GAMEPLAY_GRID_SPACE=0: GridMotion is not compiled, "
        "zero bytes reserved.");
#endif
}

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
    RUN_TEST(test_default_constructed_motion_is_at_rest_in_cell_zero_zero);
    RUN_TEST(test_grid_motion_is_a_plain_five_int_aggregate);
    RUN_TEST(test_place_at_sets_logical_cell_target_and_clears_progress);
    RUN_TEST(test_place_at_cancels_an_in_flight_step);
    RUN_TEST(test_begin_step_arms_the_target_without_moving_the_logical_cell);
    RUN_TEST(test_tick_step_at_rest_is_a_no_op_and_reports_no_arrival);
    RUN_TEST(test_tick_step_advances_progress_without_arriving_mid_flight);
    RUN_TEST(test_tick_step_commits_the_logical_cell_exactly_on_the_arrival_tick);
    RUN_TEST(test_tick_step_reports_arrival_once_not_on_subsequent_ticks);
    RUN_TEST(test_a_full_traversal_takes_exactly_steps_per_cell_frames);
    RUN_TEST(test_consecutive_steps_chain_without_drift);
    RUN_TEST(test_interpolated_world_at_rest_equals_cell_to_world);
    RUN_TEST(test_interpolated_world_is_exact_at_both_endpoints);
    RUN_TEST(test_interpolated_world_advances_monotonically_and_stays_between_endpoints);
    RUN_TEST(test_interpolated_world_never_leads_the_true_position_in_either_direction);
    RUN_TEST(test_interpolated_world_moves_backwards_for_a_negative_direction_step);
    RUN_TEST(test_interpolated_world_handles_negative_cells);
    RUN_TEST(test_interpolated_world_respects_a_non_zero_grid_origin);
    RUN_TEST(test_interpolated_world_scales_each_axis_by_its_own_cell_size);
    RUN_TEST(test_interpolated_world_agrees_with_grid_space_round_trip_after_arrival);
#if PIXELROOT32_ENABLE_PROJECTION
    RUN_TEST(test_interpolated_world_projection_endpoints_match_cell_to_screen);
    RUN_TEST(test_interpolated_world_projection_moves_on_both_screen_axes);
    RUN_TEST(test_interpolated_world_projection_handles_negative_cells);
    RUN_TEST(test_interpolated_world_grid_spec_overload_is_unaffected);
#endif
#endif
    RUN_TEST(test_gameplay_grid_motion_zero_cost_when_disabled);

    return UNITY_END();
}
