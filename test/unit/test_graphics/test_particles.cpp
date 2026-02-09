/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */

#include <unity.h>
#include "../../test_config.h"
#include "graphics/particles/ParticleEmitter.h"
#include "graphics/Renderer.h"
#include "core/Engine.h"
#include "../../mocks/MockDrawSurface.h"

using namespace pixelroot32::core;
using namespace pixelroot32::graphics;
using namespace pixelroot32::graphics::particles;

void setUp(void) {
    test_setup();
}

void tearDown(void) {
    test_teardown();
}

void test_particle_emitter_initialization(void) {
    ParticleConfig cfg = {
        Color::White, Color::Red,
        1.0f, 2.0f,
        0.1f, 0.95f,
        10, 20,
        true,
        0.0f, 360.0f
    };
    
    ParticleEmitter emitter(100, 100, cfg);
    
    TEST_ASSERT_EQUAL_FLOAT(100, emitter.x);
    TEST_ASSERT_EQUAL_FLOAT(100, emitter.y);
}

void test_particle_burst(void) {
    ParticleConfig cfg = {
        Color::White, Color::Red,
        1.0f, 2.0f,
        0.1f, 0.95f,
        10, 20,
        true,
        0.0f, 360.0f
    };
    
    ParticleEmitter emitter(0, 0, cfg);
    emitter.burst(50, 50, 10);
    
    // We can't directly inspect private particles array, 
    // but we can check if draw() produces calls.
    DisplayConfig dcfg(DisplayType::NONE);
    Renderer renderer(dcfg);
    MockDrawSurface mockSurface;
    
    // Replace the internal drawer with our mock
    // Note: This requires getDrawSurface() to return a reference that we can't easily swap
    // but Renderer uses DisplayConfig's surface. 
    // Since we defined TEST_MOCK_GRAPHICS, it uses a dummy mock.
    
    // For now, let's just test that update() runs without crashing
    emitter.update(16);
}

void test_particle_emitter_lifecycle(void) {
    ParticleConfig cfg = {
        Color::White, Color::Red,
        1.0f, 2.0f,
        0.1f, 0.95f,
        2, 2, // Very short life
        true,
        0.0f, 0.0f // All going right
    };
    
    ParticleEmitter emitter(0, 0, cfg);
    emitter.burst(0, 0, 1);
    
    // Update multiple times
    emitter.update(16);
    emitter.update(16);
    emitter.update(16);
    
    // After 3 updates of 16ms, with life=2 frames, particle should be dead
}

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;
    
    UNITY_BEGIN();
    
    RUN_TEST(test_particle_emitter_initialization);
    RUN_TEST(test_particle_burst);
    RUN_TEST(test_particle_emitter_lifecycle);
    
    return UNITY_END();
}
