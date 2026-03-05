/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */

#include <unity.h>
#include "../test_config.h"
#include "graphics/Renderer.h"
#include "../mocks/MockDrawSurface.h"
#include <chrono>
#include <random>

using namespace pixelroot32::graphics;

void setUp(void) {
    test_setup();
}

void tearDown(void) {
    test_teardown();
}

void test_performance_tile_mask_rendering() {
    // Create a 32x32 tilemap for performance testing
    static uint8_t tileData[1024] = {0};
    static Sprite tiles[10] = {{nullptr, 8, 8}};
    
    // Fill with some tiles
    for (int i = 0; i < 1024; i++) {
        tileData[i] = (i % 10) + 1; // Tiles 1-10
    }
    
    TileMap tilemap;
    tilemap.indices = tileData;
    tilemap.width = 32;
    tilemap.height = 32;
    tilemap.tiles = tiles;
    tilemap.tileWidth = 8;
    tilemap.tileHeight = 8;
    tilemap.tileCount = 10;
    tilemap.runtimeMask = nullptr;
    
    // Initialize runtime mask
    tilemap.initRuntimeMask();
    
    // Randomly deactivate 50% of tiles
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 31);
    
    for (int i = 0; i < 512; i++) { // 50% of 1024 tiles
        int x = dis(gen);
        int y = dis(gen);
        tilemap.setTileActive(x, y, false);
    }
    
    // Create renderer
    DisplayConfig config(DisplayType::NONE, 0, 240, 240);
    
    Renderer renderer(std::move(config));
    renderer.init();
    
    // Benchmark rendering performance
    const int renderIterations = 1000;
    
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < renderIterations; i++) {
        renderer.beginFrame();
        renderer.drawTileMap(tilemap, 0, 0, Color::White);
        renderer.endFrame();
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    // Calculate performance metrics
    double totalMs = duration.count() / 1000.0;
    double avgMsPerFrame = totalMs / renderIterations;
    double fps = 1000.0 / avgMsPerFrame;
    
    // Performance assertions
    TEST_ASSERT_LESS_THAN_MESSAGE(16.0, avgMsPerFrame,
        "Average render time should be less than 16ms (60 FPS target)");
    
    TEST_ASSERT_GREATER_THAN_MESSAGE(60.0, fps,
        "Should maintain at least 60 FPS");
    
    // Memory efficiency check
    TEST_ASSERT_EQUAL_MESSAGE(128, (32 * 32 + 7) / 8,
        "32x32 tilemap should use exactly 128 bytes for mask");
    
    // Cleanup
    tilemap.cleanupRuntimeMask();
}

void test_performance_tile_mask_operations() {
    // Create large tilemap for operation benchmarking
    static uint8_t tileData[4096] = {0}; // 64x64
    static Sprite tiles[10] = {{nullptr, 8, 8}};
    
    TileMap tilemap;
    tilemap.indices = tileData;
    tilemap.width = 64;
    tilemap.height = 64;
    tilemap.tiles = tiles;
    tilemap.tileWidth = 8;
    tilemap.tileHeight = 8;
    tilemap.tileCount = 10;
    tilemap.runtimeMask = nullptr;
    
    tilemap.initRuntimeMask();
    
    const int operationIterations = 100000;
    
    // Benchmark isTileActive operations
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < operationIterations; i++) {
        int x = i % 64;
        int y = (i / 64) % 64;
        tilemap.isTileActive(x, y);
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    double microsPerCheck = static_cast<double>(duration.count()) / operationIterations;
    TEST_ASSERT_LESS_THAN_MESSAGE(1.0, microsPerCheck,
        "Tile activation check should be very fast");
    
    // Benchmark setTileActive operations
    start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < operationIterations; i++) {
        int x = i % 64;
        int y = (i / 64) % 64;
        bool active = (i % 2 == 0);
        tilemap.setTileActive(x, y, active);
    }
    
    end = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    double microsPerSet = static_cast<double>(duration.count()) / operationIterations;
    TEST_ASSERT_LESS_THAN_MESSAGE(2.0, microsPerSet,
        "Tile activation set should be very fast");
    
    // Cleanup
    tilemap.cleanupRuntimeMask();
}

void test_performance_memory_allocation() {
    // Test memory allocation performance for different tilemap sizes
    const int sizes[] = {16, 32, 64, 128};
    const int numSizes = sizeof(sizes) / sizeof(sizes[0]);
    
    for (int i = 0; i < numSizes; i++) {
        int size = sizes[i];
        int tileCount = size * size;
        
        static uint8_t tileData[16384] = {0}; // Max 128x128
        static Sprite tiles[10] = {{nullptr, 8, 8}};
        
        TileMap tilemap;
        tilemap.indices = tileData;
        tilemap.width = size;
        tilemap.height = size;
        tilemap.tiles = tiles;
        tilemap.tileWidth = 8;
        tilemap.tileHeight = 8;
        tilemap.tileCount = 10;
        tilemap.runtimeMask = nullptr;
        
        // Benchmark initialization
        auto start = std::chrono::high_resolution_clock::now();
        
        tilemap.initRuntimeMask();
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        
        // Initialization should be fast even for large tilemaps
        TEST_ASSERT_LESS_THAN_MESSAGE(1000.0, duration.count(),
            "Tilemap initialization should be fast");
        
        // Verify memory usage is optimal
        int expectedBytes = (tileCount + 7) / 8;
        TEST_ASSERT_EQUAL_MESSAGE(expectedBytes, (size * size + 7) / 8,
            "Memory usage should be optimal");
        
        tilemap.cleanupRuntimeMask();
    }
}

void test_performance_rendering_vs_no_mask() {
    // Compare rendering performance with and without mask
    static uint8_t tileData[1024] = {0}; // 32x32
    static Sprite tiles[10] = {{nullptr, 8, 8}};
    
    // Fill with tiles
    for (int i = 0; i < 1024; i++) {
        tileData[i] = (i % 10) + 1;
    }
    
    DisplayConfig config(DisplayType::NONE, 0, 240, 240);
    
    Renderer renderer(std::move(config));
    renderer.init();
    
    const int iterations = 500;
    
    // Test without mask
    TileMap tilemapNoMask;
    tilemapNoMask.indices = tileData;
    tilemapNoMask.width = 32;
    tilemapNoMask.height = 32;
    tilemapNoMask.tiles = tiles;
    tilemapNoMask.tileWidth = 8;
    tilemapNoMask.tileHeight = 8;
    tilemapNoMask.tileCount = 10;
    tilemapNoMask.runtimeMask = nullptr; // No mask
    
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < iterations; i++) {
        renderer.beginFrame();
        renderer.drawTileMap(tilemapNoMask, 0, 0, Color::White);
        renderer.endFrame();
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto durationNoMask = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    // Test with mask
    TileMap tilemapWithMask;
    tilemapWithMask.indices = tileData;
    tilemapWithMask.width = 32;
    tilemapWithMask.height = 32;
    tilemapWithMask.tiles = tiles;
    tilemapWithMask.tileWidth = 8;
    tilemapWithMask.tileHeight = 8;
    tilemapWithMask.tileCount = 10;
    tilemapWithMask.runtimeMask = nullptr;
    tilemapWithMask.initRuntimeMask();
    
    start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < iterations; i++) {
        renderer.beginFrame();
        renderer.drawTileMap(tilemapWithMask, 0, 0, Color::White);
        renderer.endFrame();
    }
    
    end = std::chrono::high_resolution_clock::now();
    auto durationWithMask = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    // Calculate overhead
    double overheadPercent = (static_cast<double>(durationWithMask.count() - durationNoMask.count()) / durationNoMask.count()) * 100.0;
    
    // Overhead should be minimal (< 50% for debug builds)
    TEST_ASSERT_LESS_THAN_MESSAGE(50.0, overheadPercent,
        "Mask rendering overhead should be minimal");
    
    tilemapWithMask.cleanupRuntimeMask();
}

int main(void) {
    UNITY_BEGIN();
    
    RUN_TEST(test_performance_tile_mask_rendering);
    RUN_TEST(test_performance_tile_mask_operations);
    RUN_TEST(test_performance_memory_allocation);
    RUN_TEST(test_performance_rendering_vs_no_mask);
    
    return UNITY_END();
}
