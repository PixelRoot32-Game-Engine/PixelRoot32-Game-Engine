#include <unity.h>
#include "../../test_config.h"
#include "graphics/TileAnimation.h"
#include "platforms/EngineConfig.h"
#include <memory>
#include <cstring>

using namespace pixelroot32::graphics;

static TileAnimation testAnimations[3];
static uint16_t testTiles[256];
static TileAnimationManager* manager = nullptr;

void setUp(void) {
    test_setup();
    
    // Initialize test tiles (identity mapping initially)
    for (uint16_t i = 0; i < 256; i++) {
        testTiles[i] = i;
    }
    
    // Setup test animations
    testAnimations[0] = {10, 3, 5, 0};  // baseTile=10, frameCount=3, frameDuration=5
    testAnimations[1] = {20, 2, 4, 0};  // baseTile=20, frameCount=2, frameDuration=4  
    testAnimations[2] = {30, 4, 2, 0};  // baseTile=30, frameCount=4, frameDuration=2
    
    // Create manager
    manager = new TileAnimationManager(testAnimations, 3, 256);
}

void tearDown(void) {
    if (manager) {
        delete manager;
        manager = nullptr;
    }
    test_teardown();
}

void test_constructor_initializes_lookup_table(void) {
    // Verify lookup table is initialized to identity mapping
    for (uint16_t i = 0; i < 256; i++) {
        TEST_ASSERT_EQUAL_UINT8(i, manager->resolveFrame(i));
    }
}

void test_constructor_zero_animations(void) {
    TileAnimationManager emptyManager(nullptr, 0, 256);
    
    // Should still work with identity mapping
    TEST_ASSERT_EQUAL_UINT8(50, emptyManager.resolveFrame(50));
    TEST_ASSERT_EQUAL_UINT8(100, emptyManager.resolveFrame(100));
}

void test_step_advances_animations(void) {
    // Initial state: all tiles should map to themselves
    TEST_ASSERT_EQUAL_UINT8(10, manager->resolveFrame(10));
    
    // Step 5 times - animation should advance to frame 1
    for (int i = 0; i < 5; i++) {
        manager->step();
    }
    TEST_ASSERT_EQUAL_UINT8(11, manager->resolveFrame(10));
}

void test_step_multiple_animations_independent(void) {
    // Advance to different states for each animation
    // Animation 0: frameDuration=5, Animation 1: frameDuration=4, Animation 2: frameDuration=2
    
    // Step 8 times to get different states
    for (int i = 0; i < 8; i++) {
        manager->step();
    }
    
    // Animation 0: 8/5 = 1 remainder 3 -> frame 1
    TEST_ASSERT_EQUAL_UINT8(11, manager->resolveFrame(10));
    
    // Animation 1: 8/4 = 2 remainder 0 -> frame 0 (wrapped)  
    TEST_ASSERT_EQUAL_UINT8(20, manager->resolveFrame(20));
    
    // Animation 2: 8/2 = 4 remainder 0 -> frame 0 (wrapped)
    TEST_ASSERT_EQUAL_UINT8(30, manager->resolveFrame(30));
}

void test_resolveFrame_non_animated_tiles_unchanged(void) {
    // Non-animated tiles should pass through unchanged
    TEST_ASSERT_EQUAL_UINT8(0, manager->resolveFrame(0));
    TEST_ASSERT_EQUAL_UINT8(5, manager->resolveFrame(5));
    TEST_ASSERT_EQUAL_UINT8(100, manager->resolveFrame(100));
    TEST_ASSERT_EQUAL_UINT8(255, manager->resolveFrame(255));
    
    // Even after stepping
    manager->step();
    TEST_ASSERT_EQUAL_UINT8(0, manager->resolveFrame(0));
    TEST_ASSERT_EQUAL_UINT8(100, manager->resolveFrame(100));
}

void test_resolveFrame_bounds_checking(void) {
    // Out of bounds tiles should return themselves (or 0 for index 0)
    TEST_ASSERT_EQUAL_UINT8(0, manager->resolveFrame(0));
    
    // Test with smaller tile count
    TileAnimationManager smallManager(testAnimations, 1, 50);
    
    // Out of bounds should return input value
    TEST_ASSERT_EQUAL_UINT8(60, smallManager.resolveFrame(60));
    TEST_ASSERT_EQUAL_UINT8(100, smallManager.resolveFrame(100));
}

void test_reset_returns_to_initial_state(void) {
    // Advance animations
    for (int i = 0; i < 12; i++) {
        manager->step();
    }
    
    // Verify animations are advanced
    TEST_ASSERT_NOT_EQUAL_UINT8(10, manager->resolveFrame(10));
    
    // Reset
    manager->reset();
    
    // Verify back to initial state
    TEST_ASSERT_EQUAL_UINT8(10, manager->resolveFrame(10));
    TEST_ASSERT_EQUAL_UINT8(20, manager->resolveFrame(20));
    TEST_ASSERT_EQUAL_UINT8(30, manager->resolveFrame(30));
}

void test_single_frame_animation(void) {
    // Create animation with single frame
    TileAnimation singleFrameAnim = {50, 1, 3, 0};  // baseTile=50, frameCount=1, frameDuration=3
    TileAnimationManager singleManager(&singleFrameAnim, 1, 256);
    
    // Should always return the same frame regardless of steps
    TEST_ASSERT_EQUAL_UINT8(50, singleManager.resolveFrame(50));
    
    for (int i = 0; i < 10; i++) {
        singleManager.step();
        TEST_ASSERT_EQUAL_UINT8(50, singleManager.resolveFrame(50));
    }
}

void test_animation_wrapping(void) {
    // Test animation wraps back to frame 0
    TileAnimation wrapAnim = {40, 3, 2, 0};  // baseTile=40, frameCount=3, frameDuration=2
    TileAnimationManager wrapManager(&wrapAnim, 1, 256);
    
    // Frame progression: 40 -> 41 -> 42 -> 40 -> 41 -> 42...
    
    // Initial
    TEST_ASSERT_EQUAL_UINT8(40, wrapManager.resolveFrame(40));
    
    // Step 2 times -> frame 1
    wrapManager.step();
    wrapManager.step();
    TEST_ASSERT_EQUAL_UINT8(41, wrapManager.resolveFrame(40));
    
    // Step 2 more times -> frame 2
    wrapManager.step();
    wrapManager.step();
    TEST_ASSERT_EQUAL_UINT8(42, wrapManager.resolveFrame(40));
    
    // Step 2 more times -> back to frame 0
    wrapManager.step();
    wrapManager.step();
    TEST_ASSERT_EQUAL_UINT8(40, wrapManager.resolveFrame(40));
}

// Main test runner
int main(void) {
    UNITY_BEGIN();
    
    RUN_TEST(test_constructor_initializes_lookup_table);
    RUN_TEST(test_constructor_zero_animations);
    RUN_TEST(test_step_advances_animations);
    RUN_TEST(test_step_multiple_animations_independent);
    RUN_TEST(test_resolveFrame_non_animated_tiles_unchanged);
    RUN_TEST(test_resolveFrame_bounds_checking);
    RUN_TEST(test_reset_returns_to_initial_state);
    RUN_TEST(test_single_frame_animation);
    RUN_TEST(test_animation_wrapping);
    
    return UNITY_END();
}
