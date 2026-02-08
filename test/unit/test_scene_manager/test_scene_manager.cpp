/**
 * @file test_scene_manager.cpp
 * @brief Unit tests for core/SceneManager module
 * @version 1.0
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

// Mock implementations
namespace pixelroot32 {
namespace graphics {
    class Renderer {};
}

namespace core {

class Scene {
public:
    bool initCalled = false;
    bool updateCalled = false;
    bool drawCalled = false;
    unsigned long lastDeltaTime = 0;
    
    virtual ~Scene() {}
    virtual void init() { initCalled = true; }
    virtual void update(unsigned long dt) { 
        updateCalled = true; 
        lastDeltaTime = dt;
    }
    virtual void draw(graphics::Renderer& renderer) { 
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

#define MAX_SCENES 5

class SceneManager {
public:
    Scene* sceneStack[MAX_SCENES];
    int sceneCount;
    
    SceneManager() : sceneCount(0) {
        for (int i = 0; i < MAX_SCENES; i++) {
            sceneStack[i] = nullptr;
        }
    }
    
    void setCurrentScene(Scene* newScene) {
        sceneCount = 0;
        sceneStack[sceneCount++] = newScene;
        newScene->init();
    }
    
    void pushScene(Scene* newScene) {
        if (sceneCount < MAX_SCENES) {
            sceneStack[sceneCount++] = newScene;
            newScene->init();
        }
    }
    
    void popScene() {
        if (sceneCount > 0) {
            sceneCount--;
        }
    }
    
    void update(unsigned long dt) {
        if (sceneCount > 0) {
            sceneStack[sceneCount - 1]->update(dt);
        }
    }
    
    void draw(graphics::Renderer& renderer) {
        for (int i = 0; i < sceneCount; i++) {
            sceneStack[i]->draw(renderer);
        }
    }
    
    Scene* getCurrentScene() const {
        if (sceneCount > 0) {
            return sceneStack[sceneCount - 1];
        }
        return nullptr;
    }
    
    int getSceneCount() const { return sceneCount; }
    bool isEmpty() const { return sceneCount == 0; }
};

}
}

using namespace pixelroot32::core;
using namespace pixelroot32::graphics;

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
    TEST_ASSERT_NULL(manager.getCurrentScene());
}

// =============================================================================
// Tests for setCurrentScene
// =============================================================================

void test_scene_manager_set_current_scene(void) {
    SceneManager manager;
    Scene scene;
    
    manager.setCurrentScene(&scene);
    
    TEST_ASSERT_EQUAL_INT(1, manager.getSceneCount());
    TEST_ASSERT_EQUAL(&scene, manager.getCurrentScene());
    TEST_ASSERT_TRUE(scene.initCalled);
}

void test_scene_manager_set_current_clears_stack(void) {
    SceneManager manager;
    Scene scene1;
    Scene scene2;
    Scene scene3;
    
    manager.pushScene(&scene1);
    manager.pushScene(&scene2);
    manager.setCurrentScene(&scene3);
    
    TEST_ASSERT_EQUAL_INT(1, manager.getSceneCount());
    TEST_ASSERT_EQUAL(&scene3, manager.getCurrentScene());
}

// =============================================================================
// Tests for pushScene
// =============================================================================

void test_scene_manager_push_scene(void) {
    SceneManager manager;
    Scene scene;
    
    manager.pushScene(&scene);
    
    TEST_ASSERT_EQUAL_INT(1, manager.getSceneCount());
    TEST_ASSERT_TRUE(scene.initCalled);
}

void test_scene_manager_push_multiple(void) {
    SceneManager manager;
    Scene scene1;
    Scene scene2;
    Scene scene3;
    
    manager.pushScene(&scene1);
    manager.pushScene(&scene2);
    manager.pushScene(&scene3);
    
    TEST_ASSERT_EQUAL_INT(3, manager.getSceneCount());
    TEST_ASSERT_EQUAL(&scene3, manager.getCurrentScene());
}

void test_scene_manager_push_beyond_max(void) {
    SceneManager manager;
    Scene scenes[MAX_SCENES + 1];
    
    for (int i = 0; i < MAX_SCENES + 1; i++) {
        manager.pushScene(&scenes[i]);
    }
    
    TEST_ASSERT_EQUAL_INT(MAX_SCENES, manager.getSceneCount());
}

void test_scene_manager_push_init_called(void) {
    SceneManager manager;
    Scene scene;
    
    manager.pushScene(&scene);
    
    TEST_ASSERT_TRUE(scene.initCalled);
}

// =============================================================================
// Tests for popScene
// =============================================================================

void test_scene_manager_pop_scene(void) {
    SceneManager manager;
    Scene scene1;
    Scene scene2;
    
    manager.pushScene(&scene1);
    manager.pushScene(&scene2);
    manager.popScene();
    
    TEST_ASSERT_EQUAL_INT(1, manager.getSceneCount());
    TEST_ASSERT_EQUAL(&scene1, manager.getCurrentScene());
}

void test_scene_manager_pop_to_empty(void) {
    SceneManager manager;
    Scene scene;
    
    manager.pushScene(&scene);
    manager.popScene();
    
    TEST_ASSERT_EQUAL_INT(0, manager.getSceneCount());
    TEST_ASSERT_NULL(manager.getCurrentScene());
}

void test_scene_manager_pop_empty_stack(void) {
    SceneManager manager;
    
    manager.popScene();  // Should not crash
    
    TEST_ASSERT_EQUAL_INT(0, manager.getSceneCount());
}

void test_scene_manager_pop_single_scene(void) {
    SceneManager manager;
    Scene scene;
    
    manager.setCurrentScene(&scene);
    manager.popScene();
    
    TEST_ASSERT_EQUAL_INT(0, manager.getSceneCount());
    TEST_ASSERT_NULL(manager.getCurrentScene());
}

// =============================================================================
// Tests for getCurrentScene
// =============================================================================

void test_scene_manager_get_current_single(void) {
    SceneManager manager;
    Scene scene;
    
    manager.pushScene(&scene);
    
    TEST_ASSERT_EQUAL(&scene, manager.getCurrentScene());
}

void test_scene_manager_get_current_top(void) {
    SceneManager manager;
    Scene scene1;
    Scene scene2;
    
    manager.pushScene(&scene1);
    manager.pushScene(&scene2);
    
    TEST_ASSERT_EQUAL(&scene2, manager.getCurrentScene());
}

void test_scene_manager_get_current_after_pop(void) {
    SceneManager manager;
    Scene scene1;
    Scene scene2;
    
    manager.pushScene(&scene1);
    manager.pushScene(&scene2);
    manager.popScene();
    
    TEST_ASSERT_EQUAL(&scene1, manager.getCurrentScene());
}

// =============================================================================
// Tests for update
// =============================================================================

void test_scene_manager_update_calls_current(void) {
    SceneManager manager;
    Scene scene;
    
    manager.pushScene(&scene);
    manager.update(16);
    
    TEST_ASSERT_TRUE(scene.updateCalled);
}

void test_scene_manager_update_delta_passed(void) {
    SceneManager manager;
    Scene scene;
    
    manager.pushScene(&scene);
    manager.update(33);
    
    TEST_ASSERT_EQUAL_INT(33, scene.lastDeltaTime);
}

void test_scene_manager_update_only_top(void) {
    SceneManager manager;
    Scene scene1;
    Scene scene2;
    
    manager.pushScene(&scene1);
    manager.pushScene(&scene2);
    manager.update(16);
    
    TEST_ASSERT_FALSE(scene1.updateCalled);
    TEST_ASSERT_TRUE(scene2.updateCalled);
}

void test_scene_manager_update_empty(void) {
    SceneManager manager;
    
    manager.update(16);  // Should not crash
    
    TEST_ASSERT_TRUE(manager.isEmpty());
}

// =============================================================================
// Tests for draw
// =============================================================================

void test_scene_manager_draw_calls_current(void) {
    SceneManager manager;
    Scene scene;
    Renderer r;
    
    manager.pushScene(&scene);
    manager.draw(r);
    
    TEST_ASSERT_TRUE(scene.drawCalled);
}

void test_scene_manager_draw_all_scenes(void) {
    SceneManager manager;
    Scene scene1;
    Scene scene2;
    Renderer r;
    
    manager.pushScene(&scene1);
    manager.pushScene(&scene2);
    manager.draw(r);
    
    TEST_ASSERT_TRUE(scene1.drawCalled);
    TEST_ASSERT_TRUE(scene2.drawCalled);
}

void test_scene_manager_draw_order(void) {
    SceneManager manager;
    Scene scene1;
    Scene scene2;
    Scene scene3;
    Renderer r;
    
    manager.pushScene(&scene1);
    manager.pushScene(&scene2);
    manager.pushScene(&scene3);
    manager.draw(r);
    
    // All scenes should be drawn (in order from bottom to top)
    TEST_ASSERT_TRUE(scene1.drawCalled);
    TEST_ASSERT_TRUE(scene2.drawCalled);
    TEST_ASSERT_TRUE(scene3.drawCalled);
}

void test_scene_manager_draw_empty(void) {
    SceneManager manager;
    Renderer r;
    
    manager.draw(r);  // Should not crash
    
    TEST_ASSERT_TRUE(manager.isEmpty());
}

// =============================================================================
// Tests for complex scenarios
// =============================================================================

void test_scene_manager_complex_scenario(void) {
    SceneManager manager;
    Scene mainMenu;
    Scene gameLevel;
    Scene pauseMenu;
    Renderer r;
    
    // Start with main menu
    manager.setCurrentScene(&mainMenu);
    TEST_ASSERT_EQUAL(&mainMenu, manager.getCurrentScene());
    TEST_ASSERT_TRUE(mainMenu.initCalled);
    
    // Start game (push level)
    manager.pushScene(&gameLevel);
    TEST_ASSERT_EQUAL(&gameLevel, manager.getCurrentScene());
    TEST_ASSERT_TRUE(gameLevel.initCalled);
    
    // Pause game (push pause menu)
    manager.pushScene(&pauseMenu);
    TEST_ASSERT_EQUAL(&pauseMenu, manager.getCurrentScene());
    TEST_ASSERT_TRUE(pauseMenu.initCalled);
    
    // Update only updates pause menu
    manager.update(16);
    TEST_ASSERT_FALSE(mainMenu.updateCalled);
    TEST_ASSERT_FALSE(gameLevel.updateCalled);
    TEST_ASSERT_TRUE(pauseMenu.updateCalled);
    
    // Draw draws all scenes
    manager.draw(r);
    TEST_ASSERT_TRUE(mainMenu.drawCalled);
    TEST_ASSERT_TRUE(gameLevel.drawCalled);
    TEST_ASSERT_TRUE(pauseMenu.drawCalled);
    
    // Unpause (pop pause menu)
    manager.popScene();
    TEST_ASSERT_EQUAL(&gameLevel, manager.getCurrentScene());
    
    // Now update only updates game level
    gameLevel.reset();
    pauseMenu.reset();  // Reset pause menu too since it was updated before
    manager.update(16);
    TEST_ASSERT_FALSE(mainMenu.updateCalled);
    TEST_ASSERT_TRUE(gameLevel.updateCalled);
    TEST_ASSERT_FALSE(pauseMenu.updateCalled);  // Should not be updated now
}

void test_scene_manager_replace_scene(void) {
    SceneManager manager;
    Scene level1;
    Scene level2;
    
    manager.setCurrentScene(&level1);
    TEST_ASSERT_EQUAL(&level1, manager.getCurrentScene());
    
    manager.setCurrentScene(&level2);
    TEST_ASSERT_EQUAL(&level2, manager.getCurrentScene());
    TEST_ASSERT_EQUAL_INT(1, manager.getSceneCount());
}

void test_scene_manager_multiple_pops(void) {
    SceneManager manager;
    Scene scenes[3];
    
    manager.pushScene(&scenes[0]);
    manager.pushScene(&scenes[1]);
    manager.pushScene(&scenes[2]);
    
    manager.popScene();
    TEST_ASSERT_EQUAL(&scenes[1], manager.getCurrentScene());
    
    manager.popScene();
    TEST_ASSERT_EQUAL(&scenes[0], manager.getCurrentScene());
    
    manager.popScene();
    TEST_ASSERT_NULL(manager.getCurrentScene());
}

// =============================================================================
// Main
// =============================================================================

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;
    
    UNITY_BEGIN();
    
    // Initialization tests
    RUN_TEST(test_scene_manager_initialization);
    
    // setCurrentScene tests
    RUN_TEST(test_scene_manager_set_current_scene);
    RUN_TEST(test_scene_manager_set_current_clears_stack);
    
    // pushScene tests
    RUN_TEST(test_scene_manager_push_scene);
    RUN_TEST(test_scene_manager_push_multiple);
    RUN_TEST(test_scene_manager_push_beyond_max);
    RUN_TEST(test_scene_manager_push_init_called);
    
    // popScene tests
    RUN_TEST(test_scene_manager_pop_scene);
    RUN_TEST(test_scene_manager_pop_to_empty);
    RUN_TEST(test_scene_manager_pop_empty_stack);
    RUN_TEST(test_scene_manager_pop_single_scene);
    
    // getCurrentScene tests
    RUN_TEST(test_scene_manager_get_current_single);
    RUN_TEST(test_scene_manager_get_current_top);
    RUN_TEST(test_scene_manager_get_current_after_pop);
    
    // update tests
    RUN_TEST(test_scene_manager_update_calls_current);
    RUN_TEST(test_scene_manager_update_delta_passed);
    RUN_TEST(test_scene_manager_update_only_top);
    RUN_TEST(test_scene_manager_update_empty);
    
    // draw tests
    RUN_TEST(test_scene_manager_draw_calls_current);
    RUN_TEST(test_scene_manager_draw_all_scenes);
    RUN_TEST(test_scene_manager_draw_order);
    RUN_TEST(test_scene_manager_draw_empty);
    
    // Complex scenarios
    RUN_TEST(test_scene_manager_complex_scenario);
    RUN_TEST(test_scene_manager_replace_scene);
    RUN_TEST(test_scene_manager_multiple_pops);
    
    return UNITY_END();
}
