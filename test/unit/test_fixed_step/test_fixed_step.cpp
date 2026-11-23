/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 *
 * The frame loop's one non-negotiable property: the simulation advances in
 * fixed steps, so the physics a scene sees does not depend on how fast the
 * display draws. These tests drive runFrame(elapsedMs) with a hand-held clock,
 * which is the whole reason that overload exists -- nothing here depends on the
 * wall clock, so the assertions are exact.
 */
#include <unity.h>
#include "core/Engine.h"
#include "core/Scene.h"
#include "graphics/DisplayConfig.h"
#include "mocks/MockDrawSurface.h"

using pixelroot32::core::Engine;
using pixelroot32::core::Scene;
using pixelroot32::graphics::DisplayConfig;
using pixelroot32::graphics::MockDrawSurface;
using pixelroot32::graphics::Renderer;

void setUp(void) {}
void tearDown(void) {}

namespace {

/**
 * A scene whose position is a pure function of the sum of the dt it was handed.
 * If the frame time ever leaked into update(), the two cadences below would
 * land on different positions.
 */
class IntegratorScene : public Scene {
public:
    void update(unsigned long deltaTime) override {
        updateCount++;
        lastDeltaTime = deltaTime;
        position += velocity * static_cast<long>(deltaTime);
    }

    void draw(Renderer& renderer) override {
        (void)renderer;
        drawCount++;
    }

    // Keep draw() out of these tests: they measure simulation, not rendering.
    bool shouldRedrawFramebuffer() const override { return false; }

    int updateCount = 0;
    int drawCount = 0;
    unsigned long lastDeltaTime = 0;
    long position = 0;
    long velocity = 3;
};

DisplayConfig makeDisplayConfig() {
    return PIXELROOT32_CUSTOM_DISPLAY(new MockDrawSurface(), 240, 240);
}

/**
 * Runs `frames` frames of `frameMs` each and returns the scene's state.
 * The engine and the scene are owned by the caller's scope.
 */
template <typename Frame>
void drive(Engine& engine, IntegratorScene& scene, int frames, Frame frameMs) {
    engine.init();
    engine.setScene(&scene);
    for (int i = 0; i < frames; ++i) {
        TEST_ASSERT_TRUE(engine.runFrame(frameMs(i)));
    }
}

}  // namespace

// =============================================================================
// 1. The exit criterion of phase 1: same wall-clock time, different cadence,
//    identical physics. 63 x 16 ms and 144 x 7 ms are both 1008 ms.
// =============================================================================

void test_same_elapsed_time_gives_same_physics(void) {
    DisplayConfig slowConfig = makeDisplayConfig();
    Engine slowEngine(slowConfig);
    IntegratorScene slowScene;
    drive(slowEngine, slowScene, 63, [](int) { return Engine::FIXED_STEP_MS; });

    DisplayConfig fastConfig = makeDisplayConfig();
    Engine fastEngine(fastConfig);
    IntegratorScene fastScene;
    drive(fastEngine, fastScene, 144, [](int) { return 7UL; });

    // Both cover 1008 ms, which is exactly 63 fixed steps.
    TEST_ASSERT_EQUAL_INT(63, slowScene.updateCount);
    TEST_ASSERT_EQUAL_INT(63, fastScene.updateCount);
    TEST_ASSERT_EQUAL_INT(slowScene.position, fastScene.position);
    TEST_ASSERT_EQUAL_INT(63L * 16L * 3L, fastScene.position);
}

// =============================================================================
// 2. No scene ever sees the frame time: every update() is one whole step, even
//    when the frame was 7 ms or 16 ms.
// =============================================================================

void test_scene_only_sees_whole_fixed_steps(void) {
    DisplayConfig config = makeDisplayConfig();
    Engine engine(config);
    IntegratorScene scene;
    drive(engine, scene, 10, [](int) { return 7UL; });

    // 70 ms is four whole steps, with 6 ms still waiting in the accumulator.
    TEST_ASSERT_EQUAL_INT(4, scene.updateCount);
    TEST_ASSERT_EQUAL_UINT(Engine::FIXED_STEP_MS, scene.lastDeltaTime);
    TEST_ASSERT_EQUAL_INT(4L * 16L * 3L, scene.position);
}

// =============================================================================
// 3. getDeltaTime() reports the step while the engine drives the loop.
// =============================================================================

void test_delta_time_is_the_step(void) {
    DisplayConfig config = makeDisplayConfig();
    Engine engine(config);
    IntegratorScene scene;
    drive(engine, scene, 1, [](int) { return Engine::FIXED_STEP_MS; });

    TEST_ASSERT_EQUAL_UINT(Engine::FIXED_STEP_MS, engine.getDeltaTime());
    TEST_ASSERT_EQUAL_UINT(Engine::FIXED_STEP_MS, scene.lastDeltaTime);
}

// =============================================================================
// 4. A hitch is clamped instead of turning into a catch-up burst, and the
//    excess is dropped rather than paid back over the following frames.
// =============================================================================

void test_long_hitch_is_clamped(void) {
    DisplayConfig config = makeDisplayConfig();
    Engine engine(config);
    IntegratorScene scene;
    drive(engine, scene, 1, [](int) { return 1000UL; });

    TEST_ASSERT_EQUAL_INT(static_cast<int>(Engine::MAX_STEPS_PER_FRAME),
                          scene.updateCount);

    // One ordinary frame after the hitch must not pay off the 936 ms of debt.
    TEST_ASSERT_TRUE(engine.runFrame(Engine::FIXED_STEP_MS));
    TEST_ASSERT_EQUAL_INT(static_cast<int>(Engine::MAX_STEPS_PER_FRAME) + 1,
                          scene.updateCount);
}

// =============================================================================
// 5. An ordinary frame reports "keep running" and advances exactly one step.
//    (The other half of the contract, a closed window, needs a real surface:
//    MockDrawSurface inherits DrawSurface's processEvents() == true.)
// =============================================================================

void test_ordinary_frame_reports_true_and_steps(void) {
    DisplayConfig config = makeDisplayConfig();
    Engine engine(config);
    IntegratorScene scene;
    drive(engine, scene, 3, [](int) { return Engine::FIXED_STEP_MS; });
    const int stepsBefore = scene.updateCount;

    TEST_ASSERT_TRUE(engine.runFrame(Engine::FIXED_STEP_MS));
    TEST_ASSERT_EQUAL_INT(stepsBefore + 1, scene.updateCount);
}

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;
    UNITY_BEGIN();
    RUN_TEST(test_same_elapsed_time_gives_same_physics);
    RUN_TEST(test_scene_only_sees_whole_fixed_steps);
    RUN_TEST(test_delta_time_is_the_step);
    RUN_TEST(test_long_hitch_is_clamped);
    RUN_TEST(test_ordinary_frame_reports_true_and_steps);
    return UNITY_END();
}
