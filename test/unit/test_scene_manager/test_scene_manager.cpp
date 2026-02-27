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

using namespace pixelroot32::core;
using namespace pixelroot32::graphics;

// Mock Scene implementation
class MockScene : public Scene {
public:
    bool initCalled = false;
    bool updateCalled = false;
    bool drawCalled = false;
    unsigned long lastDeltaTime = 0;
    
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
    MockScene scenes[MAX_SCENES + 1];
    
    for (int i = 0; i < MAX_SCENES + 1; i++) {
        manager.pushScene(&scenes[i]);
    }
    
    TEST_ASSERT_EQUAL_INT(MAX_SCENES, manager.getSceneCount());
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
    
    return UNITY_END();
}
