/**
 * @file test_scene_manager.cpp
 * @brief Unit tests for core/SceneManager module
 * @version 1.1
 * @date 2026-02-08
 * 
 * Tests for SceneManager class including:
 * - Scene stack management (push/pop)
 * - Current scene tracking
 * - Update and draw propagation
 */

#include <unity.h>
#include <cstring>
#include "../../test_config.h"
#include "core/SceneManager.h"
#include "core/Scene.h"
#include "graphics/Renderer.h"
#include "platforms/EngineConfig.h"

using namespace pixelroot32::core;
using namespace pixelroot32::graphics;

// Mock Scene implementation
class MockScene : public Scene {
public:
    bool initCalled = false;
    bool updateCalled = false;
    bool drawCalled = false;
    unsigned long lastDeltaTime = 0;
    bool shouldRedraw = true;  // Default to true for aggregateShouldRedrawFramebuffer tests
    
    virtual ~MockScene() {}
    virtual void init() override { initCalled = true; }
    virtual void update(unsigned long dt) override { 
        updateCalled = true; 
        lastDeltaTime = dt;
    }
    virtual void draw(Renderer& renderer) override { 
        (void)renderer;
        drawCalled = true; 
    }
    virtual bool shouldRedrawFramebuffer() const override {
        return shouldRedraw;
    }
    void reset() {
        initCalled = false;
        updateCalled = false;
        drawCalled = false;
        lastDeltaTime = 0;
    }
};

void setUp(void) {
    test_setup();
}

void tearDown(void) {
    test_teardown();
}

// =============================================================================
// Tests for initialization
// =============================================================================

void test_scene_manager_initialization(void) {
    SceneManager manager;
    TEST_ASSERT_EQUAL_INT(0, manager.getSceneCount());
    TEST_ASSERT_TRUE(manager.isEmpty());
    TEST_ASSERT_FALSE(manager.getCurrentScene().has_value());
}

// =============================================================================
// Tests for setCurrentScene
// =============================================================================

void test_scene_manager_set_current_scene(void) {
    SceneManager manager;
    MockScene scene;
    
    manager.setCurrentScene(&scene);
    
    TEST_ASSERT_EQUAL_INT(1, manager.getSceneCount());
    TEST_ASSERT_TRUE(manager.getCurrentScene().has_value());
    TEST_ASSERT_EQUAL(&scene, manager.getCurrentScene().value());
    TEST_ASSERT_TRUE(scene.initCalled);
}

void test_scene_manager_set_current_clears_stack(void) {
    SceneManager manager;
    MockScene scene1;
    MockScene scene2;
    MockScene scene3;
    
    manager.pushScene(&scene1);
    manager.pushScene(&scene2);
    manager.setCurrentScene(&scene3);
    
    TEST_ASSERT_EQUAL_INT(1, manager.getSceneCount());
    TEST_ASSERT_TRUE(manager.getCurrentScene().has_value());
    TEST_ASSERT_EQUAL(&scene3, manager.getCurrentScene().value());
}

// =============================================================================
// Tests for pushScene
// =============================================================================

void test_scene_manager_push_scene(void) {
    SceneManager manager;
    MockScene scene;
    
    manager.pushScene(&scene);
    
    TEST_ASSERT_EQUAL_INT(1, manager.getSceneCount());
    TEST_ASSERT_TRUE(scene.initCalled);
}

void test_scene_manager_push_multiple(void) {
    SceneManager manager;
    MockScene scene1;
    MockScene scene2;
    MockScene scene3;
    
    manager.pushScene(&scene1);
    manager.pushScene(&scene2);
    manager.pushScene(&scene3);
    
    TEST_ASSERT_EQUAL_INT(3, manager.getSceneCount());
    TEST_ASSERT_TRUE(manager.getCurrentScene().has_value());
    TEST_ASSERT_EQUAL(&scene3, manager.getCurrentScene().value());
}

void test_scene_manager_push_beyond_max(void) {
    SceneManager manager;
    MockScene scenes[pixelroot32::platforms::config::MaxScenes + 1];
    
    for (int i = 0; i < pixelroot32::platforms::config::MaxScenes + 1; i++) {
        manager.pushScene(&scenes[i]);
    }
    
    TEST_ASSERT_EQUAL_INT(pixelroot32::platforms::config::MaxScenes, manager.getSceneCount());
}

// =============================================================================
// Tests for popScene
// =============================================================================

void test_scene_manager_pop_scene(void) {
    SceneManager manager;
    MockScene scene1;
    MockScene scene2;
    
    manager.pushScene(&scene1);
    manager.pushScene(&scene2);
    manager.popScene();
    
    TEST_ASSERT_EQUAL_INT(1, manager.getSceneCount());
    TEST_ASSERT_TRUE(manager.getCurrentScene().has_value());
    TEST_ASSERT_EQUAL(&scene1, manager.getCurrentScene().value());
}

void test_scene_manager_pop_to_empty(void) {
    SceneManager manager;
    MockScene scene;
    
    manager.pushScene(&scene);
    manager.popScene();
    
    TEST_ASSERT_EQUAL_INT(0, manager.getSceneCount());
    TEST_ASSERT_FALSE(manager.getCurrentScene().has_value());
}

void test_scene_manager_pop_empty_stack(void) {
    SceneManager manager;
    
    manager.popScene();  // Should not crash
    
    TEST_ASSERT_EQUAL_INT(0, manager.getSceneCount());
}

// =============================================================================
// Tests for update and draw
// =============================================================================

void test_scene_manager_update(void) {
    SceneManager manager;
    MockScene scene;
    unsigned long dt = 16;
    
    manager.pushScene(&scene);
    manager.update(dt);
    
    TEST_ASSERT_TRUE(scene.updateCalled);
    TEST_ASSERT_EQUAL_UINT32(dt, scene.lastDeltaTime);
}

void test_scene_manager_draw(void) {
    SceneManager manager;
    MockScene scene1;
    MockScene scene2;
    DisplayConfig config(DisplayType::NONE, 0, 240, 240);
    Renderer renderer(config);
    
    manager.pushScene(&scene1);
    manager.pushScene(&scene2);
    manager.draw(renderer);
    
    TEST_ASSERT_TRUE(scene1.drawCalled);
    TEST_ASSERT_TRUE(scene2.drawCalled);
}

// =============================================================================
// FASE 2 coverage expansion tests
// =============================================================================

void test_scene_manager_aggregate_should_redraw_framebuffer_empty_stack(void) {
    // Test empty stack - should return true (needs redraw)
    SceneManager manager;
    TEST_ASSERT_TRUE(manager.aggregateShouldRedrawFramebuffer());
}

void test_scene_manager_aggregate_should_redraw_framebuffer_with_scene(void) {
    // Test stack with scene that needs redraw
    SceneManager manager;
    MockScene scene;
    scene.shouldRedraw = true;
    manager.pushScene(&scene);
    
    TEST_ASSERT_TRUE(manager.aggregateShouldRedrawFramebuffer());
}

void test_scene_manager_aggregate_should_redraw_framebuffer_no_redraw_needed(void) {
    // Test stack where no scene needs redraw
    SceneManager manager;
    MockScene scene;
    scene.shouldRedraw = false;
    manager.pushScene(&scene);
    
    TEST_ASSERT_FALSE(manager.aggregateShouldRedrawFramebuffer());
}

void test_scene_manager_aggregate_should_redraw_multiple_scenes(void) {
    // Test with multiple scenes - one needs redraw
    SceneManager manager;
    MockScene scene1;
    MockScene scene2;
    scene1.shouldRedraw = false;
    scene2.shouldRedraw = true;
    manager.pushScene(&scene1);
    manager.pushScene(&scene2);
    
    // Should return true if any scene needs redraw
    TEST_ASSERT_TRUE(manager.aggregateShouldRedrawFramebuffer());
}

void test_scene_manager_aggregate_no_redraw_when_none_need_it(void) {
    // Test with multiple scenes - none need redraw
    SceneManager manager;
    MockScene scene1;
    MockScene scene2;
    scene1.shouldRedraw = false;
    scene2.shouldRedraw = false;
    manager.pushScene(&scene1);
    manager.pushScene(&scene2);
    
    TEST_ASSERT_FALSE(manager.aggregateShouldRedrawFramebuffer());
}

void test_scene_manager_draw_empty_stack(void) {
    // Test drawing with empty stack - should not crash
    SceneManager manager;
    DisplayConfig config(DisplayType::NONE, 0, 240, 240);
    Renderer renderer(config);
    
    manager.draw(renderer);
    // Verify manager is still functional - scene count should be 0
    TEST_ASSERT_EQUAL_INT(0, manager.getSceneCount());
}

void test_scene_manager_push_scene_at_max_boundary(void) {
    // Test push at MaxScenes boundary - last valid push
    SceneManager manager;
    MockScene scenes[pixelroot32::platforms::config::MaxScenes];
    
    // Push exactly MaxScenes scenes
    for (int i = 0; i < pixelroot32::platforms::config::MaxScenes; i++) {
        manager.pushScene(&scenes[i]);
    }
    
    TEST_ASSERT_EQUAL_INT(pixelroot32::platforms::config::MaxScenes, manager.getSceneCount());
    
    // Push one more - should be ignored
    MockScene extraScene;
    manager.pushScene(&extraScene);
    
    // Count should remain at MaxScenes
    TEST_ASSERT_EQUAL_INT(pixelroot32::platforms::config::MaxScenes, manager.getSceneCount());
}

void test_scene_manager_set_current_scene_clears_existing_stack(void) {
    // Test that setCurrentScene clears the existing stack
    SceneManager manager;
    MockScene scene1;
    MockScene scene2;
    MockScene scene3;
    
    // Push multiple scenes
    manager.pushScene(&scene1);
    manager.pushScene(&scene2);
    TEST_ASSERT_EQUAL_INT(2, manager.getSceneCount());
    
    // Replace with new scene - should clear stack
    manager.setCurrentScene(&scene3);
    
    TEST_ASSERT_EQUAL_INT(1, manager.getSceneCount());
    TEST_ASSERT_EQUAL(&scene3, manager.getCurrentScene().value());
}

int main(int argc, char **argv) {
    UNITY_BEGIN();
    
    RUN_TEST(test_scene_manager_initialization);
    RUN_TEST(test_scene_manager_set_current_scene);
    RUN_TEST(test_scene_manager_set_current_clears_stack);
    RUN_TEST(test_scene_manager_push_scene);
    RUN_TEST(test_scene_manager_push_multiple);
    RUN_TEST(test_scene_manager_push_beyond_max);
    RUN_TEST(test_scene_manager_pop_scene);
    RUN_TEST(test_scene_manager_pop_to_empty);
    RUN_TEST(test_scene_manager_pop_empty_stack);
    RUN_TEST(test_scene_manager_update);
    RUN_TEST(test_scene_manager_draw);
    
    // FASE 2 coverage expansion tests
    RUN_TEST(test_scene_manager_aggregate_should_redraw_framebuffer_empty_stack);
    RUN_TEST(test_scene_manager_aggregate_should_redraw_framebuffer_with_scene);
    RUN_TEST(test_scene_manager_aggregate_should_redraw_framebuffer_no_redraw_needed);
    RUN_TEST(test_scene_manager_aggregate_should_redraw_multiple_scenes);
    RUN_TEST(test_scene_manager_aggregate_no_redraw_when_none_need_it);
    RUN_TEST(test_scene_manager_draw_empty_stack);
    RUN_TEST(test_scene_manager_push_scene_at_max_boundary);
    RUN_TEST(test_scene_manager_set_current_scene_clears_existing_stack);
    
    return UNITY_END();
}
