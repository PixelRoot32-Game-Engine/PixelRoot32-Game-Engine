/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 *
 * @file test_scene_transition.cpp
 * @brief Integration tests for SceneManager transition state machine + Engine integration.
 *
 * Covers:
 * - SceneManager state machine: Idle → FadingOut → SceneSwap → FadingIn → Idle
 * - Input blocking during transition (scene.update skipped)
 * - Double trigger rejection
 * - Engine::triggerTransition() delegation
 * - Draw hook applies effect during transition
 */

#include <unity.h>
#include <cstring>
#include "graphics/TransitionEffect.h"
#include "core/SceneManager.h"
#include "core/Scene.h"
#include "core/Engine.h"
#include "graphics/DisplayConfig.h"
#include "graphics/Renderer.h"
#include "../../test_config.h"
#include "../../mocks/MockDrawSurface.h"

#if PIXELROOT32_ENABLE_SCENE_TRANSITIONS

using namespace pixelroot32::core;
using namespace pixelroot32::graphics;

// =============================================================================
// Mock Scene for transition tests
// =============================================================================

class TransitionMockScene : public Scene {
public:
    bool initCalled = false;
    int updateCallCount = 0;
    unsigned long lastDeltaTime = 0;
    bool shouldRedraw = true;

    virtual ~TransitionMockScene() {}
    virtual void init() override { initCalled = true; }
    virtual void update(unsigned long dt) override {
        updateCallCount++;
        lastDeltaTime = dt;
    }
    virtual void draw(Renderer& renderer) override {
        (void)renderer;
    }
    virtual bool shouldRedrawFramebuffer() const override {
        return shouldRedraw;
    }
    void reset() {
        initCalled = false;
        updateCallCount = 0;
        lastDeltaTime = 0;
    }
};

// =============================================================================
// Forward declare helper
// =============================================================================

static void runUpdates(SceneManager* mgr, int count, unsigned long dt) {
    for (int i = 0; i < count; ++i) {
        mgr->update(dt);
    }
}

// TestEngine subclass to access protected members for Engine integration tests.
class TestTransitionEngine : public Engine {
public:
    TestTransitionEngine(const DisplayConfig& dc) : Engine(dc) {}

    void testUpdate(unsigned long dt) {
        deltaTime = dt;
        // Use nullptr instead of SDL_GetKeyboardState to avoid SDL dependency in unit tests.
#ifdef PLATFORM_NATIVE
        inputManager.update(deltaTime, nullptr);
#else
        inputManager.update(deltaTime);
#endif
        sceneManager.update(deltaTime);
    }

    void testDraw() {
        draw();
    }

    SceneManager& testSceneManager() {
        return sceneManager;
    }
};

// =============================================================================
// setUp / tearDown
// =============================================================================

void setUp(void) {
    test_setup();
}

void tearDown(void) {
    test_teardown();
}

// =============================================================================
// ST-01: Default state — not transitioning, state is Idle
// =============================================================================

void test_transition_default_state(void) {
    SceneManager mgr;
    TransitionEffect effect;

    mgr.setTransitionEffect(&effect);

    TEST_ASSERT_FALSE(mgr.isTransitioning());
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(TransitionState::Idle),
                            static_cast<uint8_t>(mgr.getTransitionState()));
}

// =============================================================================
// ST-02: transitionToScene starts FadingOut
// =============================================================================

void test_transition_starts_fading_out(void) {
    SceneManager mgr;
    TransitionEffect effect;
    TransitionMockScene currentScene;
    TransitionMockScene targetScene;

    mgr.setTransitionEffect(&effect);
    mgr.pushScene(&currentScene);
    mgr.transitionToScene(&targetScene, TransitionType::Fade, 500);

    TEST_ASSERT_TRUE(mgr.isTransitioning());
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(TransitionState::FadingOut),
                            static_cast<uint8_t>(mgr.getTransitionState()));
    TEST_ASSERT_TRUE(effect.isActive());
}

// =============================================================================
// ST-03: Input blocked during FadingOut — scene.update not called
// =============================================================================

void test_transition_input_blocked_during_fade_out(void) {
    SceneManager mgr;
    TransitionEffect effect;
    TransitionMockScene currentScene;
    TransitionMockScene targetScene;

    mgr.setTransitionEffect(&effect);
    mgr.pushScene(&currentScene);
    mgr.transitionToScene(&targetScene, TransitionType::Fade, 500);

    // Before transition, an update would be forwarded.
    TEST_ASSERT_EQUAL_INT(0, currentScene.updateCallCount);

    // During FadingOut, update is NOT forwarded to the scene.
    mgr.update(16);
    TEST_ASSERT_EQUAL_INT(0, currentScene.updateCallCount);
}

// =============================================================================
// ST-04: FadingOut → SceneSwap after effect completes
// =============================================================================

void test_transition_fade_out_completes_to_scene_swap(void) {
    SceneManager mgr;
    TransitionEffect effect;
    TransitionMockScene currentScene;
    TransitionMockScene targetScene;

    mgr.setTransitionEffect(&effect);
    mgr.pushScene(&currentScene);
    mgr.transitionToScene(&targetScene, TransitionType::Fade, 500);

    // Advance past the full duration.
    mgr.update(500);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(TransitionState::SceneSwap),
                            static_cast<uint8_t>(mgr.getTransitionState()));
}

// =============================================================================
// ST-05: Scene is atomically swapped at SceneSwap tick
// =============================================================================

void test_transition_scene_swap_atomic(void) {
    SceneManager mgr;
    TransitionEffect effect;
    TransitionMockScene currentScene;
    TransitionMockScene targetScene;

    mgr.setTransitionEffect(&effect);
    mgr.pushScene(&currentScene);
    TEST_ASSERT_EQUAL_PTR(&currentScene, mgr.getCurrentScene().value());

    mgr.transitionToScene(&targetScene, TransitionType::Fade, 500);

    // Advance past FadingOut to SceneSwap.
    mgr.update(500);

    // Now in SceneSwap state.
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(TransitionState::SceneSwap),
                            static_cast<uint8_t>(mgr.getTransitionState()));

    // On the SceneSwap tick, the scene is swapped.
    mgr.update(16);

    // Current scene should now be the target.
    TEST_ASSERT_TRUE(mgr.getCurrentScene().has_value());
    TEST_ASSERT_EQUAL_PTR(&targetScene, mgr.getCurrentScene().value());
    // The target scene's init() should have been called by setCurrentScene.
    TEST_ASSERT_TRUE(targetScene.initCalled);
}

// =============================================================================
// ST-06: After SceneSwap, FadingIn begins
// =============================================================================

void test_transition_fading_in_after_swap(void) {
    SceneManager mgr;
    TransitionEffect effect;
    TransitionMockScene currentScene;
    TransitionMockScene targetScene;

    mgr.setTransitionEffect(&effect);
    mgr.pushScene(&currentScene);
    mgr.transitionToScene(&targetScene, TransitionType::Fade, 500);

    // Advance through FadingOut and SceneSwap.
    mgr.update(500);
    mgr.update(16);

    // Should now be in FadingIn.
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(TransitionState::FadingIn),
                            static_cast<uint8_t>(mgr.getTransitionState()));
    // Effect should be active for Fade In.
    TEST_ASSERT_TRUE(effect.isActive());
    // Progress should be 0 (just started).
    TEST_ASSERT_FLOAT_EQUAL(0.0f, effect.getProgress());
}

// =============================================================================
// ST-07: Full cycle completes — Idle → FadingOut → SceneSwap → FadingIn → Idle
// =============================================================================

void test_transition_full_cycle_completes(void) {
    SceneManager mgr;
    TransitionEffect effect;
    TransitionMockScene currentScene;
    TransitionMockScene targetScene;

    mgr.setTransitionEffect(&effect);
    mgr.pushScene(&currentScene);

    // Initially Idle.
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(TransitionState::Idle),
                            static_cast<uint8_t>(mgr.getTransitionState()));

    // Start transition.
    mgr.transitionToScene(&targetScene, TransitionType::Fade, 500);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(TransitionState::FadingOut),
                            static_cast<uint8_t>(mgr.getTransitionState()));

    // FadingOut (500ms of updates).
    runUpdates(&mgr, 50, 10);  // 500ms total
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(TransitionState::SceneSwap),
                            static_cast<uint8_t>(mgr.getTransitionState()));

    // SceneSwap (one tick).
    mgr.update(10);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(TransitionState::FadingIn),
                            static_cast<uint8_t>(mgr.getTransitionState()));

    // FadingIn (500ms of updates).
    runUpdates(&mgr, 50, 10);  // 500ms total
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(TransitionState::Idle),
                            static_cast<uint8_t>(mgr.getTransitionState()));

    // Back to Idle — isTransitioning false.
    TEST_ASSERT_FALSE(mgr.isTransitioning());
}

// =============================================================================
// ST-08: Double trigger rejected during active transition
// =============================================================================

void test_transition_double_trigger_rejected(void) {
    SceneManager mgr;
    TransitionEffect effect;
    TransitionMockScene sceneA;
    TransitionMockScene sceneB;
    TransitionMockScene sceneC;

    mgr.setTransitionEffect(&effect);
    mgr.pushScene(&sceneA);
    mgr.transitionToScene(&sceneB, TransitionType::Fade, 500);

    // Try to trigger another transition while still in FadingOut.
    mgr.transitionToScene(&sceneC, TransitionType::Iris, 300);

    // Should still be targeting sceneB (the first trigger).
    // Advance past SceneSwap and check.
    mgr.update(500);  // Complete FadingOut
    mgr.update(16);   // SceneSwap

    // sceneC should NOT be the current scene.
    TEST_ASSERT_EQUAL_PTR(&sceneB, mgr.getCurrentScene().value());
}

// =============================================================================
// ST-09: Input blocked during FadingIn — scene.update not called
// =============================================================================

void test_transition_input_blocked_during_fade_in(void) {
    SceneManager mgr;
    TransitionEffect effect;
    TransitionMockScene currentScene;
    TransitionMockScene targetScene;

    mgr.setTransitionEffect(&effect);
    mgr.pushScene(&currentScene);
    mgr.transitionToScene(&targetScene, TransitionType::Fade, 500);

    // Advance through FadingOut and SceneSwap.
    mgr.update(500);  // FadingOut complete
    mgr.update(16);   // SceneSwap → now in FadingIn

    // During FadingIn, the new scene's update should NOT be called.
    TEST_ASSERT_EQUAL_INT(0, targetScene.updateCallCount);

    // Advance FadingIn a bit.
    mgr.update(16);
    TEST_ASSERT_EQUAL_INT(0, targetScene.updateCallCount);
}

// =============================================================================
// ST-10: Update resumes on the new scene after transition completes
// =============================================================================

void test_transition_update_resumes_after_complete(void) {
    SceneManager mgr;
    TransitionEffect effect;
    TransitionMockScene currentScene;
    TransitionMockScene targetScene;

    mgr.setTransitionEffect(&effect);
    mgr.pushScene(&currentScene);
    mgr.transitionToScene(&targetScene, TransitionType::Fade, 100);

    // Run through full cycle: 100ms Out + 1 tick SceneSwap + 100ms In.
    // ceil(100/8)=13 + 1 + ceil(100/8)=13 = 27 updates minimum.
    // Use exactly 27 updates so we land right at Idle with no extra Idle updates.
    runUpdates(&mgr, 27, 8);  // 216ms total

    // Should be Idle now — the 27th update triggered FadingIn → Idle.
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(TransitionState::Idle),
                            static_cast<uint8_t>(mgr.getTransitionState()));

    // After returning to Idle, scene.update should be forwarded to the new scene.
    // No updates have been forwarded yet during Idle.
    TEST_ASSERT_EQUAL_INT(0, targetScene.updateCallCount);

    mgr.update(16);
    TEST_ASSERT_EQUAL_INT(1, targetScene.updateCallCount);
}

// =============================================================================
// ST-11: Iris transition also completes full cycle
// =============================================================================

void test_transition_iris_full_cycle(void) {
    SceneManager mgr;
    TransitionEffect effect;
    TransitionMockScene currentScene;
    TransitionMockScene targetScene;

    mgr.setTransitionEffect(&effect);
    mgr.pushScene(&currentScene);
    mgr.transitionToScene(&targetScene, TransitionType::Iris, 300);

    // Advance through full cycle: 300ms Out + 1 tick SceneSwap + 300ms In.
    // ceil(300/8)=38 + 1 + 38 = 77 updates minimum.
    runUpdates(&mgr, 80, 8);  // 640ms total

    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(TransitionState::Idle),
                            static_cast<uint8_t>(mgr.getTransitionState()));
    TEST_ASSERT_FALSE(mgr.isTransitioning());
    TEST_ASSERT_EQUAL_PTR(&targetScene, mgr.getCurrentScene().value());
}

// =============================================================================
// ST-12: Engine triggerTransition delegates to SceneManager
// =============================================================================

void test_engine_trigger_transition_delegates(void) {
    auto mock = std::make_unique<MockDrawSurface>();
    DisplayConfig config = PIXELROOT32_CUSTOM_DISPLAY(mock.release(), 240, 240);
    TestTransitionEngine engine(config);
    engine.init();

    TransitionMockScene currentScene;
    TransitionMockScene targetScene;
    engine.setScene(&currentScene);

    // Before trigger: not transitioning
    TEST_ASSERT_FALSE(engine.testSceneManager().isTransitioning());

    // Trigger transition via Engine public API.
    engine.triggerTransition(&targetScene, TransitionType::Fade, 200);

    // After trigger: delegation confirmed — sceneManager is transitioning.
    TEST_ASSERT_TRUE(engine.testSceneManager().isTransitioning());

    // Run enough updates to complete the full transition cycle.
    // 200ms FadingOut + 1 tick SceneSwap + 200ms FadingIn.
    // At 8ms per internal step with 16ms test updates: well within 100 iterations.
    for (int i = 0; i < 100; ++i) {
        engine.testUpdate(16);
    }

    // Verify transition completed and scene was swapped.
    TEST_ASSERT_TRUE(engine.getCurrentScene().has_value());
    TEST_ASSERT_EQUAL_PTR(&targetScene, engine.getCurrentScene().value());
    TEST_ASSERT_FALSE(engine.testSceneManager().isTransitioning());
}

// =============================================================================
// ST-13: Draw hook applies transition effect (smoke test)
// =============================================================================

void test_engine_draw_hook_smoke(void) {
    // Create a mock draw surface with a real sprite buffer.
    auto mock = std::make_unique<MockDrawSurface>();
    uint8_t frameBuffer[240 * 240];
    memset(frameBuffer, 0xAB, sizeof(frameBuffer));
    mock->setSpriteBuffer(frameBuffer, sizeof(frameBuffer));

    DisplayConfig config = PIXELROOT32_CUSTOM_DISPLAY(mock.release(), 240, 240);
    TestTransitionEngine engine(config);
    engine.init();

    TransitionMockScene currentScene;
    TransitionMockScene targetScene;
    engine.setScene(&currentScene);

    // Trigger a transition via Engine API.
    engine.triggerTransition(&targetScene, TransitionType::Fade, 100);

    // Draw hook should be active: after trigger, transition is in progress.
    TEST_ASSERT_TRUE(engine.testSceneManager().isTransitioning());

    // Calling draw() during transition exercises the draw hook code path
    // (transitionEffect_.apply on the sprite buffer). No crash = hook works.
    engine.testDraw();

    // Run enough updates to complete the full transition cycle.
    for (int i = 0; i < 100; ++i) {
        engine.testUpdate(16);
    }

    // Verify transition completed and scene was swapped.
    TEST_ASSERT_TRUE(engine.getCurrentScene().has_value());
    TEST_ASSERT_EQUAL_PTR(&targetScene, engine.getCurrentScene().value());
    TEST_ASSERT_FALSE(engine.testSceneManager().isTransitioning());
}

// =============================================================================
// ST-14: transitionToScene with null effect pointer — no crash
// =============================================================================

void test_transition_no_effect_pointer(void) {
    SceneManager mgr;
    // Intentionally NOT setting an effect pointer.
    TransitionMockScene currentScene;
    TransitionMockScene targetScene;

    mgr.pushScene(&currentScene);
    mgr.transitionToScene(&targetScene, TransitionType::Fade, 500);

    // Should immediately transition through states even without an effect.
    TEST_ASSERT_TRUE(mgr.isTransitioning());

    // Update should not crash despite null effect pointer.
    mgr.update(16);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(TransitionState::SceneSwap),
                            static_cast<uint8_t>(mgr.getTransitionState()));

    mgr.update(16);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(TransitionState::FadingIn),
                            static_cast<uint8_t>(mgr.getTransitionState()));

    mgr.update(16);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(TransitionState::Idle),
                            static_cast<uint8_t>(mgr.getTransitionState()));

    // Scene should still be swapped.
    TEST_ASSERT_EQUAL_PTR(&targetScene, mgr.getCurrentScene().value());
}

// =============================================================================
// main
// =============================================================================

int main(void) {
    UNITY_BEGIN();

    // State machine defaults
    RUN_TEST(test_transition_default_state);

    // FadingOut phase
    RUN_TEST(test_transition_starts_fading_out);
    RUN_TEST(test_transition_input_blocked_during_fade_out);
    RUN_TEST(test_transition_fade_out_completes_to_scene_swap);

    // SceneSwap phase
    RUN_TEST(test_transition_scene_swap_atomic);

    // FadingIn phase
    RUN_TEST(test_transition_fading_in_after_swap);
    RUN_TEST(test_transition_input_blocked_during_fade_in);

    // Full cycle
    RUN_TEST(test_transition_full_cycle_completes);
    RUN_TEST(test_transition_iris_full_cycle);
    RUN_TEST(test_transition_update_resumes_after_complete);

    // Edge cases
    RUN_TEST(test_transition_double_trigger_rejected);
    RUN_TEST(test_transition_no_effect_pointer);

    // Engine integration smoke tests
    RUN_TEST(test_engine_trigger_transition_delegates);
    RUN_TEST(test_engine_draw_hook_smoke);

    return UNITY_END();
}

#else // PIXELROOT32_ENABLE_SCENE_TRANSITIONS

void setUp(void) {}
void tearDown(void) {}

void test_transition_disabled(void) {
    TEST_IGNORE_MESSAGE("PIXELROOT32_ENABLE_SCENE_TRANSITIONS not defined");
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_transition_disabled);
    return UNITY_END();
}

#endif
