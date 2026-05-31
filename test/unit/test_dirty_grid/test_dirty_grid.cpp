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
    (void)g.init(128, 128);
    TEST_ASSERT_EQUAL_UINT8(16, g.getCols());
    TEST_ASSERT_EQUAL_UINT8(16, g.getRows());
    TEST_ASSERT_FALSE(g.isFullDirty());
}

void test_dirty_grid_init_grid_size_240(void) {
    DirtyGrid g;
    (void)g.init(240, 240);
    TEST_ASSERT_EQUAL_UINT8(30, g.getCols());
    TEST_ASSERT_EQUAL_UINT8(30, g.getRows());
}

void test_dirty_grid_mark_cell_swap_prev(void) {
    DirtyGrid g;
    (void)g.init(64, 64);
    g.markCell(3, 2);
    TEST_ASSERT_FALSE(g.isPrevDirty(3, 2));
    g.swapAndClear();
    TEST_ASSERT_TRUE(g.isPrevDirty(3, 2));
}

void test_dirty_grid_mark_rect_covers_cells(void) {
    DirtyGrid g;
    (void)g.init(32, 32);
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
    (void)g.init(16, 16);
    g.markRect(-50, -50, 100, 100);
    g.swapAndClear();
    TEST_ASSERT_TRUE(g.isPrevDirty(0, 0));
    TEST_ASSERT_TRUE(g.isPrevDirty(1, 1));
}

void test_dirty_grid_swap_clears_curr_next_frame_pattern(void) {
    DirtyGrid g;
    (void)g.init(24, 24);
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
    (void)g.init(8, 8);
    TEST_ASSERT_FALSE(g.isFullDirty());
    g.markAll();
    TEST_ASSERT_TRUE(g.isFullDirty());
}

void test_dirty_grid_out_of_bounds_mark_ignored(void) {
    DirtyGrid g;
    (void)g.init(16, 16);
    g.markCell(255, 255);
    g.swapAndClear();
    TEST_ASSERT_FALSE(g.isPrevDirty(255, 255));
}

void test_dirty_grid_mark_rect_invalid_size(void) {
    DirtyGrid g;
    (void)g.init(16, 16);
    g.markRect(0, 0, 0, 8);
    g.markRect(0, 0, 8, 0);
    g.swapAndClear();
    TEST_ASSERT_FALSE(g.isPrevDirty(0, 0));
}

void test_dirty_grid_popcount_prev_curr(void) {
    DirtyGrid g;
    (void)g.init(16, 16);
    g.markCell(1, 0);
    TEST_ASSERT_EQUAL_UINT32(1u, g.countCurrMarkedCells());
    TEST_ASSERT_EQUAL_UINT32(0u, g.countPrevMarkedCells());
    g.swapAndClear();
    TEST_ASSERT_EQUAL_UINT32(1u, g.countPrevMarkedCells());
    TEST_ASSERT_EQUAL_UINT32(0u, g.countCurrMarkedCells());
}

void test_dirty_grid_clear_framebuffer8_from_prev_one_cell(void) {
    DirtyGrid g;
    (void)g.init(16, 16);
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
    (void)g.init(kW, kH);
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

// =============================================================================
// Phase 4: DirtyGrid partial byte path tests (DG-01 through DG-06)
// =============================================================================

// DG-01: Partial byte in middle of row — bits != 0xFF && bits != 0 path
void test_dirty_grid_clear_partial_byte_mid_row(void) {
    DirtyGrid g;
    (void)g.init(32, 8);   // 4 cols × 1 row
    uint8_t buf[32 * 8];   // 32px × 8px
    std::memset(buf, 0xFFu, sizeof(buf));

    // Mark cell 3 only (bit 3 in byte 0) — partial byte (0x08, not 0xFF)
    g.markCell(3, 0);
    g.swapAndClear();

    g.clearFramebuffer8FromPrev(buf, 32, 8, 0);

    // Cell 3 covers pixels (24,0) to (31,7)
    for (int y = 0; y < 8; ++y) {
        for (int x = 24; x < 32; ++x) {
            TEST_ASSERT_EQUAL_UINT8_MESSAGE(0, buf[y * 32 + x],
                "Cell 3 pixels should be cleared");
        }
        for (int x = 0; x < 24; ++x) {
            TEST_ASSERT_EQUAL_UINT8_MESSAGE(0xFFu, buf[y * 32 + x],
                "Cells 0-2 pixels should be untouched");
        }
    }
}

// DG-02: All-0xFF run spanning multiple bytes in same row
void test_dirty_grid_clear_run_spanning_two_bytes(void) {
    DirtyGrid g;
    constexpr int kW = 128;
    constexpr int kH = 8;
    (void)g.init(kW, kH);  // 16 cols × 1 row → 2 bytes per row
    uint8_t buf[kW * kH];
    std::memset(buf, 0xFFu, sizeof(buf));

    // Mark cells 0-15 (all 16 cols) → both bytes = 0xFF
    g.markRect(0, 0, kW, 8);
    g.swapAndClear();

    g.clearFramebuffer8FromPrev(buf, kW, kH, 0);

    // All 128×8 pixels should be cleared (merged memset run)
    for (int i = 0; i < kW * kH; ++i) {
        TEST_ASSERT_EQUAL_UINT8_MESSAGE(0, buf[i],
            "All pixels should be cleared for 0xFF run spanning 2 bytes");
    }
}

void test_dirty_grid_clear_run_spanning_three_cells(void) {
    DirtyGrid g;
    constexpr int kW = 24;
    constexpr int kH = 8;
    (void)g.init(kW, kH);  // 3 cols → 1 byte
    uint8_t buf[kW * kH];
    std::memset(buf, 0xFFu, sizeof(buf));

    // Mark cells 0,1,2 (3 consecutive cells in 1 byte)
    g.markCell(0, 0);
    g.markCell(1, 0);
    g.markCell(2, 0);
    g.swapAndClear();

    g.clearFramebuffer8FromPrev(buf, kW, kH, 0);

    // Pixels 0-23 should be 0, rest of buffer untouched (no more rows)
    for (int y = 0; y < kH; ++y) {
        for (int x = 0; x < kW; ++x) {
            TEST_ASSERT_EQUAL_UINT8_MESSAGE(0, buf[y * kW + x],
                "All pixels in first cell row should be cleared");
        }
    }
}

// DG-03: Partial byte at row end — bits != 0xFF path with partial row width
void test_dirty_grid_clear_partial_byte_row_end(void) {
    DirtyGrid g;
    constexpr int kW = 30;
    constexpr int kH = 8;
    (void)g.init(kW, kH);  // 3 cols → 1 byte (3/8 ceiling = 1)
    uint8_t buf[kW * kH];
    std::memset(buf, 0xFFu, sizeof(buf));

    // Mark last 2 cells (1 and 2; col 0 unmarked)
    g.markCell(1, 0);
    g.markCell(2, 0);
    g.swapAndClear();

    g.clearFramebuffer8FromPrev(buf, kW, kH, 0);

    // Cell 1 = pixels (8-15), Cell 2 = pixels (16-23)
    for (int y = 0; y < kH; ++y) {
        for (int x = 8; x < 24; ++x) {
            TEST_ASSERT_EQUAL_UINT8_MESSAGE(0, buf[y * kW + x],
                "Cells 1-2 pixels should be cleared");
        }
        // Cell 0 pixels unchanged
        for (int x = 0; x < 8; ++x) {
            TEST_ASSERT_EQUAL_UINT8_MESSAGE(0xFFu, buf[y * kW + x],
                "Cell 0 pixels should be untouched");
        }
    }
}

// DG-04: Skip zero-byte segments — gap of unmarked cells between marked cells
void test_dirty_grid_clear_skips_zero_bytes(void) {
    DirtyGrid g;
    constexpr int kW = 192;
    constexpr int kH = 8;
    (void)g.init(kW, kH);  // 24 cols → 3 bytes per row
    uint8_t buf[kW * kH];
    std::memset(buf, 0xFFu, sizeof(buf));

    // Mark cell 0 (byte 0) and cell 16 (byte 2) → byte 1 = 0x00
    g.markCell(0, 0);
    g.markCell(16, 0);
    g.swapAndClear();

    g.clearFramebuffer8FromPrev(buf, kW, kH, 0);

    // Cell 0 pixels (0-7) cleared
    for (int y = 0; y < kH; ++y) {
        for (int x = 0; x < 8; ++x) {
            TEST_ASSERT_EQUAL_UINT8_MESSAGE(0, buf[y * kW + x],
                "Cell 0 pixels should be cleared");
        }
    }
    // Cell 16 pixels (128-135) cleared
    for (int y = 0; y < kH; ++y) {
        for (int x = 128; x < 136; ++x) {
            TEST_ASSERT_EQUAL_UINT8_MESSAGE(0, buf[y * kW + x],
                "Cell 16 pixels should be cleared");
        }
    }
    // Middle bytes (cells 8-15, pixels 64-127) should still be 0xFF
    for (int y = 0; y < kH; ++y) {
        for (int x = 64; x < 128; ++x) {
            TEST_ASSERT_EQUAL_UINT8_MESSAGE(0xFFu, buf[y * kW + x],
                "Middle byte region should be untouched");
        }
    }
}

// DG-05: Clear is clamped to framebuffer width
void test_dirty_grid_clear_clamped_to_framebuffer(void) {
    DirtyGrid g;
    constexpr int kGridW = 64;
    constexpr int kFbW = 20;
    constexpr int kH = 8;
    (void)g.init(kGridW, kH);  // 8 cols × 1 row
    uint8_t buf[kFbW * kH];
    std::memset(buf, 0xFFu, sizeof(buf));

    // Mark cell 3 (pixels 24-31) — beyond framebuffer width 20
    g.markCell(3, 0);
    g.swapAndClear();

    // Should not crash — cell 3 start px=24 ≥ framebufferWidth=20, so skipped
    g.clearFramebuffer8FromPrev(buf, kFbW, kH, 0);

    // All pixels should remain 0xFF since cell 3 is clamped out
    for (int i = 0; i < kFbW * kH; ++i) {
        TEST_ASSERT_EQUAL_UINT8_MESSAGE(0xFFu, buf[i],
            "No pixels should change when marked cell is beyond framebuffer");
    }
}

// DG-06: prev=nullptr → early return (default constructed grid)
void test_dirty_grid_clear_prev_nullptr(void) {
    DirtyGrid g;  // not initialized → prev = nullptr
    uint8_t buf[64];
    std::memset(buf, 0xFFu, sizeof(buf));

    // Should return early without crashing
    g.clearFramebuffer8FromPrev(buf, 16, 16, 0);

    // Buffer untouched
    for (int i = 0; i < 64; ++i) {
        TEST_ASSERT_EQUAL_UINT8_MESSAGE(0xFFu, buf[i],
            "Buffer should be untouched when prev=nullptr");
    }
}

// =============================================================================
// main
// =============================================================================

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

    // Phase 4: DirtyGrid partial byte path tests
    RUN_TEST(test_dirty_grid_clear_partial_byte_mid_row);
    RUN_TEST(test_dirty_grid_clear_run_spanning_two_bytes);
    RUN_TEST(test_dirty_grid_clear_run_spanning_three_cells);
    RUN_TEST(test_dirty_grid_clear_partial_byte_row_end);
    RUN_TEST(test_dirty_grid_clear_skips_zero_bytes);
    RUN_TEST(test_dirty_grid_clear_clamped_to_framebuffer);
    RUN_TEST(test_dirty_grid_clear_prev_nullptr);

    return UNITY_END();
}
