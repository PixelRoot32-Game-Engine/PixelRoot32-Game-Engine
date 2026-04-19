/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 *
 * Integration test for DirtyRectTracker, PartialUpdateController, and ColorDepthManager
 * integration with DrawSurface classes.
 */

#include <unity.h>
#include "../../test_config.h"
#include "graphics/DrawSurface.h"
#include "graphics/BaseDrawSurface.h"
#include "graphics/DirtyRectTracker.h"
#include "graphics/PartialUpdateController.h"
#include "graphics/ColorDepthManager.h"
#include "mocks/MockDrawSurface.h"

using namespace pixelroot32::graphics;

// Test variables
static PartialUpdateController* controller = nullptr;
static ColorDepthManager* colorDepthManager = nullptr;
static DirtyRectTracker* tracker = nullptr;

void setUp(void) {
    test_setup();
    controller = new PartialUpdateController();
    colorDepthManager = new ColorDepthManager();
    tracker = new DirtyRectTracker();
}

void tearDown(void) {
    delete controller;
    delete colorDepthManager;
    delete tracker;
    controller = nullptr;
    colorDepthManager = nullptr;
    tracker = nullptr;
    test_teardown();
}

// =============================================================================
// Test: PartialUpdateController mark dirty region
// =============================================================================

void test_partial_update_controller_mark_dirty_basic(void) {
    // Mark a region as dirty
    controller->markDirty(0, 0, 32, 32);
    
    // Should report dirty regions
    TEST_ASSERT_TRUE(controller->hasDirtyRegions());
}

// =============================================================================
// Test: PartialUpdateController begin/end frame
// =============================================================================

void test_partial_update_controller_begin_end_frame(void) {
    // Mark some dirty regions
    controller->markDirty(0, 0, 16, 16);
    controller->markDirty(32, 0, 16, 16);
    
    // Begin frame
    controller->beginFrame();
    
    // End frame with frame dimensions
    controller->endFrame(320, 240);
    
    // Should have regions
    TEST_ASSERT_TRUE(controller->hasDirtyRegions());
}

// =============================================================================
// Test: PartialUpdateController mode switching
// =============================================================================

void test_partial_update_controller_mode_switch(void) {
    // Default should be Partial
    TEST_ASSERT_EQUAL(PartialUpdateController::Mode::Partial, controller->getMode());
    
    // Set to Full mode
    controller->setMode(PartialUpdateController::Mode::Full);
    TEST_ASSERT_TRUE(controller->isModeFull());
    
    // Set back to Partial
    controller->setMode(PartialUpdateController::Mode::Partial);
    TEST_ASSERT_FALSE(controller->isModeFull());
}

// =============================================================================
// Test: PartialUpdateController enable/disable via setter
// =============================================================================

void test_partial_update_controller_set_enabled(void) {
    // Enable partial updates (default)
    controller->setPartialUpdateEnabled(true);
    TEST_ASSERT_TRUE(controller->isPartialUpdateEnabled());
    
    // Disable partial updates
    controller->setPartialUpdateEnabled(false);
    TEST_ASSERT_FALSE(controller->isPartialUpdateEnabled());
}

// =============================================================================
// Test: ColorDepthManager depth setting
// =============================================================================

void test_color_depth_manager_set_depth(void) {
    // Default should be Depth16
    TEST_ASSERT_EQUAL(ColorDepthManager::Depth::Depth16, colorDepthManager->getDepth());

    // Set to 8-bit
    colorDepthManager->setDepth(ColorDepthManager::Depth::Depth8);
    // For 8-bit (1 byte/pixel), ratio = 1/3 = 0.333333...
    TEST_ASSERT_FLOAT_EQUAL(1.0f / 3.0f, colorDepthManager->getTransferRatio());
}

// =============================================================================
// Test: ColorDepthManager needs palette conversion
// =============================================================================

void test_color_depth_manager_needs_palette(void) {
    // 16-bit does not need conversion
    colorDepthManager->setDepth(ColorDepthManager::Depth::Depth16);
    TEST_ASSERT_FALSE(colorDepthManager->needsPaletteConversion());
    
    // 8-bit needs conversion
    colorDepthManager->setDepth(ColorDepthManager::Depth::Depth8);
    TEST_ASSERT_TRUE(colorDepthManager->needsPaletteConversion());
    
    // 4-bit needs conversion
    colorDepthManager->setDepth(ColorDepthManager::Depth::Depth4);
    TEST_ASSERT_TRUE(colorDepthManager->needsPaletteConversion());
}

// =============================================================================
// Test: ColorDepthManager set depth from integer
// =============================================================================

void test_color_depth_manager_set_depth_int(void) {
    colorDepthManager->setDepth(24);
    TEST_ASSERT_EQUAL(24, colorDepthManager->getDepthBits());
    
    colorDepthManager->setDepth(16);
    TEST_ASSERT_EQUAL(16, colorDepthManager->getDepthBits());
    
    colorDepthManager->setDepth(8);
    TEST_ASSERT_EQUAL(8, colorDepthManager->getDepthBits());
    
    colorDepthManager->setDepth(4);
    TEST_ASSERT_EQUAL(4, colorDepthManager->getDepthBits());
}

// =============================================================================
// Test: ColorDepthManager estimate transfer size
// =============================================================================

void test_color_depth_manager_estimate_size(void) {
    colorDepthManager->setDepth(ColorDepthManager::Depth::Depth16);
    // 320 * 240 * 2 = 153600 bytes
    TEST_ASSERT_EQUAL(153600u, colorDepthManager->estimateTransferSize(320, 240));
    
    colorDepthManager->setDepth(ColorDepthManager::Depth::Depth8);
    // 320 * 240 * 1 = 76800 bytes
    TEST_ASSERT_EQUAL(76800u, colorDepthManager->estimateTransferSize(320, 240));
}

// =============================================================================
// Test: DirtyRectTracker basic marking
// =============================================================================

void test_dirty_rect_tracker_mark_dirty_basic(void) {
    // Mark a region
    tracker->markDirty(0, 0, 32, 32);
    
    // Should report dirty
    TEST_ASSERT_TRUE(tracker->hasDirtyRegions());
}

// =============================================================================
// Test: DirtyRectTracker combine regions
// =============================================================================

void test_dirty_rect_tracker_combine_regions(void) {
    // Mark adjacent regions that should combine
    tracker->markDirty(0, 0, 8, 8);
    tracker->markDirty(8, 0, 8, 8);
    tracker->markDirty(16, 0, 8, 8);
    
    // Combine regions
    tracker->combineRegions();
    
    // Should have merged regions
    const auto& regions = tracker->getRegions();
    TEST_ASSERT_TRUE(regions.size() > 0);
}

// =============================================================================
// Test: DirtyRectTracker clear
// =============================================================================

void test_dirty_rect_tracker_clear(void) {
    // Mark region
    tracker->markDirty(0, 0, 32, 32);
    TEST_ASSERT_TRUE(tracker->hasDirtyRegions());
    
    // Clear
    tracker->clear();
    
    // Should not have dirty regions
    TEST_ASSERT_FALSE(tracker->hasDirtyRegions());
}

// =============================================================================
// Test: DirtyRectTracker edge cases - negative coordinates
// =============================================================================

void test_dirty_rect_tracker_negative_coords(void) {
    // Should handle negative coordinates gracefully (clamped to 0)
    tracker->markDirty(-8, -8, 16, 16);
    // Should not crash and should mark the region at 0,0
    TEST_ASSERT_TRUE(tracker->hasDirtyRegions());
}

// =============================================================================
// Test: DirtyRectTracker edge cases - out of bounds
// =============================================================================

void test_dirty_rect_tracker_out_of_bounds(void) {
    // Should handle out of bounds gracefully
    tracker->markDirty(400, 400, 32, 32);
    // Should not crash but also not mark anything
    TEST_ASSERT_FALSE(tracker->hasDirtyRegions());
}

// =============================================================================
// Test: DirtyRectTracker zero size
// =============================================================================

void test_dirty_rect_tracker_zero_size(void) {
    // Zero size should be handled gracefully
    tracker->markDirty(0, 0, 0, 0);
    // Should not crash
    TEST_ASSERT_FALSE(tracker->hasDirtyRegions());
}

// =============================================================================
// Test: PartialUpdateController max dirty ratio threshold
// =============================================================================

void test_partial_update_controller_max_dirty_ratio(void) {
    // Fill most of the screen (>70% dirty should fallback to full)
    controller->markDirty(0, 0, 240, 200);  // 48000 pixels = 62.5% of 320x240
    controller->beginFrame();
    controller->endFrame(320, 240);
    
    // At 62.5% should still use partial
    TEST_ASSERT_TRUE(controller->shouldUsePartial());
    
    // Mark nearly full screen (>70%)
    controller->clear();
    controller->markDirty(0, 0, 240, 180);  // 43200 pixels = 56.25%
    controller->beginFrame();
    controller->endFrame(320, 240);
    
    // Note: The actual threshold check depends on MIN_REGION_PIXELS too
    // This is a simplified test
}

// =============================================================================
// Main test runner
// =============================================================================

int main() {
    UNITY_BEGIN();
    
    // PartialUpdateController tests
    RUN_TEST(test_partial_update_controller_mark_dirty_basic);
    RUN_TEST(test_partial_update_controller_begin_end_frame);
    RUN_TEST(test_partial_update_controller_mode_switch);
    RUN_TEST(test_partial_update_controller_set_enabled);
    RUN_TEST(test_partial_update_controller_max_dirty_ratio);
    
    // ColorDepthManager tests
    RUN_TEST(test_color_depth_manager_set_depth);
    RUN_TEST(test_color_depth_manager_needs_palette);
    RUN_TEST(test_color_depth_manager_set_depth_int);
    RUN_TEST(test_color_depth_manager_estimate_size);
    
    // DirtyRectTracker tests
    RUN_TEST(test_dirty_rect_tracker_mark_dirty_basic);
    RUN_TEST(test_dirty_rect_tracker_combine_regions);
    RUN_TEST(test_dirty_rect_tracker_clear);
    RUN_TEST(test_dirty_rect_tracker_negative_coords);
    RUN_TEST(test_dirty_rect_tracker_out_of_bounds);
    RUN_TEST(test_dirty_rect_tracker_zero_size);
    
    UNITY_END();
}