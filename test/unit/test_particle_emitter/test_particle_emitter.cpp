/**
 * @file test_particle_emitter.cpp
 * @brief Unit tests for graphics/particles/ParticleEmitter module
 * @version 1.0
 * @date 2026-03-29
 * 
 * Tests for ParticleEmitter using only public API:
 * - Constructor initialization
 * - Burst spawning (without accessing internal state)
 * - Update and draw (with mocked renderer)
 * - Edge cases
 */

#include <unity.h>
#include "../../test_config.h"

#ifdef PIXELROOT32_ENABLE_PARTICLES

#include "graphics/particles/ParticleEmitter.h"
#include "graphics/particles/ParticleConfig.h"
#include "graphics/particles/Particle.h"
#include "math/Vector2.h"
#include "core/Engine.h"
#include "graphics/Renderer.h"
#include "graphics/DisplayConfig.h"
#include "graphics/BaseDrawSurface.h"

using namespace pixelroot32::graphics::particles;
using namespace pixelroot32::math;
using namespace pixelroot32::core;
using namespace pixelroot32::graphics;

// Global engine for tests that need it
extern Engine engine;

void setUp(void) {
    test_setup();
}

void tearDown(void) {
    test_teardown();
}

// =============================================================================
// Test fixtures - Create valid ParticleConfig using correct types
// =============================================================================

ParticleConfig createBasicConfig() {
    ParticleConfig cfg;
    cfg.minAngleDeg = 0;
    cfg.maxAngleDeg = 360;
    cfg.minSpeed = 1.0f;  // Scalar (float), not Fixed16
    cfg.maxSpeed = 5.0f;
    cfg.minLife = 30;
    cfg.maxLife = 60;
    cfg.gravity = 0.0f;
    cfg.friction = 1.0f;
    cfg.startColor = Color::White;
    cfg.endColor = Color::Black;
    cfg.fadeColor = false;
    return cfg;
}

ParticleConfig createExplosionConfig() {
    ParticleConfig cfg;
    cfg.minAngleDeg = 0;
    cfg.maxAngleDeg = 360;
    cfg.minSpeed = 2.0f;
    cfg.maxSpeed = 8.0f;
    cfg.minLife = 20;
    cfg.maxLife = 40;
    cfg.gravity = 0.5f;
    cfg.friction = 0.95f;
    cfg.startColor = Color::Yellow;
    cfg.endColor = Color::Red;
    cfg.fadeColor = true;
    return cfg;
}

// =============================================================================
// Constructor tests
// =============================================================================

void test_particle_emitter_constructor_basic(void) {
    ParticleConfig cfg = createBasicConfig();
    Vector2 pos(100, 100);
    
    // Should not crash
    ParticleEmitter emitter(pos, cfg);
    
    // Basic verification through existence
    TEST_ASSERT_TRUE(true);
}

void test_particle_emitter_constructor_at_origin(void) {
    ParticleConfig cfg = createBasicConfig();
    Vector2 pos(0, 0);
    
    // Should not crash
    ParticleEmitter emitter(pos, cfg);
    
    TEST_ASSERT_TRUE(true);
}

void test_particle_emitter_constructor_different_positions(void) {
    ParticleConfig cfg = createBasicConfig();
    
    // Test various positions
    ParticleEmitter emitter1(Vector2(0, 0), cfg);
    ParticleEmitter emitter2(Vector2(100, 50), cfg);
    ParticleEmitter emitter3(Vector2(-50, -100), cfg);
    
    TEST_ASSERT_TRUE(true);
}

// =============================================================================
// Burst tests - only verify no crash, cannot verify internal state
// =============================================================================

void test_particle_emitter_burst_basic(void) {
    ParticleConfig cfg = createBasicConfig();
    Vector2 pos(100, 100);
    ParticleEmitter emitter(pos, cfg);
    
    // Should not crash - burst particles
    emitter.burst(pos, 5);
    
    TEST_ASSERT_TRUE(true);
}

void test_particle_emitter_burst_zero_count(void) {
    ParticleConfig cfg = createBasicConfig();
    Vector2 pos(100, 100);
    ParticleEmitter emitter(pos, cfg);
    
    // Edge case: burst with 0 particles should not crash
    emitter.burst(pos, 0);
    
    TEST_ASSERT_TRUE(true);
}

void test_particle_emitter_burst_larger_count(void) {
    ParticleConfig cfg = createBasicConfig();
    Vector2 pos(100, 100);
    ParticleEmitter emitter(pos, cfg);
    
    // Burst more than MAX_PARTICLES - should just cap at max
    emitter.burst(pos, 100);
    
    TEST_ASSERT_TRUE(true);
}

void test_particle_emitter_burst_at_different_positions(void) {
    ParticleConfig cfg = createBasicConfig();
    ParticleEmitter emitter(Vector2(0, 0), cfg);
    
    // Burst at different positions
    emitter.burst(Vector2(10, 20), 3);
    emitter.burst(Vector2(50, 60), 2);
    emitter.burst(Vector2(-10, -5), 1);
    
    TEST_ASSERT_TRUE(true);
}

void test_particle_emitter_multiple_bursts(void) {
    ParticleConfig cfg = createBasicConfig();
    Vector2 pos(100, 100);
    ParticleEmitter emitter(pos, cfg);
    
    // Multiple bursts
    emitter.burst(pos, 5);
    emitter.burst(pos, 5);
    emitter.burst(pos, 5);
    
    TEST_ASSERT_TRUE(true);
}

// =============================================================================
// Update tests - requires engine setup for screen dimensions
// =============================================================================

void test_particle_emitter_update_basic(void) {
    ParticleConfig cfg = createBasicConfig();
    Vector2 pos(100, 100);
    ParticleEmitter emitter(pos, cfg);
    
    emitter.burst(pos, 5);
    
    // Update should not crash - requires screen dimensions from engine
    unsigned long deltaTime = 16;
    emitter.update(deltaTime);
    
    TEST_ASSERT_TRUE(true);
}

void test_particle_emitter_update_zero_delta(void) {
    ParticleConfig cfg = createBasicConfig();
    Vector2 pos(100, 100);
    ParticleEmitter emitter(pos, cfg);
    
    emitter.burst(pos, 3);
    
    // Edge case: zero delta time
    emitter.update(0);
    
    TEST_ASSERT_TRUE(true);
}

void test_particle_emitter_update_large_delta(void) {
    ParticleConfig cfg = createBasicConfig();
    Vector2 pos(100, 100);
    ParticleEmitter emitter(pos, cfg);
    
    emitter.burst(pos, 3);
    
    // Edge case: very large delta time
    emitter.update(1000);
    
    TEST_ASSERT_TRUE(true);
}

void test_particle_emitter_update_no_burst(void) {
    ParticleConfig cfg = createBasicConfig();
    Vector2 pos(100, 100);
    ParticleEmitter emitter(pos, cfg);
    
    // Update without any particles - should not crash
    emitter.update(16);
    
    TEST_ASSERT_TRUE(true);
}

// =============================================================================
// Draw tests - use mock renderer
// =============================================================================

namespace {
class MockDrawSurfaceParticle : public BaseDrawSurface {
public:
    std::vector<std::tuple<int, int, int, int, uint16_t>> rectCalls;
    
    void init() override {}
    void clearBuffer() override { rectCalls.clear(); }
    void sendBuffer() override {}
    void drawRectangle(int x, int y, int w, int h, uint16_t c) override {}
    void drawFilledRectangle(int x, int y, int w, int h, uint16_t c) override {
        rectCalls.push_back({x, y, w, h, c});
    }
    void drawPixel(int, int, uint16_t) override {}
};
}

void test_particle_emitter_draw_basic(void) {
    auto mockDrawer = std::make_unique<MockDrawSurfaceParticle>();
    DisplayConfig config = PIXELROOT32_CUSTOM_DISPLAY(mockDrawer.release(), 240, 240);
    Renderer renderer(config);
    
    ParticleConfig cfg = createBasicConfig();
    Vector2 pos(100, 100);
    ParticleEmitter emitter(pos, cfg);
    
    emitter.burst(pos, 3);
    
    // Draw should not crash
    emitter.draw(renderer);
    
    TEST_ASSERT_TRUE(true);
}

void test_particle_emitter_draw_no_burst(void) {
    auto mockDrawer = std::make_unique<MockDrawSurfaceParticle>();
    DisplayConfig config = PIXELROOT32_CUSTOM_DISPLAY(mockDrawer.release(), 240, 240);
    Renderer renderer(config);
    
    ParticleConfig cfg = createBasicConfig();
    Vector2 pos(100, 100);
    ParticleEmitter emitter(pos, cfg);
    
    // Draw without any particles - should still work
    emitter.draw(renderer);
    
    TEST_ASSERT_TRUE(true);
}

void test_particle_emitter_draw_with_fade_color(void) {
    auto mockDrawer = std::make_unique<MockDrawSurfaceParticle>();
    DisplayConfig config = PIXELROOT32_CUSTOM_DISPLAY(mockDrawer.release(), 240, 240);
    Renderer renderer(config);
    
    ParticleConfig cfg = createExplosionConfig();  // fadeColor = true
    Vector2 pos(100, 100);
    ParticleEmitter emitter(pos, cfg);
    
    emitter.burst(pos, 3);
    emitter.update(10);  // Update to change colors
    
    // Draw with color fading
    emitter.draw(renderer);
    
    TEST_ASSERT_TRUE(true);
}

// =============================================================================
// Edge case and integration tests
// =============================================================================

void test_particle_emitter_update_then_draw(void) {
    auto mockDrawer = std::make_unique<MockDrawSurfaceParticle>();
    DisplayConfig config = PIXELROOT32_CUSTOM_DISPLAY(mockDrawer.release(), 240, 240);
    Renderer renderer(config);
    
    ParticleConfig cfg = createBasicConfig();
    Vector2 pos(100, 100);
    ParticleEmitter emitter(pos, cfg);
    
    // Full lifecycle: burst -> update -> draw
    emitter.burst(pos, 5);
    emitter.update(16);
    emitter.draw(renderer);
    
    TEST_ASSERT_TRUE(true);
}

void test_particle_emitter_multiple_updates(void) {
    ParticleConfig cfg = createBasicConfig();
    Vector2 pos(100, 100);
    ParticleEmitter emitter(pos, cfg);
    
    emitter.burst(pos, 3);
    
    // Multiple updates
    for (int i = 0; i < 10; i++) {
        emitter.update(16);
    }
    
    TEST_ASSERT_TRUE(true);
}

void test_particle_emitter_full_lifecycle(void) {
    auto mockDrawer = std::make_unique<MockDrawSurfaceParticle>();
    DisplayConfig config = PIXELROOT32_CUSTOM_DISPLAY(mockDrawer.release(), 240, 240);
    Renderer renderer(config);
    
    ParticleConfig cfg = createExplosionConfig();
    Vector2 pos(100, 100);
    ParticleEmitter emitter(pos, cfg);
    
    // Full lifecycle
    emitter.burst(pos, 10);
    
    // Run for several frames
    for (int i = 0; i < 60; i++) {
        emitter.update(16);
        emitter.draw(renderer);
    }
    
    TEST_ASSERT_TRUE(true);
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
    
    // Constructor tests
    RUN_TEST(test_particle_emitter_constructor_basic);
    RUN_TEST(test_particle_emitter_constructor_at_origin);
    RUN_TEST(test_particle_emitter_constructor_different_positions);
    
    // Burst tests
    RUN_TEST(test_particle_emitter_burst_basic);
    RUN_TEST(test_particle_emitter_burst_zero_count);
    RUN_TEST(test_particle_emitter_burst_larger_count);
    RUN_TEST(test_particle_emitter_burst_at_different_positions);
    RUN_TEST(test_particle_emitter_multiple_bursts);
    
    // Update tests
    RUN_TEST(test_particle_emitter_update_basic);
    RUN_TEST(test_particle_emitter_update_zero_delta);
    RUN_TEST(test_particle_emitter_update_large_delta);
    RUN_TEST(test_particle_emitter_update_no_burst);
    
    // Draw tests
    RUN_TEST(test_particle_emitter_draw_basic);
    RUN_TEST(test_particle_emitter_draw_no_burst);
    RUN_TEST(test_particle_emitter_draw_with_fade_color);
    
    // Integration tests
    RUN_TEST(test_particle_emitter_update_then_draw);
    RUN_TEST(test_particle_emitter_multiple_updates);
    RUN_TEST(test_particle_emitter_full_lifecycle);
    
    return UNITY_END();
}

#else

void setUp(void) {}
void tearDown(void) {}

void test_particle_emitter_disabled(void) {
    TEST_IGNORE_MESSAGE("PIXELROOT32_ENABLE_PARTICLES not defined - particles disabled");
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_particle_emitter_disabled);
    return UNITY_END();
}

#endif
