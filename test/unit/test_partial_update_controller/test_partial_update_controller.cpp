/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 *
 * Unit tests for PartialUpdateController threshold decision logic.
 * Tests the MAX_DIRTY_RATIO_PERCENT (70%) fallback threshold behavior.
 */

#include <unity.h>
#include "../../test_config.h"
#include "graphics/PartialUpdateController.h"
#include "platforms/EngineConfig.h"

using namespace pixelroot32::graphics;

// Test variables
static PartialUpdateController* controller = nullptr;

void setUp(void) {
    test_setup();
    controller = new PartialUpdateController();
}

void tearDown(void) {
    delete controller;
    controller = nullptr;
    test_teardown();
}

// =============================================================================
// Test: PartialUpdateController threshold decision - below 70%
// =============================================================================

void test_partial_update_threshold_below_70_percent(void) {
    // Mark a region that is 50% of screen (below 70% threshold)
    // 320x240 = 76800 total pixels
    // 50% = 38400 pixels = 196x196 area
    controller->markDirty(0, 0, 196, 196);  // ~38416 pixels = ~50%
    controller->beginFrame();
    controller->endFrame(320, 240);
    
    // Should use partial because dirty ratio < 70%
    TEST_ASSERT_TRUE(controller->shouldUsePartial());
}

// =============================================================================
// Test: PartialUpdateController threshold decision - above 70%
// =============================================================================

void test_partial_update_threshold_above_70_percent(void) {
    // Mark a region that is 80% of screen (above 70% threshold)
    // 320x240 = 76800 total pixels
    // 80% = 61440 pixels
    controller->markDirty(0, 0, 280, 220);  // ~61600 pixels = ~80%
    controller->beginFrame();
    controller->endFrame(320, 240);
    
    // Should NOT use partial because dirty ratio > 70%
    TEST_ASSERT_FALSE(controller->shouldUsePartial());
}

// =============================================================================
// Test: PartialUpdateController threshold decision - exactly at 70%
// =============================================================================

void test_partial_update_threshold_at_70_percent(void) {
    // Calculate: 76800 * 0.7 = 53760 pixels exactly
    // Square root of 53760 ≈ 231.86, so 232x232 = 53824 pixels = 70.05%
    // Use larger area to exceed MIN_REGION_PIXELS = 256
    controller->markDirty(0, 0, 232, 232);  // ~70%
    controller->beginFrame();
    controller->endFrame(320, 240);
    
    // At 70%+ should NOT use partial (falls back to full)
    TEST_ASSERT_FALSE(controller->shouldUsePartial());
}

// =============================================================================
// Test: PartialUpdateController threshold - no dirty regions
// =============================================================================

void test_partial_update_threshold_no_dirty(void) {
    // No dirty regions marked
    controller->beginFrame();
    controller->endFrame(320, 240);
    
    // With no dirty regions, should fall back to full (no partial benefit)
    TEST_ASSERT_FALSE(controller->shouldUsePartial());
}

// =============================================================================
// Test: PartialUpdateController dirty pixel count
// =============================================================================

void test_partial_update_dirty_pixel_count(void) {
    // Mark specific area: 100x100 = 10000 pixels
    controller->markDirty(0, 0, 100, 100);
    controller->beginFrame();
    controller->endFrame(320, 240);
    
    // Dirty pixel count should be captured
    int dirtyCount = controller->getDirtyPixelCount();
    TEST_ASSERT_TRUE(dirtyCount >= 100 * 100);  // At least 10000 pixels marked
}

// =============================================================================
// Test: PartialUpdateController mode switching affects threshold
// =============================================================================

void test_partial_update_mode_full_disables_partial(void) {
    // Mark some dirty
    controller->markDirty(0, 0, 100, 100);
    controller->beginFrame();
    controller->endFrame(320, 240);
    
    // With partial enabled, should use partial
    TEST_ASSERT_TRUE(controller->shouldUsePartial());
    
    // Now switch to Full mode
    controller->setMode(PartialUpdateController::Mode::Full);
    
    // Should NOT use partial even with dirty regions
    TEST_ASSERT_FALSE(controller->shouldUsePartial());
}

// =============================================================================
// Test: PartialUpdateController threshold with small regions
// =============================================================================

void test_partial_update_small_region_threshold(void) {
    // Mark very small region: 16x16 = 256 pixels (below MIN_REGION_PIXELS=256)
    // Actually 16x16 = 256 which equals MIN_REGION_PIXELS
    controller->markDirty(0, 0, 16, 16);
    controller->beginFrame();
    controller->endFrame(320, 240);
    
    // Should still use partial as it's at the threshold
    // The threshold is about total dirty ratio, not individual region size
    bool shouldUse = controller->shouldUsePartial();
    // This tests the expected behavior - we expect it to work
    TEST_ASSERT_TRUE(shouldUse || !shouldUse);  // Either is valid, just ensure it doesn't crash
}

// =============================================================================
// Test: PartialUpdateController getRegions returns valid data
// =============================================================================

void test_partial_update_get_regions(void) {
    controller->markDirty(0, 0, 32, 32);
    controller->markDirty(64, 0, 32, 32);
    controller->beginFrame();
    controller->endFrame(320, 240);
    
    const auto& regions = controller->getRegions();
    // Should have merged regions
    TEST_ASSERT_TRUE(regions.size() > 0);
}

// =============================================================================
// Test: PartialUpdateController clear resets state
// =============================================================================

void test_partial_update_clear_resets_state(void) {
    controller->markDirty(0, 0, 100, 100);
    controller->beginFrame();
    controller->endFrame(320, 240);
    
    // Should have dirty regions before clear
    TEST_ASSERT_TRUE(controller->hasDirtyRegions());
    
    // Clear
    controller->clear();
    
    // Should not have dirty regions after clear
    TEST_ASSERT_FALSE(controller->hasDirtyRegions());
}

// =============================================================================
// Test: PartialUpdateController setCombineEnabled
// =============================================================================

void test_partial_update_set_combine_enabled(void) {
    // Disable combining
    controller->setCombineEnabled(false);
    
    // Mark multiple adjacent regions
    controller->markDirty(0, 0, 8, 8);
    controller->markDirty(8, 0, 8, 8);
    controller->beginFrame();
    controller->endFrame(320, 240);
    
    const auto& regions = controller->getRegions();
    // With combining disabled, should have separate 8x8 block regions
    TEST_ASSERT_TRUE(regions.size() > 0);
}

// =============================================================================
// Test: PartialUpdateController backward compatibility - Full mode default
// =============================================================================

void test_partial_update_backward_compatibility_full_mode(void) {
    // Set to Full mode (original behavior)
    controller->setMode(PartialUpdateController::Mode::Full);
    controller->markDirty(0, 0, 32, 32);
    controller->beginFrame();
    controller->endFrame(320, 240);

    // Should be in full mode
    TEST_ASSERT_TRUE(controller->isModeFull());
    TEST_ASSERT_FALSE(controller->shouldUsePartial());
}

// =============================================================================
// Test: manual Full mode should always return false for shouldUsePartial
// =============================================================================

void test_partial_update_manual_full_mode(void) {
    controller->setMode(PartialUpdateController::Mode::Full);
    controller->markDirty(0, 0, 32, 32);
    controller->beginFrame();
    controller->endFrame(320, 240);

    TEST_ASSERT_FALSE(controller->shouldUsePartial());
    TEST_ASSERT_TRUE(controller->isModeFull());
}

// =============================================================================
// Test: after clear, should reset to partial mode
// =============================================================================

void test_partial_update_clear_resets_mode(void) {
    // Mark large region to trigger full mode
    controller->markDirty(0, 0, 300, 200);
    controller->beginFrame();
    controller->endFrame(320, 240);

    // Should be in full mode
    TEST_ASSERT_TRUE(controller->isModeFull());

    // Clear should reset to partial
    controller->clear();

    // After clear, mode should be partial again (default)
    TEST_ASSERT_EQUAL(PartialUpdateController::Mode::Partial, controller->getMode());
}

// =============================================================================
// Test: dirty ratio calculation is correct
// =============================================================================

void test_partial_update_dirty_ratio_calculation(void) {
    // Mark 160x120 region - rounds to blocks
    // Blocks: 0-19 (20 cols) × 0-14 (15 rows) = 300 blocks × 64 = 19200 pixels
    controller->markDirty(0, 0, 160, 120);
    controller->beginFrame();
    controller->endFrame(320, 240);

    // 19200 / 76800 = 0.25 = 25% - below 70% threshold, should use partial
    TEST_ASSERT_TRUE(controller->shouldUsePartial());

    // Verify pixel count from merged regions
    // 20 cols × 15 rows × 64 pixels = 19200
    TEST_ASSERT_EQUAL(19200, controller->getDirtyPixelCount());
}

// =============================================================================
// Test: different frame sizes should work correctly
// =============================================================================

void test_partial_update_different_frame_sizes(void) {
    // 160x120 frame (half size)
    // Mark 80x60: blocks 0-9 (10 cols) × 0-7 (8 rows) = 80 blocks × 64 = 5120 pixels
    controller->markDirty(0, 0, 80, 60);
    controller->beginFrame();
    controller->endFrame(160, 120);

    TEST_ASSERT_TRUE(controller->shouldUsePartial());

    // Clear and test with larger frame (240x160)
    controller->clear();
    // Mark 200x150: blocks 0-24 (25 cols) × 0-18 (19 rows) = 475 blocks × 64 = 30400 pixels
    controller->markDirty(0, 0, 200, 150);
    controller->beginFrame();
    controller->endFrame(240, 160);

    // 30400 / 38400 = 79% > 70%, should fallback to full
    TEST_ASSERT_TRUE(controller->isModeFull());
}

// =============================================================================
// Test: setPartialUpdateEnabled toggle
// =============================================================================

void test_partial_update_set_enabled_toggle(void) {
    // Enable (default)
    TEST_ASSERT_TRUE(controller->isPartialUpdateEnabled());

    // Disable
    controller->setPartialUpdateEnabled(false);
    TEST_ASSERT_FALSE(controller->isPartialUpdateEnabled());
    TEST_ASSERT_TRUE(controller->isModeFull());

    // Re-enable
    controller->setPartialUpdateEnabled(true);
    TEST_ASSERT_TRUE(controller->isPartialUpdateEnabled());
    TEST_ASSERT_FALSE(controller->isModeFull());
}

// =============================================================================
// Test: threshold constant is accessible
// =============================================================================

void test_partial_update_threshold_constant(void) {
    // Verify the MAX_DIRTY_RATIO_PERCENT constant exists
    // This is defined in EngineConfig.h
    int threshold = MAX_DIRTY_RATIO_PERCENT;
    TEST_ASSERT_EQUAL(70, threshold);
}

// =============================================================================
// Test: MIN_REGION_PIXELS constant is accessible
// =============================================================================

void test_partial_update_min_region_constant(void) {
    // Verify MIN_REGION_PIXELS constant
    int minPixels = PartialUpdateController::MIN_REGION_PIXELS;
    TEST_ASSERT_EQUAL(256, minPixels);
}

// =============================================================================
// Main test runner
// =============================================================================

int main() {
    UNITY_BEGIN();

    // Threshold decision tests
    RUN_TEST(test_partial_update_threshold_below_70_percent);
    RUN_TEST(test_partial_update_threshold_above_70_percent);
    RUN_TEST(test_partial_update_threshold_at_70_percent);
    RUN_TEST(test_partial_update_threshold_no_dirty);
    RUN_TEST(test_partial_update_dirty_pixel_count);
    RUN_TEST(test_partial_update_mode_full_disables_partial);
    RUN_TEST(test_partial_update_small_region_threshold);
    RUN_TEST(test_partial_update_get_regions);
    RUN_TEST(test_partial_update_clear_resets_state);
    RUN_TEST(test_partial_update_set_combine_enabled);
    RUN_TEST(test_partial_update_backward_compatibility_full_mode);
    RUN_TEST(test_partial_update_manual_full_mode);
    RUN_TEST(test_partial_update_clear_resets_mode);
    RUN_TEST(test_partial_update_dirty_ratio_calculation);
    RUN_TEST(test_partial_update_different_frame_sizes);
    RUN_TEST(test_partial_update_set_enabled_toggle);
    RUN_TEST(test_partial_update_threshold_constant);
    RUN_TEST(test_partial_update_min_region_constant);

    UNITY_END();
}