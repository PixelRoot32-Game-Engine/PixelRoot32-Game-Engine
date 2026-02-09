/**
 * @file test_game_loop.cpp
 * @brief End-to-end tests for a simplified game loop.
 */

#include <unity.h>
#include "core/Engine.h"
#include "core/Scene.h"
#include "core/Entity.h"
#include "graphics/DisplayConfig.h"
#include "graphics/Renderer.h"
#include "../test_config.h"

#ifdef PLATFORM_NATIVE
#include <SDL2/SDL.h>
#endif

using namespace pixelroot32::core;
using namespace pixelroot32::graphics;

void setUp(void) {
    // set stuff up here
}

void tearDown(void) {
    // clean stuff up here
}

// =============================================================================
// Mock Entities
// =============================================================================

class MovingEntity : public Entity {
public:
    float vx, vy;
    bool drawCalled = false;

    MovingEntity(float x, float y, float vx, float vy) 
        : Entity(x, y, 10, 10, EntityType::GENERIC), vx(vx), vy(vy) {
    }

    void update(unsigned long dt) override {
        // Move based on velocity and dt (in seconds)
        x += vx * (dt / 1000.0f);
        y += vy * (dt / 1000.0f);
    }

    void draw(Renderer& r) override {
        drawCalled = true;
    }
};

// =============================================================================
// Test Engine Subclass
// =============================================================================

class TestGameEngine : public Engine {
public:
    TestGameEngine(const DisplayConfig& dc) : Engine(dc) {}
    
    /**
     * @brief Manually triggers an update with a fixed delta time for deterministic testing.
     */
    void updateManual(unsigned long fixedDt) {
        deltaTime = fixedDt;
        // Don't call Engine::update() as it would recalculate deltaTime using millis()
        // Instead, manually update subsystems
#ifdef PLATFORM_NATIVE
        inputManager.update(deltaTime, nullptr);
#else
        inputManager.update(deltaTime);
#endif
        sceneManager.update(deltaTime);
    }

    /**
     * @brief Manually triggers a draw.
     */
    void drawManual() {
        draw();
    }
};

// =============================================================================
// Test Cases
// =============================================================================

/**
 * @brief Test basic movement in the game loop.
 */
void test_game_loop_movement(void) {
    DisplayConfig config(DisplayType::NONE, 0, 240, 240);
    TestGameEngine engine(config);
    engine.init();
    
    Scene* scene = new Scene();
    // 100 px/s to the right
    MovingEntity* entity = new MovingEntity(10, 10, 100.0f, 0.0f);
    scene->addEntity(entity);
    engine.setScene(scene);
    
    // Initial position
    TEST_ASSERT_FLOAT_EQUAL(10.0f, entity->x);
    
    // Step 1: 16ms (approx 60fps)
    engine.updateManual(16);
    // 10 + 100 * 0.016 = 11.6
    TEST_ASSERT_FLOAT_EQUAL(11.6f, entity->x);
    
    // Step 2: Another 16ms
    engine.updateManual(16);
    // 11.6 + 1.6 = 13.2
    TEST_ASSERT_FLOAT_EQUAL(13.2f, entity->x);
    
    // Step 3: A long jump (1 second)
    engine.updateManual(1000);
    // 13.2 + 100 = 113.2
    TEST_ASSERT_FLOAT_EQUAL(113.2f, entity->x);
    
    delete scene;
    delete entity;
}

/**
 * @brief Test that draw is called on entities during the loop.
 */
void test_game_loop_render_propagation(void) {
    DisplayConfig config(DisplayType::NONE, 0, 240, 240);
    TestGameEngine engine(config);
    engine.init();
    
    Scene* scene = new Scene();
    MovingEntity* entity = new MovingEntity(10, 10, 0, 0);
    scene->addEntity(entity);
    engine.setScene(scene);
    
    TEST_ASSERT_FALSE(entity->drawCalled);
    
    engine.drawManual();
    
    TEST_ASSERT_TRUE(entity->drawCalled);
    
    delete scene;
    delete entity;
}

/**
 * @brief Test scene transitions in the loop.
 */
void test_game_loop_scene_transition(void) {
    DisplayConfig config(DisplayType::NONE, 0, 240, 240);
    TestGameEngine engine(config);
    engine.init();
    
    Scene* scene1 = new Scene();
    MovingEntity* e1 = new MovingEntity(0, 0, 0, 0);
    scene1->addEntity(e1);
    
    Scene* scene2 = new Scene();
    MovingEntity* e2 = new MovingEntity(0, 0, 0, 0);
    scene2->addEntity(e2);
    
    engine.setScene(scene1);
    engine.updateManual(16);
    engine.drawManual();
    
    TEST_ASSERT_TRUE(e1->drawCalled);
    TEST_ASSERT_FALSE(e2->drawCalled);
    
    // Reset flags
    e1->drawCalled = false;
    
    // Switch scene
    engine.setScene(scene2);
    engine.updateManual(16);
    engine.drawManual();
    
    TEST_ASSERT_FALSE(e1->drawCalled);
    TEST_ASSERT_TRUE(e2->drawCalled);
    
    delete scene1;
    delete scene2;
    delete e1;
    delete e2;
}

// =============================================================================
// Main
// =============================================================================

int main(int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(test_game_loop_movement);
    RUN_TEST(test_game_loop_render_propagation);
    RUN_TEST(test_game_loop_scene_transition);
    return UNITY_END();
}
