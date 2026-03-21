/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */

#include <unity.h>
#include "../../test_config.h"
#include "graphics/Renderer.h"
#include <memory>

using namespace pixelroot32::graphics;

void setUp(void) {
    test_setup();
}

void tearDown(void) {
    test_teardown();
}

void test_tile_mask_initialization_all_active() {
    // Create a simple 16x16 tilemap for testing
    uint8_t indices[256] = {0};
    Sprite tiles[1] = {{nullptr, 8, 8}}; // Dummy sprite
    
    TileMap tileMap;
    tileMap.indices = indices;
    tileMap.width = 16;
    tileMap.height = 16;
    tileMap.tiles = tiles;
    tileMap.tileWidth = 8;
    tileMap.tileHeight = 8;
    tileMap.tileCount = 1;
    tileMap.runtimeMask = nullptr;
    
    // Initialize runtime mask
    tileMap.initRuntimeMask();
    
    // Verify that runtimeMask is allocated
    TEST_ASSERT_NOT_NULL(tileMap.getRuntimeMask());
    
    // Verify that all tiles are active by default
    for (int y = 0; y < 16; y++) {
        for (int x = 0; x < 16; x++) {
            TEST_ASSERT_TRUE_MESSAGE(tileMap.isTileActive(x, y), 
                "All tiles should be active after initialization");
        }
    }
    
    // Cleanup
    tileMap.cleanupRuntimeMask();
}

void test_tile_mask_set_single_inactive() {
    uint8_t indices[256] = {0};
    Sprite tiles[1] = {{nullptr, 8, 8}};
    
    TileMap tileMap;
    tileMap.indices = indices;
    tileMap.width = 16;
    tileMap.height = 16;
    tileMap.tiles = tiles;
    tileMap.tileWidth = 8;
    tileMap.tileHeight = 8;
    tileMap.tileCount = 1;
    tileMap.runtimeMask = nullptr;
    
    tileMap.initRuntimeMask();
    
    // Set tile (5, 7) as inactive
    tileMap.setTileActive(5, 7, false);
    
    // Verify that only tile (5, 7) is inactive
    TEST_ASSERT_FALSE_MESSAGE(tileMap.isTileActive(5, 7), 
        "Tile (5, 7) should be inactive");
    
    // Verify that adjacent tiles are still active
    TEST_ASSERT_TRUE_MESSAGE(tileMap.isTileActive(4, 7), 
        "Tile (4, 7) should remain active");
    TEST_ASSERT_TRUE_MESSAGE(tileMap.isTileActive(6, 7), 
        "Tile (6, 7) should remain active");
    TEST_ASSERT_TRUE_MESSAGE(tileMap.isTileActive(5, 6), 
        "Tile (5, 6) should remain active");
    TEST_ASSERT_TRUE_MESSAGE(tileMap.isTileActive(5, 8), 
        "Tile (5, 8) should remain active");
    
    tileMap.cleanupRuntimeMask();
}

void test_tile_mask_reactivate_tile() {
    uint8_t indices[256] = {0};
    Sprite tiles[1] = {{nullptr, 8, 8}};
    
    TileMap tileMap;
    tileMap.indices = indices;
    tileMap.width = 16;
    tileMap.height = 16;
    tileMap.tiles = tiles;
    tileMap.tileWidth = 8;
    tileMap.tileHeight = 8;
    tileMap.tileCount = 1;
    tileMap.runtimeMask = nullptr;
    
    tileMap.initRuntimeMask();
    
    // Set tile (3, 3) as inactive, then reactivate it
    tileMap.setTileActive(3, 3, false);
    TEST_ASSERT_FALSE(tileMap.isTileActive(3, 3));
    
    tileMap.setTileActive(3, 3, true);
    TEST_ASSERT_TRUE_MESSAGE(tileMap.isTileActive(3, 3), 
        "Tile (3, 3) should be active after reactivation");
    
    tileMap.cleanupRuntimeMask();
}

void test_tile_mask_boundary_conditions() {
    uint8_t indices[256] = {0};
    Sprite tiles[1] = {{nullptr, 8, 8}};
    
    TileMap tileMap;
    tileMap.indices = indices;
    tileMap.width = 16;
    tileMap.height = 16;
    tileMap.tiles = tiles;
    tileMap.tileWidth = 8;
    tileMap.tileHeight = 8;
    tileMap.tileCount = 1;
    tileMap.runtimeMask = nullptr;
    
    tileMap.initRuntimeMask();
    
    // Test corner tiles
    tileMap.setTileActive(0, 0, false);
    tileMap.setTileActive(15, 15, false);
    
    TEST_ASSERT_FALSE_MESSAGE(tileMap.isTileActive(0, 0), 
        "Tile (0, 0) should be inactive");
    TEST_ASSERT_FALSE_MESSAGE(tileMap.isTileActive(15, 15), 
        "Tile (15, 15) should be inactive");
    
    // Test out-of-bounds coordinates (should return true - active by default)
    TEST_ASSERT_TRUE_MESSAGE(tileMap.isTileActive(-1, 0), 
        "Out-of-bounds should return active");
    TEST_ASSERT_TRUE_MESSAGE(tileMap.isTileActive(0, -1), 
        "Out-of-bounds should return active");
    TEST_ASSERT_TRUE_MESSAGE(tileMap.isTileActive(16, 0), 
        "Out-of-bounds should return active");
    TEST_ASSERT_TRUE_MESSAGE(tileMap.isTileActive(0, 16), 
        "Out-of-bounds should return active");
    
    tileMap.cleanupRuntimeMask();
}

void test_tile_mask_without_mask_all_active() {
    uint8_t indices[256] = {0};
    Sprite tiles[1] = {{nullptr, 8, 8}};
    
    TileMap tileMap;
    tileMap.indices = indices;
    tileMap.width = 16;
    tileMap.height = 16;
    tileMap.tiles = tiles;
    tileMap.tileWidth = 8;
    tileMap.tileHeight = 8;
    tileMap.tileCount = 1;
    tileMap.runtimeMask = nullptr; // No mask initialized
    
    // Without mask, all tiles should report as active
    TEST_ASSERT_TRUE_MESSAGE(tileMap.isTileActive(0, 0), 
        "Without mask, all tiles should be active");
    TEST_ASSERT_TRUE_MESSAGE(tileMap.isTileActive(7, 7), 
        "Without mask, all tiles should be active");
    TEST_ASSERT_TRUE_MESSAGE(tileMap.isTileActive(15, 15), 
        "Without mask, all tiles should be active");
    
    // setTileActive should be ignored without mask
    tileMap.setTileActive(5, 5, false);
    TEST_ASSERT_TRUE_MESSAGE(tileMap.isTileActive(5, 5), 
        "setTileActive should be ignored without mask");
}

void test_tile_mask_memory_efficiency() {
    // Test different tilemap sizes to verify correct memory allocation
    uint8_t indices1[256] = {0};
    Sprite tiles[1] = {{nullptr, 8, 8}};
    
    // 16x16 tilemap = 256 tiles = 32 bytes
    TileMap tileMap16;
    tileMap16.indices = indices1;
    tileMap16.width = 16;
    tileMap16.height = 16;
    tileMap16.tiles = tiles;
    tileMap16.tileWidth = 8;
    tileMap16.tileHeight = 8;
    tileMap16.tileCount = 1;
    tileMap16.runtimeMask = nullptr;
    
    tileMap16.initRuntimeMask();
    TEST_ASSERT_NOT_NULL(tileMap16.getRuntimeMask());
    tileMap16.cleanupRuntimeMask();
    
    // 32x32 tilemap = 1024 tiles = 128 bytes
    uint8_t indices2[1024] = {0};
    TileMap tileMap32;
    tileMap32.indices = indices2;
    tileMap32.width = 32;
    tileMap32.height = 32;
    tileMap32.tiles = tiles;
    tileMap32.tileWidth = 8;
    tileMap32.tileHeight = 8;
    tileMap32.tileCount = 1;
    tileMap32.runtimeMask = nullptr;
    
    tileMap32.initRuntimeMask();
    TEST_ASSERT_NOT_NULL(tileMap32.getRuntimeMask());
    tileMap32.cleanupRuntimeMask();
}

void test_tile_mask_multiple_operations() {
    uint8_t indices[256] = {0};
    Sprite tiles[1] = {{nullptr, 8, 8}};
    
    TileMap tileMap;
    tileMap.indices = indices;
    tileMap.width = 16;
    tileMap.height = 16;
    tileMap.tiles = tiles;
    tileMap.tileWidth = 8;
    tileMap.tileHeight = 8;
    tileMap.tileCount = 1;
    tileMap.runtimeMask = nullptr;
    
    tileMap.initRuntimeMask();
    
    // Set multiple tiles inactive
    tileMap.setTileActive(1, 1, false);
    tileMap.setTileActive(2, 2, false);
    tileMap.setTileActive(3, 3, false);
    
    TEST_ASSERT_FALSE(tileMap.isTileActive(1, 1));
    TEST_ASSERT_FALSE(tileMap.isTileActive(2, 2));
    TEST_ASSERT_FALSE(tileMap.isTileActive(3, 3));
    
    // Reactivate one of them
    tileMap.setTileActive(2, 2, true);
    TEST_ASSERT_TRUE(tileMap.isTileActive(2, 2));
    TEST_ASSERT_FALSE(tileMap.isTileActive(1, 1));
    TEST_ASSERT_FALSE(tileMap.isTileActive(3, 3));
    
    tileMap.cleanupRuntimeMask();
}

void test_tile_mask_reinitialization() {
    uint8_t indices[256] = {0};
    Sprite tiles[1] = {{nullptr, 8, 8}};
    
    TileMap tileMap;
    tileMap.indices = indices;
    tileMap.width = 16;
    tileMap.height = 16;
    tileMap.tiles = tiles;
    tileMap.tileWidth = 8;
    tileMap.tileHeight = 8;
    tileMap.tileCount = 1;
    tileMap.runtimeMask = nullptr;
    
    // Initialize first time
    tileMap.initRuntimeMask();
    tileMap.setTileActive(5, 5, false);
    tileMap.setTileActive(3, 3, false);
    TEST_ASSERT_FALSE(tileMap.isTileActive(5, 5));
    TEST_ASSERT_FALSE(tileMap.isTileActive(3, 3));
    
    // Reinitialize (should reset all tiles to active)
    tileMap.initRuntimeMask();
    
    // All tiles should be active again
    TEST_ASSERT_TRUE_MESSAGE(tileMap.isTileActive(5, 5), 
        "All tiles should be active after reinitialization");
    TEST_ASSERT_TRUE_MESSAGE(tileMap.isTileActive(3, 3), 
        "All tiles should be active after reinitialization");
    
    // Verify we can still modify tiles after reinitialization
    tileMap.setTileActive(7, 7, false);
    TEST_ASSERT_FALSE(tileMap.isTileActive(7, 7));
    TEST_ASSERT_TRUE(tileMap.isTileActive(5, 5)); // Previous tile should still be active
    
    tileMap.cleanupRuntimeMask();
}

int main(void) {
    UNITY_BEGIN();
    
    RUN_TEST(test_tile_mask_initialization_all_active);
    RUN_TEST(test_tile_mask_set_single_inactive);
    RUN_TEST(test_tile_mask_reactivate_tile);
    RUN_TEST(test_tile_mask_boundary_conditions);
    RUN_TEST(test_tile_mask_without_mask_all_active);
    RUN_TEST(test_tile_mask_memory_efficiency);
    RUN_TEST(test_tile_mask_multiple_operations);
    RUN_TEST(test_tile_mask_reinitialization);
    
    return UNITY_END();
}
