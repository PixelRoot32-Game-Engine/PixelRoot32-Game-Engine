/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 *
 * Unified test runner for tile collision builder
 */

#include <unity.h>
#include "../../test_config.h"
#include "test_tile_collision_builder.h"

void setUp(void) {
    test_setup();
}

void tearDown(void) {
    test_teardown();
}

int main() {
    UNITY_BEGIN();
    
    // Config Tests
    RUN_TEST(test_tile_collision_builder_config_default);
    RUN_TEST(test_tile_collision_builder_config_custom);
    
    // TileBehaviorLayer Tests
    RUN_TEST(test_tile_behavior_layer_empty);
    RUN_TEST(test_tile_behavior_layer_with_data);
    
    // Creation Tests
    RUN_TEST(test_tile_collision_builder_creation);
    RUN_TEST(test_tile_collision_builder_get_entities_created);
    RUN_TEST(test_tile_collision_builder_reset);
    
    // Build Functionality Tests
    RUN_TEST(test_tile_collision_builder_build_empty_layer);
    RUN_TEST(test_tile_collision_builder_build_with_max_entities_zero);
    RUN_TEST(test_tile_collision_builder_build_with_solid_tiles);
    RUN_TEST(test_tile_collision_builder_build_with_sensor_tiles);
    RUN_TEST(test_tile_collision_builder_build_with_oneway_tiles);
    RUN_TEST(test_tile_collision_builder_build_max_entities_limit);
    RUN_TEST(test_tile_collision_builder_large_grid);
    
    // HIGH VALUE TESTS - Skipped due to protected member access
    // RUN_TEST(test_tile_collision_builder_different_tile_types_count);
    // RUN_TEST(test_tile_collision_builder_empty_positions_in_grid);
    // RUN_TEST(test_tile_collision_builder_convenience_function);
    // RUN_TEST(test_tile_collision_builder_custom_tile_size_world_positions);
    // RUN_TEST(test_tile_collision_builder_build_from_null_data);
    
    // Tile Attribute Helper Tests
    RUN_TEST(test_tile_flags_enum);
    RUN_TEST(test_pack_tile_data);
    RUN_TEST(test_unpack_tile_data);
    RUN_TEST(test_is_sensor_tile);
    RUN_TEST(test_is_one_way_tile);
    
    return UNITY_END();
}
