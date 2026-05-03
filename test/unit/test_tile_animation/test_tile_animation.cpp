/**
 * @file test_tile_animation.cpp
 * @brief Unit tests for graphics/TileAnimation module
 * @version 1.0
 * @date 2026-03-29
 * 
 * Tests for TileAnimationManager including:
 * - Lookup table initialization
 * - Animation step progression
 * - Frame resolution
 * - Reset functionality
 * - Multiple animations
 * - Edge cases (empty, single frame, wrapping)
 */

#include <unity.h>
#include "../../test_config.h"
#include "graphics/TileAnimation.h"

using namespace pixelroot32::graphics;

// One wall-clock step slightly above 1/60 s so tickAccum crosses 1000 (60 Hz pacing).
static constexpr unsigned long kOne60HzTickMs = 17u;

#define TEST_TILE_COUNT 64
#define MAX_ANIMATIONS 4

void setUp(void) {
    test_setup();
}

void tearDown(void) {
    test_teardown();
}

// =============================================================================
// Test fixtures
// =============================================================================

TileAnimation createAnimation(uint8_t baseIndex, uint8_t frameCount, uint8_t frameDuration) {
    TileAnimation anim;
    anim.baseTileIndex = baseIndex;
    anim.frameCount = frameCount;
    anim.frameDuration = frameDuration;
    anim.reserved = 0;
    return anim;
}

// =============================================================================
// Tests for Constructor - Lookup table initialization
// =============================================================================

void test_tile_animation_constructor_initializes_lookup_to_identity(void) {
    TileAnimation anims[0] = {};
    TileAnimationManager manager(anims, 0, 10);
    
    for (uint8_t i = 0; i < 10; i++) {
        TEST_ASSERT_EQUAL_UINT8(i, manager.resolveFrame(i));
    }
}

void test_tile_animation_constructor_with_single_animation(void) {
    TileAnimation anims[1] = { createAnimation(10, 3, 1) };
    TileAnimationManager manager(anims, 1, 20);

    // Frame 0: all slots in the sequence map to the visible tile (base + frame 0).
    TEST_ASSERT_EQUAL_UINT8(10, manager.resolveFrame(10));
    TEST_ASSERT_EQUAL_UINT8(10, manager.resolveFrame(11));
    TEST_ASSERT_EQUAL_UINT8(10, manager.resolveFrame(12));
}

void test_tile_animation_constructor_validates_tile_count(void) {
    TileAnimation anims[0] = {};
    TileAnimationManager manager(anims, 0, TEST_TILE_COUNT);
    
    TEST_ASSERT_EQUAL_UINT8(0, manager.resolveFrame(0));
    TEST_ASSERT_EQUAL_UINT8(10, manager.resolveFrame(10));
    TEST_ASSERT_EQUAL_UINT8(63, manager.resolveFrame(63));
}

// =============================================================================
// Tests for step() - Animation progression
// =============================================================================

void test_tile_animation_step_simple_animation(void) {
    TileAnimation anims[1] = { createAnimation(10, 3, 1) };
    TileAnimationManager manager(anims, 1, 20);
    
    manager.step(kOne60HzTickMs);
    
    TEST_ASSERT_EQUAL_UINT8(11, manager.resolveFrame(10));
    TEST_ASSERT_EQUAL_UINT8(11, manager.resolveFrame(11));
    TEST_ASSERT_EQUAL_UINT8(11, manager.resolveFrame(12));
}

void test_tile_animation_step_multiple_frames(void) {
    TileAnimation anims[1] = { createAnimation(5, 4, 1) };
    TileAnimationManager manager(anims, 1, 20);
    
    manager.step(kOne60HzTickMs);
    TEST_ASSERT_EQUAL_UINT8(6, manager.resolveFrame(5));
    TEST_ASSERT_EQUAL_UINT8(6, manager.resolveFrame(6));
    TEST_ASSERT_EQUAL_UINT8(6, manager.resolveFrame(7));
    TEST_ASSERT_EQUAL_UINT8(6, manager.resolveFrame(8));
    
    manager.step(kOne60HzTickMs);
    TEST_ASSERT_EQUAL_UINT8(7, manager.resolveFrame(5));
    TEST_ASSERT_EQUAL_UINT8(7, manager.resolveFrame(6));
    
    manager.step(kOne60HzTickMs);
    TEST_ASSERT_EQUAL_UINT8(8, manager.resolveFrame(5));
    
    manager.step(kOne60HzTickMs);
    TEST_ASSERT_EQUAL_UINT8(5, manager.resolveFrame(5));
}

void test_tile_animation_step_wraps_after_full_cycle(void) {
    TileAnimation anims[1] = { createAnimation(0, 2, 1) };
    TileAnimationManager manager(anims, 1, 10);
    
    manager.step(kOne60HzTickMs);
    TEST_ASSERT_EQUAL_UINT8(1, manager.resolveFrame(0));
    
    manager.step(kOne60HzTickMs);
    TEST_ASSERT_EQUAL_UINT8(0, manager.resolveFrame(0));
    
    manager.step(kOne60HzTickMs);
    TEST_ASSERT_EQUAL_UINT8(1, manager.resolveFrame(0));
}

void test_tile_animation_step_frame_duration_greater_than_one(void) {
    TileAnimation anims[1] = { createAnimation(10, 3, 2) };
    TileAnimationManager manager(anims, 1, 20);
    
    manager.step(kOne60HzTickMs);
    TEST_ASSERT_EQUAL_UINT8(10, manager.resolveFrame(10));
    
    manager.step(kOne60HzTickMs);
    TEST_ASSERT_EQUAL_UINT8(11, manager.resolveFrame(10));
    
    manager.step(kOne60HzTickMs);
    TEST_ASSERT_EQUAL_UINT8(11, manager.resolveFrame(10));
    
    manager.step(kOne60HzTickMs);
    TEST_ASSERT_EQUAL_UINT8(12, manager.resolveFrame(10));
}

void test_tile_animation_step_multiple_animations(void) {
    TileAnimation anims[2] = { 
        createAnimation(10, 2, 1),
        createAnimation(20, 3, 1)
    };
    TileAnimationManager manager(anims, 2, 30);
    
    manager.step(kOne60HzTickMs);
    
    TEST_ASSERT_EQUAL_UINT8(11, manager.resolveFrame(10));
    TEST_ASSERT_EQUAL_UINT8(21, manager.resolveFrame(20));
}

void test_tile_animation_step_empty_animations_array(void) {
    TileAnimation anims[0] = {};
    TileAnimationManager manager(anims, 0, 10);
    
    manager.step(kOne60HzTickMs);
    
    TEST_ASSERT_EQUAL_UINT8(0, manager.resolveFrame(0));
    TEST_ASSERT_EQUAL_UINT8(5, manager.resolveFrame(5));
}

void test_tile_animation_step_zero_delta_preserves_frame(void) {
    TileAnimation anims[1] = { createAnimation(10, 3, 1) };
    TileAnimationManager manager(anims, 1, 20);

    manager.step(kOne60HzTickMs);
    const uint8_t afterOne = manager.resolveFrame(10);

    manager.step(0);
    manager.step(0);

    TEST_ASSERT_EQUAL_UINT8(afterOne, manager.resolveFrame(10));
}

// =============================================================================
// Tests for resolveFrame() - Frame resolution
// =============================================================================

void test_tile_animation_resolve_non_animated_tile_returns_same(void) {
    TileAnimation anims[1] = { createAnimation(10, 2, 1) };
    TileAnimationManager manager(anims, 1, 20);
    
    TEST_ASSERT_EQUAL_UINT8(5, manager.resolveFrame(5));
    TEST_ASSERT_EQUAL_UINT8(15, manager.resolveFrame(15));
}

void test_tile_animation_resolve_animated_tile_returns_current_frame(void) {
    TileAnimation anims[1] = { createAnimation(10, 3, 1) };
    TileAnimationManager manager(anims, 1, 20);
    
    manager.step(kOne60HzTickMs);
    
    TEST_ASSERT_EQUAL_UINT8(11, manager.resolveFrame(10));
    TEST_ASSERT_EQUAL_UINT8(11, manager.resolveFrame(11));
    TEST_ASSERT_EQUAL_UINT8(11, manager.resolveFrame(12));
}

void test_tile_animation_resolve_tile_out_of_range(void) {
    TileAnimation anims[0] = {};
    TileAnimationManager manager(anims, 0, 10);
    
    TEST_ASSERT_EQUAL_UINT8(100, manager.resolveFrame(100));
    TEST_ASSERT_EQUAL_UINT8(200, manager.resolveFrame(200));
}

void test_tile_animation_resolve_tile_equals_tile_count(void) {
    TileAnimation anims[0] = {};
    TileAnimationManager manager(anims, 0, 10);
    
    TEST_ASSERT_EQUAL_UINT8(10, manager.resolveFrame(10));
}

// =============================================================================
// Tests for reset() - Reset functionality
// =============================================================================

void test_tile_animation_reset_returns_to_frame_zero(void) {
    TileAnimation anims[1] = { createAnimation(10, 3, 1) };
    TileAnimationManager manager(anims, 1, 20);
    
    manager.step(kOne60HzTickMs);
    manager.step(kOne60HzTickMs);
    TEST_ASSERT_EQUAL_UINT8(12, manager.resolveFrame(10));
    
    manager.reset();
    TEST_ASSERT_EQUAL_UINT8(10, manager.resolveFrame(10));
}

void test_tile_animation_get_visual_signature_stable_within_frame_duration(void) {
    TileAnimation anims[1] = { createAnimation(0, 4, 8) };
    TileAnimationManager manager(anims, 1, 20);

    const uint32_t s0 = manager.getVisualSignature();
    for (int i = 0; i < 7; ++i) {
        manager.step(kOne60HzTickMs);
        TEST_ASSERT_EQUAL_UINT32(s0, manager.getVisualSignature());
    }
    manager.step(kOne60HzTickMs);
    TEST_ASSERT_NOT_EQUAL(static_cast<int>(s0), static_cast<int>(manager.getVisualSignature()));
}

void test_tile_animation_reset_clears_global_frame_counter(void) {
    TileAnimation anims[1] = { createAnimation(10, 3, 1) };
    TileAnimationManager manager(anims, 1, 20);
    
    manager.step(kOne60HzTickMs);
    manager.step(kOne60HzTickMs);
    manager.step(kOne60HzTickMs);
    manager.step(kOne60HzTickMs);
    
    manager.reset();
    manager.step(kOne60HzTickMs);
    
    TEST_ASSERT_EQUAL_UINT8(11, manager.resolveFrame(10));
}

// =============================================================================
// Integration tests
// =============================================================================

void test_tile_animation_full_lifecycle(void) {
    TileAnimation anims[2] = {
        createAnimation(0, 4, 2),
        createAnimation(16, 2, 1)
    };
    TileAnimationManager manager(anims, 2, 32);
    
    TEST_ASSERT_EQUAL_UINT8(0, manager.resolveFrame(0));
    TEST_ASSERT_EQUAL_UINT8(16, manager.resolveFrame(16));
    
    manager.step(kOne60HzTickMs);
    TEST_ASSERT_EQUAL_UINT8(0, manager.resolveFrame(0));
    TEST_ASSERT_EQUAL_UINT8(17, manager.resolveFrame(16));
    
    manager.step(kOne60HzTickMs);
    TEST_ASSERT_EQUAL_UINT8(1, manager.resolveFrame(0));
    TEST_ASSERT_EQUAL_UINT8(16, manager.resolveFrame(16));
    
    manager.reset();
    TEST_ASSERT_EQUAL_UINT8(0, manager.resolveFrame(0));
    TEST_ASSERT_EQUAL_UINT8(16, manager.resolveFrame(16));
}

void test_tile_animation_appearance_unchanged_after_ctor_before_step_ticks(void) {
    TileAnimation anims[1] = { createAnimation(12, 2, 1) };
    TileAnimationManager manager(anims, 1, 24);
    TEST_ASSERT_FALSE(manager.animatedTileAppearanceChanged(12));
}

void test_tile_animation_appearance_changed_after_visual_step(void) {
    TileAnimation anims[1] = { createAnimation(12, 2, 1) };
    TileAnimationManager manager(anims, 1, 24);
    manager.step(kOne60HzTickMs);
    TEST_ASSERT_TRUE(manager.animatedTileAppearanceChanged(12));
}

// =============================================================================
// Unity test runner
// =============================================================================

void setUpSuite(void) {
}

void tearDownSuite(void) {
}

int main(void) {
    UNITY_BEGIN();
    
    RUN_TEST(test_tile_animation_constructor_initializes_lookup_to_identity);
    RUN_TEST(test_tile_animation_constructor_with_single_animation);
    RUN_TEST(test_tile_animation_constructor_validates_tile_count);
    
    RUN_TEST(test_tile_animation_step_simple_animation);
    RUN_TEST(test_tile_animation_step_multiple_frames);
    RUN_TEST(test_tile_animation_step_wraps_after_full_cycle);
    RUN_TEST(test_tile_animation_step_frame_duration_greater_than_one);
    RUN_TEST(test_tile_animation_get_visual_signature_stable_within_frame_duration);
    RUN_TEST(test_tile_animation_step_multiple_animations);
    RUN_TEST(test_tile_animation_step_empty_animations_array);
    RUN_TEST(test_tile_animation_step_zero_delta_preserves_frame);
    
    RUN_TEST(test_tile_animation_resolve_non_animated_tile_returns_same);
    RUN_TEST(test_tile_animation_resolve_animated_tile_returns_current_frame);
    RUN_TEST(test_tile_animation_resolve_tile_out_of_range);
    RUN_TEST(test_tile_animation_resolve_tile_equals_tile_count);
    
    RUN_TEST(test_tile_animation_reset_returns_to_frame_zero);
    RUN_TEST(test_tile_animation_reset_clears_global_frame_counter);
    
    RUN_TEST(test_tile_animation_full_lifecycle);

    RUN_TEST(test_tile_animation_appearance_unchanged_after_ctor_before_step_ticks);
    RUN_TEST(test_tile_animation_appearance_changed_after_visual_step);

    return UNITY_END();
}
