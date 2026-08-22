/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 *
 * Unit tests for DirtyGrid::intersectsPrevDirty — the prev-buffer
 * rectangle-intersection helper used by the projected tilemap dirty-skip.
 * Covers the contract from the tilemap-projected-dirty-skip spec:
 * fullDirty fast-path, fresh-grid safety, bounds clipping, zero-size
 * rectangles, and multi-cell hits.
 */

#include <unity.h>
#include "graphics/DirtyGrid.h"
#include "../../test_config.h"

using namespace pixelroot32::graphics;

void setUp(void) {
    test_setup();
}

void tearDown(void) {
    test_teardown();
}

// Scenario: fullDirty set → returns true immediately, without iterating cells.
void test_intersects_prev_dirty_full_dirty_fast_path(void) {
    DirtyGrid g;
    (void)g.init(64, 64);
    g.markAll();  // sets fullDirty == true

    // No swapAndClear(): prev is still empty, but fullDirty short-circuits.
    TEST_ASSERT_TRUE(g.intersectsPrevDirty(0, 0, 8, 8));
    TEST_ASSERT_TRUE(g.intersectsPrevDirty(200, 200, 8, 8));
}

// Scenario: fresh grid (zeroed prev, fullDirty false) → returns false; a
// default-constructed grid (prev == nullptr) is also safe and returns false.
void test_intersects_prev_dirty_fresh_grid_returns_false(void) {
    DirtyGrid g;
    (void)g.init(64, 64);
    TEST_ASSERT_FALSE(g.intersectsPrevDirty(0, 0, 32, 32));

    DirtyGrid uninit;  // prev == nullptr
    TEST_ASSERT_FALSE(uninit.intersectsPrevDirty(0, 0, 8, 8));
}

// Scenario: a rectangle clipped against grid bounds must not read out of
// range. A rect entirely outside returns false; a rect that only partially
// overlaps a dirty cell still finds it after clipping.
void test_intersects_prev_dirty_bounds_clipping(void) {
    DirtyGrid g;
    (void)g.init(32, 32);  // 4 cols × 4 rows → cells 0..3
    g.markCell(0, 0);
    g.swapAndClear();

    // Entirely to the left (negative x), no overlap → false.
    TEST_ASSERT_FALSE(g.intersectsPrevDirty(-16, 0, 16, 8));
    // Entirely beyond the right edge → false.
    TEST_ASSERT_FALSE(g.intersectsPrevDirty(40, 0, 16, 8));
    // Partially overlapping cell (0,0): clips to the valid range and finds it.
    TEST_ASSERT_TRUE(g.intersectsPrevDirty(-4, -4, 16, 16));
}

// Scenario: zero-size rect (w <= 0 or h <= 0) → returns false.
void test_intersects_prev_dirty_zero_size_returns_false(void) {
    DirtyGrid g;
    (void)g.init(32, 32);
    g.markRect(0, 0, 8, 8);
    g.swapAndClear();

    TEST_ASSERT_FALSE(g.intersectsPrevDirty(0, 0, 0, 8));
    TEST_ASSERT_FALSE(g.intersectsPrevDirty(0, 0, 8, 0));
    TEST_ASSERT_FALSE(g.intersectsPrevDirty(0, 0, -4, 8));
}

// Scenario: rect covers multiple cells, one dirty → returns true. A 32×16
// rect at (24,32) spans cells (3..6, 4..5); cell (4,5) is the single dirty
// cell, so the hit is detected. A rect covering no dirty cell returns false.
void test_intersects_prev_dirty_multi_cell_hit(void) {
    DirtyGrid g;
    (void)g.init(64, 64);  // 8 cols × 8 rows
    g.markCell(4, 5);
    g.swapAndClear();

    // Covers cells (3,4),(3,5),(4,4),(4,5),(5,4),(5,5),(6,4),(6,5) — includes (4,5).
    TEST_ASSERT_TRUE(g.intersectsPrevDirty(24, 32, 32, 16));
    // Covers cells (0,0),(0,1),(1,0),(1,1) — none dirty.
    TEST_ASSERT_FALSE(g.intersectsPrevDirty(0, 0, 16, 16));
}

int main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_intersects_prev_dirty_full_dirty_fast_path);
    RUN_TEST(test_intersects_prev_dirty_fresh_grid_returns_false);
    RUN_TEST(test_intersects_prev_dirty_bounds_clipping);
    RUN_TEST(test_intersects_prev_dirty_zero_size_returns_false);
    RUN_TEST(test_intersects_prev_dirty_multi_cell_hit);

    return UNITY_END();
}
