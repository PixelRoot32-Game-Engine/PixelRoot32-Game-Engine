/**
 * @file test_tile_consumption_helper.cpp
 * @brief Unit tests for physics/TileConsumptionHelper module
 * @version 1.0
 * @date 2026-03-29
 * 
 * Tests for TileConsumptionHelper - tile collection/removal helper.
 * Uses nullptr tilemap to test edge cases without complex tilemap mocking.
 */

#include <unity.h>
#include "../../test_config.h"
#include "physics/TileConsumptionHelper.h"
#include "core/Scene.h"
#include "core/Entity.h"
#include "graphics/Renderer.h"

#ifdef PIXELROOT32_ENABLE_PHYSICS

using namespace pixelroot32::core;
using namespace pixelroot32::physics;
using namespace pixelroot32::graphics;

// =============================================================================
// Mock Scene for testing - minimal implementation
// =============================================================================

class MockScene : public Scene {
public:
    MockScene() {}
    virtual ~MockScene() {}
    
    // Override required virtual methods
    virtual void init() override {}
    virtual void update(unsigned long deltaTime) override {
        (void)deltaTime;
    }
    virtual void draw(Renderer& renderer) override {
        (void)renderer;
    }
    
    // Track entity removal
    int removedCount = 0;
};

// =============================================================================
// Test fixtures
// =============================================================================

TileConsumptionConfig createDefaultConfig() {
    TileConsumptionConfig cfg;
    cfg.validateCoordinates = true;
    cfg.updateTilemap = true;
    cfg.logConsumption = false;  // Disable logging for tests
    return cfg;
}

TileConsumptionConfig createNoOpConfig() {
    TileConsumptionConfig cfg;
    cfg.validateCoordinates = false;
    cfg.updateTilemap = false;
    cfg.logConsumption = false;
    return cfg;
}

// =============================================================================
// Constructor tests
// =============================================================================

void test_tile_consumption_constructor_with_nullptr_tilemap(void) {
    MockScene scene;
    
    // Should not crash with nullptr tilemap
    TileConsumptionHelper helper(scene, nullptr, createDefaultConfig());
    
    TEST_ASSERT_TRUE(true);
}

void test_tile_consumption_constructor_with_custom_config(void) {
    MockScene scene;
    TileConsumptionConfig cfg = createNoOpConfig();
    
    TileConsumptionHelper helper(scene, nullptr, cfg);
    
    TEST_ASSERT_TRUE(true);
}

void test_tile_consumption_constructor_default_config(void) {
    MockScene scene;
    
    // Default config should work
    TileConsumptionHelper helper(scene, nullptr, TileConsumptionConfig());
    
    TEST_ASSERT_TRUE(true);
}

// =============================================================================
// consumeTile tests - with nullptr tilemap, tests validation logic only
// =============================================================================

void test_consume_tile_with_nullptr_actor_and_tilemap(void) {
    MockScene scene;
    TileConsumptionHelper helper(scene, nullptr, createNoOpConfig());
    
    // Should not crash - no tilemap to update, no actor to remove
    bool result = helper.consumeTile(nullptr, 5, 5);
    
    TEST_ASSERT_TRUE(result);  // Should succeed since no tilemap validation
}

void test_consume_tile_with_invalid_coordinates(void) {
    MockScene scene;
    TileConsumptionConfig cfg = createDefaultConfig();
    cfg.validateCoordinates = true;
    
    // Note: Without tilemap, validateCoordinates will use default dimensions (0,0)
    // which means coordinates are considered valid
    TileConsumptionHelper helper(scene, nullptr, cfg);
    
    // This should succeed because tilemap dimensions are 0,0 so all coords are valid
    bool result = helper.consumeTile(nullptr, 100, 100);
    
    TEST_ASSERT_TRUE(result);
}

void test_consume_tile_with_zero_coordinates(void) {
    MockScene scene;
    TileConsumptionHelper helper(scene, nullptr, createNoOpConfig());
    
    // Edge case: zero coordinates
    bool result = helper.consumeTile(nullptr, 0, 0);
    
    TEST_ASSERT_TRUE(result);
}

void test_consume_tile_with_max_coordinates(void) {
    MockScene scene;
    TileConsumptionHelper helper(scene, nullptr, createNoOpConfig());
    
    // Edge case: max uint16_t values
    bool result = helper.consumeTile(nullptr, 65535, 65535);
    
    TEST_ASSERT_TRUE(result);
}

// =============================================================================
// consumeTileFromUserData tests
// =============================================================================

void test_consume_tile_from_userdata_zero(void) {
    MockScene scene;
    TileConsumptionHelper helper(scene, nullptr, createDefaultConfig());
    
    // Edge case: packedUserData = 0 should fail
    bool result = helper.consumeTileFromUserData(nullptr, 0);
    
    TEST_ASSERT_FALSE(result);
}

void test_consume_tile_from_userdata_non_collectible(void) {
    MockScene scene;
    TileConsumptionHelper helper(scene, nullptr, createDefaultConfig());
    
    // Pack tile data with NO TILE_COLLECTIBLE flag
    // Format: x (9 bits) | y (9 bits) | flags (upper bits)
    uint16_t x = 5;
    uint16_t y = 10;
    uint16_t flags = 0;  // No collectible flag
    
    uintptr_t packed = (static_cast<uintptr_t>(flags) << 18) | 
                       (static_cast<uintptr_t>(y) << 9) | 
                       static_cast<uintptr_t>(x);
    
    bool result = helper.consumeTileFromUserData(nullptr, packed);
    
    // Should fail because tile is not collectible
    TEST_ASSERT_FALSE(result);
}

// =============================================================================
// isTileConsumed tests
// =============================================================================

void test_is_tile_consumed_with_nullptr_tilemap(void) {
    MockScene scene;
    TileConsumptionHelper helper(scene, nullptr, createDefaultConfig());
    
    // Without tilemap, should return false (nothing consumed)
    bool result = helper.isTileConsumed(5, 5);
    
    TEST_ASSERT_FALSE(result);
}

void test_is_tile_consumed_with_max_coordinates(void) {
    MockScene scene;
    TileConsumptionHelper helper(scene, nullptr, createNoOpConfig());
    
    // Edge case: max coordinates
    bool result = helper.isTileConsumed(65535, 65535);
    
    TEST_ASSERT_FALSE(result);
}

// =============================================================================
// restoreTile tests
// =============================================================================

void test_restore_tile_with_nullptr_tilemap(void) {
    MockScene scene;
    TileConsumptionHelper helper(scene, nullptr, createDefaultConfig());
    
    // Without tilemap, restore should fail
    bool result = helper.restoreTile(5, 5);
    
    TEST_ASSERT_FALSE(result);
}

void test_restore_tile_with_invalid_coordinates(void) {
    MockScene scene;
    TileConsumptionConfig cfg = createDefaultConfig();
    cfg.validateCoordinates = true;
    TileConsumptionHelper helper(scene, nullptr, cfg);
    
    // With validateCoordinates, out of bounds should fail
    // (but without tilemap dims, it might succeed - just test it doesn't crash)
    bool result = helper.restoreTile(1000, 1000);
    
    TEST_ASSERT_FALSE(result);
}

// =============================================================================
// Edge case tests - full workflow simulation
// =============================================================================

void test_multiple_consume_same_tile(void) {
    MockScene scene;
    TileConsumptionHelper helper(scene, nullptr, createNoOpConfig());
    
    // Consume same tile multiple times
    bool result1 = helper.consumeTile(nullptr, 5, 5);
    bool result2 = helper.consumeTile(nullptr, 5, 5);
    bool result3 = helper.consumeTile(nullptr, 5, 5);
    
    // First should succeed, subsequent should also succeed (no tilemap to check)
    TEST_ASSERT_TRUE(result1);
    TEST_ASSERT_TRUE(result2);
    TEST_ASSERT_TRUE(result3);
}

void test_consume_different_tiles(void) {
    MockScene scene;
    TileConsumptionHelper helper(scene, nullptr, createNoOpConfig());
    
    // Consume different tiles
    bool r1 = helper.consumeTile(nullptr, 1, 1);
    bool r2 = helper.consumeTile(nullptr, 2, 2);
    bool r3 = helper.consumeTile(nullptr, 3, 3);
    bool r4 = helper.consumeTile(nullptr, 10, 10);
    bool r5 = helper.consumeTile(nullptr, 0, 0);
    
    TEST_ASSERT_TRUE(r1);
    TEST_ASSERT_TRUE(r2);
    TEST_ASSERT_TRUE(r3);
    TEST_ASSERT_TRUE(r4);
    TEST_ASSERT_TRUE(r5);
}

void test_interleave_consume_and_restore(void) {
    MockScene scene;
    TileConsumptionHelper helper(scene, nullptr, createDefaultConfig());
    
    // Without tilemap, restore always returns false
    // Just verify no crashes
    helper.consumeTile(nullptr, 5, 5);
    helper.restoreTile(5, 5);
    helper.consumeTile(nullptr, 5, 5);
    helper.restoreTile(5, 5);
    
    TEST_ASSERT_TRUE(true);
}

void test_consume_after_restore(void) {
    MockScene scene;
    TileConsumptionHelper helper(scene, nullptr, createNoOpConfig());
    
    // Consume, restore (fails without tilemap), consume again
    bool r1 = helper.consumeTile(nullptr, 5, 5);
    bool rest = helper.restoreTile(5, 5);  // Should fail
    bool r2 = helper.consumeTile(nullptr, 5, 5);
    
    TEST_ASSERT_TRUE(r1);
    TEST_ASSERT_FALSE(rest);  // Should fail without tilemap
    TEST_ASSERT_TRUE(r2);
}

// =============================================================================
// Additional coverage tests - consumeTile with actor, logging, etc.
// =============================================================================

void test_consume_tile_with_logging_enabled(void) {
    MockScene scene;
    TileConsumptionConfig cfg = createDefaultConfig();
    cfg.logConsumption = true;  // Enable logging
    
    TileConsumptionHelper helper(scene, nullptr, cfg);
    
    // Should not crash with logging enabled
    bool result = helper.consumeTile(nullptr, 5, 5);
    TEST_ASSERT_TRUE(result);
}

void test_consume_tile_with_updateTilemap_enabled(void) {
    MockScene scene;
    TileConsumptionConfig cfg = createDefaultConfig();
    cfg.updateTilemap = true;
    
    TileConsumptionHelper helper(scene, nullptr, cfg);
    
    // With updateTilemap=true but tilemap=nullptr, should still work
    bool result = helper.consumeTile(nullptr, 5, 5);
    TEST_ASSERT_TRUE(result);
}

void test_multiple_tiles_consumption(void) {
    MockScene scene;
    TileConsumptionHelper helper(scene, nullptr, createNoOpConfig());
    
    // Consume multiple different tiles
    bool r1 = helper.consumeTile(nullptr, 1, 1);
    bool r2 = helper.consumeTile(nullptr, 2, 2);
    bool r3 = helper.consumeTile(nullptr, 3, 3);
    bool r4 = helper.consumeTile(nullptr, 10, 10);
    bool r5 = helper.consumeTile(nullptr, 0, 0);
    
    TEST_ASSERT_TRUE(r1);
    TEST_ASSERT_TRUE(r2);
    TEST_ASSERT_TRUE(r3);
    TEST_ASSERT_TRUE(r4);
    TEST_ASSERT_TRUE(r5);
}

void test_is_tile_consumed_sequence(void) {
    MockScene scene;
    TileConsumptionHelper helper(scene, nullptr, createNoOpConfig());
    
    // Initially not consumed
    TEST_ASSERT_FALSE(helper.isTileConsumed(5, 5));
    
    // After consume, should still return false (no tilemap to track)
    helper.consumeTile(nullptr, 5, 5);
    TEST_ASSERT_FALSE(helper.isTileConsumed(5, 5));
    
    // After restore attempt, should still be false
    helper.restoreTile(5, 5);
    TEST_ASSERT_FALSE(helper.isTileConsumed(5, 5));
}

// =============================================================================
// Unity test runner
// =============================================================================

void setUp(void) {
    test_setup();
}

void tearDown(void) {
    test_teardown();
}

void setUpSuite(void) {
}

void tearDownSuite(void) {
}

int main(void) {
    UNITY_BEGIN();
    
    // Constructor tests
    RUN_TEST(test_tile_consumption_constructor_with_nullptr_tilemap);
    RUN_TEST(test_tile_consumption_constructor_with_custom_config);
    RUN_TEST(test_tile_consumption_constructor_default_config);
    
    // consumeTile tests
    RUN_TEST(test_consume_tile_with_nullptr_actor_and_tilemap);
    RUN_TEST(test_consume_tile_with_invalid_coordinates);
    RUN_TEST(test_consume_tile_with_zero_coordinates);
    RUN_TEST(test_consume_tile_with_max_coordinates);
    
    // consumeTileFromUserData tests
    RUN_TEST(test_consume_tile_from_userdata_zero);
    RUN_TEST(test_consume_tile_from_userdata_non_collectible);
    
    // isTileConsumed tests
    RUN_TEST(test_is_tile_consumed_with_nullptr_tilemap);
    RUN_TEST(test_is_tile_consumed_with_max_coordinates);
    
    // restoreTile tests
    RUN_TEST(test_restore_tile_with_nullptr_tilemap);
    RUN_TEST(test_restore_tile_with_invalid_coordinates);
    
    // Edge case tests
    RUN_TEST(test_multiple_consume_same_tile);
    RUN_TEST(test_consume_different_tiles);
    RUN_TEST(test_interleave_consume_and_restore);
    RUN_TEST(test_consume_after_restore);
    
    // Additional coverage tests
    RUN_TEST(test_consume_tile_with_logging_enabled);
    RUN_TEST(test_consume_tile_with_updateTilemap_enabled);
    RUN_TEST(test_multiple_tiles_consumption);
    RUN_TEST(test_is_tile_consumed_sequence);
    
    return UNITY_END();
}

#else

void setUp(void) {}
void tearDown(void) {}

void test_tile_consumption_disabled(void) {
    TEST_IGNORE_MESSAGE("PIXELROOT32_ENABLE_PHYSICS not defined");
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_tile_consumption_disabled);
    return UNITY_END();
}

#endif
