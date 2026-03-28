#include <unity.h>
#include "../../test_config.h"
#include "core/Scene.h"
#include "physics/TileCollisionBuilder.h"
#include "physics/TileAttributes.h"

using namespace pixelroot32::core;
using namespace pixelroot32::physics;

void test_tile_collision_builder_config_default() {
    TileCollisionBuilderConfig config;
    
    TEST_ASSERT_EQUAL(16U, config.tileWidth);
    TEST_ASSERT_EQUAL(16U, config.tileHeight);
}

void test_tile_collision_builder_config_custom() {
    TileCollisionBuilderConfig config(8, 8);
    
    TEST_ASSERT_EQUAL(8U, config.tileWidth);
    TEST_ASSERT_EQUAL(8U, config.tileHeight);
}

void test_tile_behavior_layer_empty() {
    TileBehaviorLayer layer;
    layer.data = nullptr;
    layer.width = 0;
    layer.height = 0;
    
    TEST_ASSERT_NULL(layer.data);
    TEST_ASSERT_EQUAL(0U, layer.width);
    TEST_ASSERT_EQUAL(0U, layer.height);
}

void test_tile_behavior_layer_with_data() {
    uint8_t data[4] = {1, 2, 3, 4};
    TileBehaviorLayer layer;
    layer.data = data;
    layer.width = 2;
    layer.height = 2;
    
    TEST_ASSERT_NOT_NULL(layer.data);
    TEST_ASSERT_EQUAL(2U, layer.width);
    TEST_ASSERT_EQUAL(2U, layer.height);
}

void test_tile_collision_builder_creation() {
    Scene scene;
    TileCollisionBuilderConfig config;
    TileCollisionBuilder builder(scene, config);
    
    TEST_ASSERT_TRUE(true);
}

void test_tile_collision_builder_build_empty_layer() {
    Scene scene;
    TileCollisionBuilderConfig config(16, 16);
    config.maxEntities = 100;
    TileCollisionBuilder builder(scene, config);
    
    TileBehaviorLayer layer;
    layer.data = nullptr;
    layer.width = 0;
    layer.height = 0;
    
    int result = builder.buildFromBehaviorLayer(layer, 0);
    TEST_ASSERT_EQUAL(0, result);
}

void test_tile_collision_builder_build_with_max_entities_zero() {
    Scene scene;
    TileCollisionBuilderConfig config(16, 16);
    config.maxEntities = 0;
    TileCollisionBuilder builder(scene, config);
    
    uint8_t data[4] = {1, 1, 1, 1};
    TileBehaviorLayer layer;
    layer.data = data;
    layer.width = 2;
    layer.height = 2;
    
    int result = builder.buildFromBehaviorLayer(layer, 0);
    TEST_ASSERT_EQUAL(-1, result);
}

void test_tile_flags_enum() {
    TEST_ASSERT_EQUAL(1U, static_cast<uint8_t>(TILE_SOLID));
    TEST_ASSERT_EQUAL(2U, static_cast<uint8_t>(TILE_SENSOR));
    TEST_ASSERT_EQUAL(4U, static_cast<uint8_t>(TILE_DAMAGE));
    TEST_ASSERT_EQUAL(8U, static_cast<uint8_t>(TILE_COLLECTIBLE));
    TEST_ASSERT_EQUAL(16U, static_cast<uint8_t>(TILE_ONEWAY));
    TEST_ASSERT_EQUAL(32U, static_cast<uint8_t>(TILE_TRIGGER));
}

void test_pack_tile_data() {
    void* data = reinterpret_cast<void*>(packTileData(10, 20, TILE_SOLID));
    TEST_ASSERT_NOT_NULL(data);
}

void test_unpack_tile_data() {
    uintptr_t packed = packTileData(5, 10, TILE_SENSOR);
    uint16_t x, y;
    TileFlags flags;
    unpackTileData(packed, x, y, flags);
    
    TEST_ASSERT_EQUAL(5U, x);
    TEST_ASSERT_EQUAL(10U, y);
    TEST_ASSERT_EQUAL(TILE_SENSOR, flags);
}

void test_is_sensor_tile() {
    TEST_ASSERT_TRUE(isSensorTile(TILE_SENSOR));
    TEST_ASSERT_TRUE(isSensorTile(TILE_DAMAGE));
    TEST_ASSERT_TRUE(isSensorTile(TILE_COLLECTIBLE));
    TEST_ASSERT_TRUE(isSensorTile(TILE_TRIGGER));
    TEST_ASSERT_FALSE(isSensorTile(TILE_SOLID));
    TEST_ASSERT_FALSE(isSensorTile(TILE_ONEWAY));
    TEST_ASSERT_FALSE(isSensorTile(TILE_NONE));
}

void test_is_one_way_tile() {
    TEST_ASSERT_TRUE(isOneWayTile(TILE_ONEWAY));
    TEST_ASSERT_FALSE(isOneWayTile(TILE_SOLID));
    TEST_ASSERT_FALSE(isOneWayTile(TILE_SENSOR));
}

void setUp(void) {
    test_setup();
}

void tearDown(void) {
    test_teardown();
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_tile_collision_builder_config_default);
    RUN_TEST(test_tile_collision_builder_config_custom);
    RUN_TEST(test_tile_behavior_layer_empty);
    RUN_TEST(test_tile_behavior_layer_with_data);
    RUN_TEST(test_tile_collision_builder_creation);
    RUN_TEST(test_tile_collision_builder_build_empty_layer);
    RUN_TEST(test_tile_collision_builder_build_with_max_entities_zero);
    RUN_TEST(test_tile_flags_enum);
    RUN_TEST(test_pack_tile_data);
    RUN_TEST(test_unpack_tile_data);
    RUN_TEST(test_is_sensor_tile);
    RUN_TEST(test_is_one_way_tile);
    return UNITY_END();
}
