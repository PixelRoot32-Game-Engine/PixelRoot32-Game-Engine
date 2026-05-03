/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */

#include <unity.h>
#include <cstring>
#include "graphics/DirtyGrid.h"
#include "../../test_config.h"

using namespace pixelroot32::graphics;

void setUp(void) {
    test_setup();
}

void tearDown(void) {
    test_teardown();
}

void test_dirty_grid_init_grid_size_128(void) {
    DirtyGrid g;
    g.init(128, 128);
    TEST_ASSERT_EQUAL_UINT8(16, g.getCols());
    TEST_ASSERT_EQUAL_UINT8(16, g.getRows());
    TEST_ASSERT_FALSE(g.isFullDirty());
}

void test_dirty_grid_init_grid_size_240(void) {
    DirtyGrid g;
    g.init(240, 240);
    TEST_ASSERT_EQUAL_UINT8(30, g.getCols());
    TEST_ASSERT_EQUAL_UINT8(30, g.getRows());
}

void test_dirty_grid_mark_cell_swap_prev(void) {
    DirtyGrid g;
    g.init(64, 64);
    g.markCell(3, 2);
    TEST_ASSERT_FALSE(g.isPrevDirty(3, 2));
    g.swapAndClear();
    TEST_ASSERT_TRUE(g.isPrevDirty(3, 2));
}

void test_dirty_grid_mark_rect_covers_cells(void) {
    DirtyGrid g;
    g.init(32, 32);
    g.markRect(0, 0, 17, 8);
    TEST_ASSERT_FALSE(g.isPrevDirty(0, 0));
    TEST_ASSERT_FALSE(g.isPrevDirty(2, 0));
    g.swapAndClear();
    TEST_ASSERT_TRUE(g.isPrevDirty(0, 0));
    TEST_ASSERT_TRUE(g.isPrevDirty(1, 0));
    TEST_ASSERT_TRUE(g.isPrevDirty(2, 0));
    TEST_ASSERT_FALSE(g.isPrevDirty(0, 1));
}

void test_dirty_grid_mark_rect_clipped(void) {
    DirtyGrid g;
    g.init(16, 16);
    g.markRect(-50, -50, 100, 100);
    g.swapAndClear();
    TEST_ASSERT_TRUE(g.isPrevDirty(0, 0));
    TEST_ASSERT_TRUE(g.isPrevDirty(1, 1));
}

void test_dirty_grid_swap_clears_curr_next_frame_pattern(void) {
    DirtyGrid g;
    g.init(24, 24);
    g.markCell(0, 0);
    g.swapAndClear();
    TEST_ASSERT_TRUE(g.isPrevDirty(0, 0));
    g.markCell(1, 1);
    TEST_ASSERT_FALSE(g.isPrevDirty(1, 1));
    g.swapAndClear();
    TEST_ASSERT_FALSE(g.isPrevDirty(0, 0));
    TEST_ASSERT_TRUE(g.isPrevDirty(1, 1));
}

void test_dirty_grid_mark_all(void) {
    DirtyGrid g;
    g.init(8, 8);
    TEST_ASSERT_FALSE(g.isFullDirty());
    g.markAll();
    TEST_ASSERT_TRUE(g.isFullDirty());
}

void test_dirty_grid_out_of_bounds_mark_ignored(void) {
    DirtyGrid g;
    g.init(16, 16);
    g.markCell(255, 255);
    g.swapAndClear();
    TEST_ASSERT_FALSE(g.isPrevDirty(255, 255));
}

void test_dirty_grid_mark_rect_invalid_size(void) {
    DirtyGrid g;
    g.init(16, 16);
    g.markRect(0, 0, 0, 8);
    g.markRect(0, 0, 8, 0);
    g.swapAndClear();
    TEST_ASSERT_FALSE(g.isPrevDirty(0, 0));
}

void test_dirty_grid_popcount_prev_curr(void) {
    DirtyGrid g;
    g.init(16, 16);
    g.markCell(1, 0);
    TEST_ASSERT_EQUAL_UINT32(1u, g.countCurrMarkedCells());
    TEST_ASSERT_EQUAL_UINT32(0u, g.countPrevMarkedCells());
    g.swapAndClear();
    TEST_ASSERT_EQUAL_UINT32(1u, g.countPrevMarkedCells());
    TEST_ASSERT_EQUAL_UINT32(0u, g.countCurrMarkedCells());
}

void test_dirty_grid_clear_framebuffer8_from_prev_one_cell(void) {
    DirtyGrid g;
    g.init(16, 16);
    uint8_t buf[256];
    std::memset(buf, 0x7Fu, sizeof(buf));
    g.markRect(0, 0, 8, 8);
    g.swapAndClear();
    g.clearFramebuffer8FromPrev(buf, 16, 16, 0);
    TEST_ASSERT_EQUAL_UINT8(0, buf[0]);
    TEST_ASSERT_EQUAL_UINT8(0, buf[7]);
    TEST_ASSERT_EQUAL_UINT8(0, buf[16 + 7]);
    TEST_ASSERT_EQUAL_UINT8(0x7Fu, buf[16 * 15 + 15]);
}

void test_dirty_grid_clear_framebuffer8_row_run_merges_adjacent_cells(void) {
    DirtyGrid g;
    constexpr int kW = 24;
    constexpr int kH = 16;
    g.init(kW, kH);
    uint8_t buf[kW * kH];
    std::memset(buf, 0xCDu, sizeof(buf));
    g.markCell(0, 0);
    g.markCell(1, 0);
    g.swapAndClear();
    g.clearFramebuffer8FromPrev(buf, kW, kH, 0);
    for (int x = 0; x < 16; ++x) {
        TEST_ASSERT_EQUAL_UINT8(0, buf[x]);
    }
    for (int x = 16; x < kW; ++x) {
        TEST_ASSERT_EQUAL_UINT8(0xCDu, buf[x]);
    }
    TEST_ASSERT_EQUAL_UINT8(0xCDu, buf[8 * kW]);
}

int main() {
    UNITY_BEGIN();

    RUN_TEST(test_dirty_grid_init_grid_size_128);
    RUN_TEST(test_dirty_grid_init_grid_size_240);
    RUN_TEST(test_dirty_grid_mark_cell_swap_prev);
    RUN_TEST(test_dirty_grid_mark_rect_covers_cells);
    RUN_TEST(test_dirty_grid_mark_rect_clipped);
    RUN_TEST(test_dirty_grid_swap_clears_curr_next_frame_pattern);
    RUN_TEST(test_dirty_grid_mark_all);
    RUN_TEST(test_dirty_grid_out_of_bounds_mark_ignored);
    RUN_TEST(test_dirty_grid_mark_rect_invalid_size);
    RUN_TEST(test_dirty_grid_popcount_prev_curr);
    RUN_TEST(test_dirty_grid_clear_framebuffer8_from_prev_one_cell);
    RUN_TEST(test_dirty_grid_clear_framebuffer8_row_run_merges_adjacent_cells);

    return UNITY_END();
}
