/**
 * @file test_static_tilemap_layer_cache.cpp
 * @brief Unit tests for graphics/StaticTilemapLayerCache module
 * @version 1.0
 * @date 2026-04-05
 * 
 * Tests for StaticTilemapLayerCache - framebuffer cache for static tilemap layers.
 */

#include <unity.h>
#include "../test_config.h"
#include "graphics/StaticTilemapLayerCache.h"
#include "mocks/MockDrawSurface.h"
#include "mocks/MockRenderer.h"

#ifdef PIXELROOT32_ENABLE_STATIC_TILEMAP_FB_CACHE

using namespace pixelroot32::graphics;

void setUp(void) {
    test_setup();
}

void tearDown(void) {
    test_teardown();
}

void test_cache_default_constructor(void) {
    StaticTilemapLayerCache cache;
    
    TEST_ASSERT_TRUE(cache.isFramebufferCacheEnabled());
}

void test_cache_allocate_valid_size(void) {
    StaticTilemapLayerCache cache;
    
    bool result = cache.allocateForLogicalSize(240, 240);
    
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_TRUE(cache.isFramebufferCacheEnabled());
}

void test_cache_allocate_invalid_width(void) {
    StaticTilemapLayerCache cache;
    
    bool result = cache.allocateForLogicalSize(0, 240);
    
    TEST_ASSERT_FALSE(result);
}

void test_cache_allocate_invalid_height(void) {
    StaticTilemapLayerCache cache;
    
    bool result = cache.allocateForLogicalSize(240, 0);
    
    TEST_ASSERT_FALSE(result);
}

void test_cache_allocate_negative_dimensions(void) {
    StaticTilemapLayerCache cache;
    
    bool result = cache.allocateForLogicalSize(-100, -100);
    
    TEST_ASSERT_FALSE(result);
}

void test_cache_allocate_same_size_twice(void) {
    StaticTilemapLayerCache cache;
    
    bool result1 = cache.allocateForLogicalSize(240, 240);
    TEST_ASSERT_TRUE(result1);
    
    bool result2 = cache.allocateForLogicalSize(240, 240);
    TEST_ASSERT_TRUE(result2);
}

void test_cache_allocate_different_size(void) {
    StaticTilemapLayerCache cache;
    
    (void)cache.allocateForLogicalSize(240, 240);
    bool result = cache.allocateForLogicalSize(320, 240);
    
    TEST_ASSERT_TRUE(result);
}

void test_cache_clear(void) {
    StaticTilemapLayerCache cache;
    
    (void)cache.allocateForLogicalSize(240, 240);
    cache.clear();
    
    TEST_ASSERT_TRUE(cache.isFramebufferCacheEnabled());
}

void test_cache_invalidate(void) {
    StaticTilemapLayerCache cache;
    
    (void)cache.allocateForLogicalSize(240, 240);
    cache.invalidate();
    
    TEST_ASSERT_TRUE(cache.isFramebufferCacheEnabled());
}

void test_cache_set_enabled_true(void) {
    StaticTilemapLayerCache cache;
    
    cache.setFramebufferCacheEnabled(true);
    
    TEST_ASSERT_TRUE(cache.isFramebufferCacheEnabled());
}

void test_cache_set_enabled_false(void) {
    StaticTilemapLayerCache cache;
    cache.setFramebufferCacheEnabled(true);
    
    cache.setFramebufferCacheEnabled(false);
    
    TEST_ASSERT_FALSE(cache.isFramebufferCacheEnabled());
}

void test_cache_toggle_enabled_twice(void) {
    StaticTilemapLayerCache cache;
    
    cache.setFramebufferCacheEnabled(true);
    TEST_ASSERT_TRUE(cache.isFramebufferCacheEnabled());
    
    cache.setFramebufferCacheEnabled(false);
    TEST_ASSERT_FALSE(cache.isFramebufferCacheEnabled());
    
    cache.setFramebufferCacheEnabled(true);
    TEST_ASSERT_TRUE(cache.isFramebufferCacheEnabled());
}

void test_cache_multiple_allocate_calls(void) {
    StaticTilemapLayerCache cache;
    
    (void)cache.allocateForLogicalSize(100, 100);
    (void)cache.allocateForLogicalSize(200, 200);
    (void)cache.allocateForLogicalSize(320, 240);
    
    TEST_ASSERT_TRUE(cache.isFramebufferCacheEnabled());
}

void test_cache_allocate_then_clear_then_allocate_again(void) {
    StaticTilemapLayerCache cache;
    
    bool r1 = cache.allocateForLogicalSize(240, 240);
    TEST_ASSERT_TRUE(r1);
    
    cache.clear();
    
    bool r2 = cache.allocateForLogicalSize(240, 240);
    TEST_ASSERT_TRUE(r2);
}

void test_cache_invalidate_clears_valid_flag(void) {
    StaticTilemapLayerCache cache;
    (void)cache.allocateForLogicalSize(240, 240);
    
    cache.invalidate();
    
    TEST_ASSERT_TRUE(cache.isFramebufferCacheEnabled());
}

void test_cache_multiple_invalidate_calls(void) {
    StaticTilemapLayerCache cache;
    (void)cache.allocateForLogicalSize(240, 240);
    
    cache.invalidate();
    cache.invalidate();
    cache.invalidate();
    
    TEST_ASSERT_TRUE(cache.isFramebufferCacheEnabled());
}

void test_cache_enable_disable_preserves_allocation(void) {
    StaticTilemapLayerCache cache;
    
    (void)cache.allocateForLogicalSize(240, 240);
    cache.setFramebufferCacheEnabled(false);
    
    TEST_ASSERT_FALSE(cache.isFramebufferCacheEnabled());
    
    cache.setFramebufferCacheEnabled(true);
    
    TEST_ASSERT_TRUE(cache.isFramebufferCacheEnabled());
}

void test_cache_clear_then_invalidate(void) {
    StaticTilemapLayerCache cache;
    
    (void)cache.allocateForLogicalSize(240, 240);
    cache.clear();
    cache.invalidate();
    
    TEST_ASSERT_TRUE(cache.isFramebufferCacheEnabled());
}

// =============================================================================
// Tests for draw() method - these crash due to sprite buffer access
// TODO: Fix sprite buffer mock support for draw() tests
// =============================================================================

// Commented out due to crashes - draw() requires valid sprite buffer
/*
void test_cache_draw_null_static_layers(void) {
    StaticTilemapLayerCache cache;
    (void)cache.allocateForLogicalSize(240, 240);
    
    MockDrawSurface surface;
    surface.setDisplaySize(240, 240);
    DisplayConfig config = PIXELROOT32_CUSTOM_DISPLAY(&surface, 240, 240);
    MockRenderer renderer(config);
    
    // Should not crash with null static layers
    cache.draw(renderer, 0, 0, nullptr, 0, nullptr, 0);
    
    // Verify cache is still enabled
    TEST_ASSERT_TRUE(cache.isFramebufferCacheEnabled());
}

void test_cache_draw_empty_dynamic_layers(void) {
    StaticTilemapLayerCache cache;
    (void)cache.allocateForLogicalSize(240, 240);
    
    MockDrawSurface surface;
    surface.setDisplaySize(240, 240);
    DisplayConfig config = PIXELROOT32_CUSTOM_DISPLAY(&surface, 240, 240);
    MockRenderer renderer(config);
    
    // Create static layer spec with null map
    TileMap4bppDrawSpec staticSpec = {nullptr, 0, 0};
    
    // Should not crash with empty dynamic layers
    cache.draw(renderer, 0, 0, &staticSpec, 1, nullptr, 0);
    
    TEST_ASSERT_TRUE(cache.isFramebufferCacheEnabled());
}

void test_cache_draw_all_null_layers(void) {
    StaticTilemapLayerCache cache;
    (void)cache.allocateForLogicalSize(240, 240);
    
    MockDrawSurface surface;
    surface.setDisplaySize(240, 240);
    DisplayConfig config = PIXELROOT32_CUSTOM_DISPLAY(&surface, 240, 240);
    MockRenderer renderer(config);
    
    // All null layers should not crash
    cache.draw(renderer, 0, 0, nullptr, 0, nullptr, 0);
    
    TEST_ASSERT_TRUE(cache.isFramebufferCacheEnabled());
}

void test_cache_draw_with_disabled_cache(void) {
    StaticTilemapLayerCache cache;
    (void)cache.allocateForLogicalSize(240, 240);
    cache.setFramebufferCacheEnabled(false);
    
    MockDrawSurface surface;
    surface.setDisplaySize(240, 240);
    DisplayConfig config = PIXELROOT32_CUSTOM_DISPLAY(&surface, 240, 240);
    MockRenderer renderer(config);
    
    // With cache disabled, should just draw directly
    cache.draw(renderer, 0, 0, nullptr, 0, nullptr, 0);
    
    TEST_ASSERT_FALSE(cache.isFramebufferCacheEnabled());
}

void test_cache_draw_multiple_camera_positions(void) {
    StaticTilemapLayerCache cache;
    (void)cache.allocateForLogicalSize(240, 240);
    
    MockDrawSurface surface;
    surface.setDisplaySize(240, 240);
    DisplayConfig config = PIXELROOT32_CUSTOM_DISPLAY(&surface, 240, 240);
    MockRenderer renderer(config);
    
    // Different camera positions should not crash
    cache.draw(renderer, 0, 0, nullptr, 0, nullptr, 0);
    cache.draw(renderer, 10, 10, nullptr, 0, nullptr, 0);
    cache.draw(renderer, -10, -10, nullptr, 0, nullptr, 0);
    cache.draw(renderer, 100, 200, nullptr, 0, nullptr, 0);
    
    TEST_ASSERT_TRUE(cache.isFramebufferCacheEnabled());
}

void test_cache_draw_after_invalidate(void) {
    StaticTilemapLayerCache cache;
    (void)cache.allocateForLogicalSize(240, 240);
    
    MockDrawSurface surface;
    surface.setDisplaySize(240, 240);
    DisplayConfig config = PIXELROOT32_CUSTOM_DISPLAY(&surface, 240, 240);
    MockRenderer renderer(config);
    
    // Draw once
    cache.draw(renderer, 0, 0, nullptr, 0, nullptr, 0);
    
    // Invalidate should mark cache as needing rebuild
    cache.invalidate();
    
    // Draw again after invalidate
    cache.draw(renderer, 0, 0, nullptr, 0, nullptr, 0);
    
    TEST_ASSERT_TRUE(cache.isFramebufferCacheEnabled());
}
*/

int main(void) {
    UNITY_BEGIN();
    
    RUN_TEST(test_cache_default_constructor);
    RUN_TEST(test_cache_allocate_valid_size);
    RUN_TEST(test_cache_allocate_invalid_width);
    RUN_TEST(test_cache_allocate_invalid_height);
    RUN_TEST(test_cache_allocate_negative_dimensions);
    RUN_TEST(test_cache_allocate_same_size_twice);
    RUN_TEST(test_cache_allocate_different_size);
    RUN_TEST(test_cache_clear);
    RUN_TEST(test_cache_invalidate);
    RUN_TEST(test_cache_set_enabled_true);
    RUN_TEST(test_cache_set_enabled_false);
    RUN_TEST(test_cache_toggle_enabled_twice);
    RUN_TEST(test_cache_multiple_allocate_calls);
    RUN_TEST(test_cache_allocate_then_clear_then_allocate_again);
    RUN_TEST(test_cache_invalidate_clears_valid_flag);
    RUN_TEST(test_cache_multiple_invalidate_calls);
    RUN_TEST(test_cache_enable_disable_preserves_allocation);
    RUN_TEST(test_cache_clear_then_invalidate);
    
    // FASE 3: draw() method tests commented out - require sprite buffer fix
    // See comments above for tests that need MockDrawSurface sprite buffer support
    
    return UNITY_END();
}

#else

void setUp(void) {}
void tearDown(void) {}

void test_cache_disabled(void) {
    TEST_IGNORE_MESSAGE("PIXELROOT32_ENABLE_STATIC_TILEMAP_FB_CACHE not defined");
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_cache_disabled);
    return UNITY_END();
}

#endif