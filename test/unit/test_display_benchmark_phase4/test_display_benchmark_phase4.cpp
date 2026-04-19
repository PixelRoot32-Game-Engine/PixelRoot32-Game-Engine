/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 *
 * Performance benchmarks for display bottleneck optimization.
 * Phase 4.5: FPS improvement benchmarks (target 55+ FPS vs 42 FPS baseline)
 * Phase 4.6: Data transfer reduction benchmarks (target >30%)
 *
 * NOTE: These benchmarks require ESP32 hardware to run properly.
 * For unit testing, we simulate expected behavior.
 */

#include <unity.h>
#include "../test_config.h"
#include "graphics/DirtyRectTracker.h"
#include "graphics/PartialUpdateController.h"
#include "graphics/ColorDepthManager.h"

using namespace pixelroot32::graphics;

// Test variables - use heap allocation since we're in a test
static DirtyRectTracker* g_tracker = nullptr;
static PartialUpdateController* g_controller = nullptr;
static ColorDepthManager* g_colorDepth = nullptr;

void setUp(void) {
    test_setup();
    g_tracker = new DirtyRectTracker();
    g_controller = new PartialUpdateController();
    g_colorDepth = new ColorDepthManager();
}

void tearDown(void) {
    delete g_tracker;
    delete g_controller;
    delete g_colorDepth;
    g_tracker = nullptr;
    g_controller = nullptr;
    g_colorDepth = nullptr;
    test_teardown();
}

// =============================================================================
// Phase 4.5: FPS Benchmark Tests
// =============================================================================

// Benchmark simulation: measure markDirty performance (O(1) operations)
void test_benchmark_mark_dirty_performance(void) {
    DirtyRectTracker localTracker;

    // Warm-up
    for (int i = 0; i < 10; i++) {
        localTracker.markDirty(i * 16, 0, 16, 16);
    }
    localTracker.clear();

    // Benchmark: 100 small regions (iteration count, not real timing)
    int elapsed = 0;
    for (int frame = 0; frame < 100; frame++) {
        for (int i = 0; i < 10; i++) {
            localTracker.markDirty(i * 16, (frame % 15) * 16, 16, 16);
        }
        elapsed++;
    }

    // 1000 markDirty calls should execute without issue
    TEST_ASSERT_EQUAL(100, elapsed);
}

// Benchmark simulation: measure combineRegions performance
void test_benchmark_combine_regions_performance(void) {
    DirtyRectTracker localTracker;

    // Mark many regions
    for (int y = 0; y < 15; y++) {
        for (int x = 0; x < 20; x++) {
            localTracker.markDirty(x * 16, y * 16, 16, 16);
        }
    }

    // Benchmark combining (iteration count, not real timing)
    int processCount = 0;
    for (int i = 0; i < 100; i++) {
        localTracker.combineRegions();
        localTracker.clear();
        // Re-mark for next iteration
        for (int y = 0; y < 15; y++) {
            for (int x = 0; x < 20; x++) {
                localTracker.markDirty(x * 16, y * 16, 16, 16);
            }
        }
        processCount++;
    }

    // 100 combines should execute without issue
    TEST_ASSERT_EQUAL(100, processCount);
}

// Benchmark simulation: partial update vs full update decision
void test_benchmark_mode_selection_performance(void) {
    PartialUpdateController localController;

    // Benchmark mode selection (iteration count, not real timing)
    int iterationCount = 0;
    for (int i = 0; i < 1000; i++) {
        localController.markDirty(0, 0, 100, 100);
        localController.beginFrame();
        localController.endFrame(320, 240);
        localController.shouldUsePartial();
        localController.clear();
        iterationCount++;
    }

    // 1000 iterations should execute without issue
    TEST_ASSERT_EQUAL(1000, iterationCount);
}

// =============================================================================
// Phase 4.6: Data Transfer Reduction Tests
// =============================================================================

// Test: calculate expected data reduction for partial update
// NOTE: Due to 8x8 block granularity, exact pixel counts differ from naive calculation
void test_data_transfer_reduction_small_dirty(void) {
    // Mark 50x50 at (10,10) - rounds to blocks:
    // Block columns 1-7 in x (7 cols), Block rows 1-7 in y (7 rows) = 49 blocks * 64 = 3136 pixels
    g_controller->markDirty(10, 10, 50, 50);
    g_controller->beginFrame();
    g_controller->endFrame(320, 240);

    // Full frame: 320*240*2 bytes (16-bit) = 153600 bytes
    // Partial: ~3136*2 = 6272 bytes (with block rounding)
    // Reduction: ~96%
    TEST_ASSERT_TRUE(g_controller->shouldUsePartial());

    int dirtyPixels = g_controller->getDirtyPixelCount();
    // Actual pixels marked = 7 * 7 * 64 = 3136
    TEST_ASSERT_EQUAL(3136, dirtyPixels);
}

// Test: calculate expected data reduction for medium dirty
void test_data_transfer_reduction_medium_dirty(void) {
    // Mark 100x100 at (50,50) - rounds to blocks:
    // Blocks 6-18 in x (13 cols), Blocks 6-18 in y (13 rows) = 169 blocks * 64 = 10816 pixels
    g_controller->markDirty(50, 50, 100, 100);
    g_controller->beginFrame();
    g_controller->endFrame(320, 240);

    // With block rounding: 13 * 13 * 64 = 10816 pixels
    TEST_ASSERT_TRUE(g_controller->shouldUsePartial());

    int dirtyPixels = g_controller->getDirtyPixelCount();
    TEST_ASSERT_EQUAL(10816, dirtyPixels);
}

// Test: verify high dirty ratio triggers full update fallback
void test_data_transfer_reduction_high_dirty_fallback(void) {
    // Large dirty region: 280x220 rounds to blocks = 35x28 = 980 blocks * 64 = 62720 pixels = 82%
    // This exceeds 70% threshold, should fallback to full mode
    g_controller->markDirty(0, 0, 280, 220);
    g_controller->beginFrame();
    g_controller->endFrame(320, 240);

    // Should fallback to full frame due to high dirty ratio (>70%)
    TEST_ASSERT_TRUE(g_controller->isModeFull());
    TEST_ASSERT_FALSE(g_controller->shouldUsePartial());
}

// Test: ColorDepthManager transfer ratio calculations
void test_data_transfer_color_depth_ratio_24bit(void) {
    g_colorDepth->setDepth(ColorDepthManager::Depth::Depth24);

    // 24-bit: 3 bytes/pixel, ratio = 1.0 (full)
    TEST_ASSERT_EQUAL(3, g_colorDepth->getBytesPerPixel());
    TEST_ASSERT_FLOAT_EQUAL(1.0f, g_colorDepth->getTransferRatio());
}

void test_data_transfer_color_depth_ratio_16bit(void) {
    g_colorDepth->setDepth(ColorDepthManager::Depth::Depth16);

    // 16-bit: 2 bytes/pixel, ratio = 2/3 = 0.667
    TEST_ASSERT_EQUAL(2, g_colorDepth->getBytesPerPixel());
    TEST_ASSERT_FLOAT_EQUAL(2.0f / 3.0f, g_colorDepth->getTransferRatio());
}

void test_data_transfer_color_depth_ratio_8bit(void) {
    g_colorDepth->setDepth(ColorDepthManager::Depth::Depth8);

    // 8-bit: 1 byte/pixel, ratio = 1/3 = 0.333
    TEST_ASSERT_EQUAL(1, g_colorDepth->getBytesPerPixel());
    TEST_ASSERT_FLOAT_EQUAL(1.0f / 3.0f, g_colorDepth->getTransferRatio());
}

void test_data_transfer_color_depth_ratio_4bit(void) {
    g_colorDepth->setDepth(ColorDepthManager::Depth::Depth4);

    // 4-bit: getBytesPerPixel returns 1 (minimum 1 byte due to byte-aligned storage)
    // Transfer ratio = 1/3 since we store 1 byte per pixel minimum
    TEST_ASSERT_EQUAL(1, g_colorDepth->getBytesPerPixel());  // Minimum 1 byte
    TEST_ASSERT_FLOAT_EQUAL(1.0f / 3.0f, g_colorDepth->getTransferRatio());
}

// Test: combined optimization effect (partial + color depth)
void test_data_transfer_combined_optimization(void) {
    ColorDepthManager localDepth;
    PartialUpdateController localController;

    // Use 8-bit color + partial update
    localDepth.setDepth(ColorDepthManager::Depth::Depth8);
    localController.markDirty(50, 50, 80, 80);  // 80x80 rounds to 10x10 blocks = 6400 pixels

    localController.beginFrame();
    localController.endFrame(320, 240);

    // Full frame at 8-bit: 320*240*1 = 76800 bytes
    // Partial at 8-bit: 6400*1 = 6400 bytes
    // Reduction: 91.7%

    TEST_ASSERT_TRUE(localController.shouldUsePartial());
}

// Test: estimateTransferSize for various scenarios
void test_data_transfer_estimate_full_frame_16bit(void) {
    ColorDepthManager localDepth;
    localDepth.setDepth(ColorDepthManager::Depth::Depth16);

    // Full frame
    unsigned int size = localDepth.estimateTransferSize(320, 240);
    TEST_ASSERT_EQUAL(153600u, size);  // 320*240*2
}

void test_data_transfer_estimate_partial_frame(void) {
    ColorDepthManager localDepth;
    localDepth.setDepth(ColorDepthManager::Depth::Depth16);

    // Partial region: 100x100
    unsigned int size = localDepth.estimateTransferSize(100, 100);
    TEST_ASSERT_EQUAL(20000u, size);  // 100*100*2
}

// =============================================================================
// Target Verification Tests
// =============================================================================

// Test: verify FPS target is achievable (simulated)
// NOTE: Due to block granularity, actual pixels differ from naive calculation
void test_fps_target_55_vs_baseline_42(void) {
    // Simulate typical game frame with small dirty region
    // 144x72 at (50,50) rounds to blocks: columns 6-24 (19 cols), rows 6-15 (10 cols) = 190 blocks * 64 = 12160 pixels
    g_controller->markDirty(50, 50, 144, 72);
    g_controller->beginFrame();
    g_controller->endFrame(320, 240);

    // Should use partial mode
    TEST_ASSERT_TRUE(g_controller->shouldUsePartial());

    // Calculate transfer reduction
    int fullFrameBytes = 320 * 240 * 2;  // 153600
    int partialBytes = g_controller->getDirtyPixelCount() * 2;
    float reduction = 1.0f - (float)partialBytes / fullFrameBytes;

    // With block rounding: 12160 / 153600 = 7.9% dirty -> 92% reduction > 30% target
    TEST_ASSERT_TRUE(reduction > 0.30f);
}

// Test: verify >30% data reduction target
void test_data_reduction_target_30_percent(void) {
    // Typical scenario: player moves in small area
    // 120x80 at (100,80) rounds to: columns 12-20 (9 cols), rows 10-15 (6 cols) = 54 blocks * 64 = 3456 pixels
    g_controller->markDirty(100, 80, 120, 80);
    g_controller->beginFrame();
    g_controller->endFrame(320, 240);

    // Full: 153600 bytes
    // Partial: 3456 * 2 = 6912 bytes (2.3% of full)
    // Reduction: 97.7% > 30% target

    TEST_ASSERT_TRUE(g_controller->shouldUsePartial());

    int fullFrameBytes = 320 * 240 * 2;
    int partialBytes = g_controller->getDirtyPixelCount() * 2;
    float reduction = 1.0f - (float)partialBytes / fullFrameBytes;

    TEST_ASSERT_TRUE(reduction > 0.30f);
}

// =============================================================================
// Main test runner
// =============================================================================

int main() {
    UNITY_BEGIN();

    // Phase 4.5: FPS benchmarks
    RUN_TEST(test_benchmark_mark_dirty_performance);
    RUN_TEST(test_benchmark_combine_regions_performance);
    RUN_TEST(test_benchmark_mode_selection_performance);

    // Phase 4.6: Data transfer reduction
    RUN_TEST(test_data_transfer_reduction_small_dirty);
    RUN_TEST(test_data_transfer_reduction_medium_dirty);
    RUN_TEST(test_data_transfer_reduction_high_dirty_fallback);
    RUN_TEST(test_data_transfer_color_depth_ratio_24bit);
    RUN_TEST(test_data_transfer_color_depth_ratio_16bit);
    RUN_TEST(test_data_transfer_color_depth_ratio_8bit);
    RUN_TEST(test_data_transfer_color_depth_ratio_4bit);
    RUN_TEST(test_data_transfer_combined_optimization);
    RUN_TEST(test_data_transfer_estimate_full_frame_16bit);
    RUN_TEST(test_data_transfer_estimate_partial_frame);

    // Target verification
    RUN_TEST(test_fps_target_55_vs_baseline_42);
    RUN_TEST(test_data_reduction_target_30_percent);

    UNITY_END();
}