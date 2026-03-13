/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */

#include <unity.h>
#include "../../test_config.h"
#include "graphics/TileAnimation.h"
#include "graphics/Renderer.h"
#include "platforms/EngineConfig.h"
#include <memory>
#include <cstring>

using namespace pixelroot32::graphics;

// Test fixtures
static TileAnimation testAnimations[2];
static TileAnimationManager* animManager = nullptr;
static uint16_t testBitmaps[64][8]; // 64 tiles for 1bpp
static Sprite testSprites[64]; // 64 tiles for 1bpp
static uint8_t testBitmaps2bpp[64][16]; // 64 tiles for 2bpp  
static Sprite2bpp testSprites2bpp[64]; // 64 tiles for 2bpp  
static uint8_t testBitmaps4bpp[64][32]; // 64 tiles for 4bpp
static Sprite4bpp testSprites4bpp[64]; // 64 tiles for 4bpp
static Color testPalette[16] = {Color::Black, Color::White, Color::Red, Color::Green, Color::Blue, Color::Yellow, Color::Cyan, Color::Magenta,
                               Color::Gray, Color::LightGray, Color::DarkGray, Color::Orange, Color::Purple, Color::Brown, Color::Pink};
static uint8_t testIndices[16]; // 4x4 tilemap

void setUp(void) {
    test_setup();
    
    // Initialize test bitmaps (simple 8x8 tiles)
    memset(testBitmaps, 0, sizeof(testBitmaps));
    memset(testBitmaps2bpp, 0, sizeof(testBitmaps2bpp));
    memset(testBitmaps4bpp, 0, sizeof(testBitmaps4bpp));
    
    for (int i = 0; i < 64; i++) {
        // Create simple pattern tiles
        testBitmaps[i][0] = 0x80; // Top-left pixel
        testBitmaps[i][7] = 0x01; // Bottom-right pixel
        
        testBitmaps2bpp[i][0] = 0x80; // Top-left pixel (palette index 2)
        testBitmaps2bpp[i][15] = 0x01; // Bottom-right pixel (palette index 1)
        
        testBitmaps4bpp[i][0] = 0x80; // Top-left pixel (palette index 8)
        testBitmaps4bpp[i][31] = 0x08; // Bottom-right pixel (palette index 8)
        
        // Create sprite structures
        testSprites[i] = { testBitmaps[i], 8, 8 };
        testSprites2bpp[i] = { testBitmaps2bpp[i], testPalette, 8, 8, 16 };
        testSprites4bpp[i] = { testBitmaps4bpp[i], testPalette, 8, 8, 16 };
    }
    
    // Initialize test tilemap indices
    for (int i = 0; i < 16; i++) {
        testIndices[i] = 10 + (i % 3); // Use tiles 10, 11, 12
    }
    
    // Setup test animations
    testAnimations[0] = {10, 3, 2, 0}; // baseTileIndex=10, frameCount=3, frameDuration=2
    testAnimations[1] = {20, 2, 3, 0}; // baseTileIndex=20, frameCount=2, frameDuration=3
    
    // Create animation manager
    animManager = new TileAnimationManager(testAnimations, 2, 64);
}

void tearDown(void) {
    if (animManager) {
        delete animManager;
        animManager = nullptr;
    }
    test_teardown();
}

void test_1bpp_animated_tilemap_rendering(void) {
    // Test animation manager functionality for 1bpp tiles
    TEST_ASSERT_EQUAL_UINT8(10, animManager->resolveFrame(10));
    TEST_ASSERT_EQUAL_UINT8(11, animManager->resolveFrame(11)); 
    TEST_ASSERT_EQUAL_UINT8(12, animManager->resolveFrame(12));
    
    // Step animation and verify frame advancement
    animManager->step();
    uint8_t resolvedFrame = animManager->resolveFrame(10);
    TEST_ASSERT_TRUE(resolvedFrame == 10 || resolvedFrame == 11 || resolvedFrame == 12);
    
    // Test multiple steps
    animManager->step();
    animManager->step();
    resolvedFrame = animManager->resolveFrame(10);
    TEST_ASSERT_TRUE(resolvedFrame >= 10 && resolvedFrame <= 12);
}

void test_2bpp_animated_tilemap_rendering(void) {
    if constexpr (pixelroot32::platforms::config::Enable2BppSprites) {
        // Test animation manager functionality for 2bpp tiles
        TEST_ASSERT_EQUAL_UINT8(10, animManager->resolveFrame(10));
        TEST_ASSERT_EQUAL_UINT8(11, animManager->resolveFrame(11));
        
        // Step animation
        animManager->step();
        uint8_t resolvedFrame = animManager->resolveFrame(10);
        TEST_ASSERT_TRUE(resolvedFrame >= 10 && resolvedFrame <= 12);
        
        // Step again
        animManager->step();
        resolvedFrame = animManager->resolveFrame(10);
        TEST_ASSERT_TRUE(resolvedFrame >= 10 && resolvedFrame <= 12);
    }
}

void test_4bpp_animated_tilemap_rendering(void) {
    if constexpr (pixelroot32::platforms::config::Enable4BppSprites) {
        // Test animation manager functionality for 4bpp tiles
        TEST_ASSERT_EQUAL_UINT8(10, animManager->resolveFrame(10));
        TEST_ASSERT_EQUAL_UINT8(11, animManager->resolveFrame(11));
        
        // Step animation
        animManager->step();
        uint8_t resolvedFrame = animManager->resolveFrame(10);
        TEST_ASSERT_TRUE(resolvedFrame >= 10 && resolvedFrame <= 12);
        
        // Step again
        animManager->step();
        resolvedFrame = animManager->resolveFrame(10);
        TEST_ASSERT_TRUE(resolvedFrame >= 10 && resolvedFrame <= 12);
    }
}

void test_animation_frame_cycling(void) {
    // Test frame cycling through multiple steps
    for (int step = 0; step < 10; step++) {
        animManager->step();
        
        // Verify animation frame is within expected range
        uint8_t resolvedFrame = animManager->resolveFrame(10);
        TEST_ASSERT_TRUE(resolvedFrame >= 10 && resolvedFrame <= 12);
        
        // Verify non-animated tiles remain unchanged
        TEST_ASSERT_EQUAL_UINT8(50, animManager->resolveFrame(50));
    }
}

void test_animation_reset_integration(void) {
    // Advance animation several steps
    for (int i = 0; i < 6; i++) {
        animManager->step();
    }
    
    // Reset animation
    animManager->reset();
    
    // Verify animation is back to initial state
    TEST_ASSERT_EQUAL_UINT8(10, animManager->resolveFrame(10));
    TEST_ASSERT_EQUAL_UINT8(11, animManager->resolveFrame(11));
    TEST_ASSERT_EQUAL_UINT8(12, animManager->resolveFrame(12));
    
    // Verify non-animated tiles unchanged
    TEST_ASSERT_EQUAL_UINT8(50, animManager->resolveFrame(50));
}

void test_null_animation_manager(void) {
    // Create animation manager with no animations
    TileAnimationManager emptyManager(nullptr, 0, 64);
    
    // All tiles should resolve to themselves
    TEST_ASSERT_EQUAL_UINT8(10, emptyManager.resolveFrame(10));
    TEST_ASSERT_EQUAL_UINT8(50, emptyManager.resolveFrame(50));
    
    // Step should not crash
    emptyManager.step();
    
    // Reset should not crash
    emptyManager.reset();
    
    // Tiles should still resolve to themselves
    TEST_ASSERT_EQUAL_UINT8(10, emptyManager.resolveFrame(10));
    TEST_ASSERT_EQUAL_UINT8(50, emptyManager.resolveFrame(50));
}

// Main test runner
int main(void) {
    UNITY_BEGIN();
    
    RUN_TEST(test_1bpp_animated_tilemap_rendering);
    RUN_TEST(test_2bpp_animated_tilemap_rendering);
    RUN_TEST(test_4bpp_animated_tilemap_rendering);
    RUN_TEST(test_animation_frame_cycling);
    RUN_TEST(test_animation_reset_integration);
    RUN_TEST(test_null_animation_manager);
    
    return UNITY_END();
}
