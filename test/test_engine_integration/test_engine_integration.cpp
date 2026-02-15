/**
 * @file test_engine_integration.cpp
 * @brief Integration tests for the Engine module and its subsystems.
 */

#include <unity.h>
#include "core/Engine.h"
#include "core/Scene.h"
#include "core/Entity.h"
#include "graphics/DisplayConfig.h"
#include "graphics/Renderer.h"
#include "input/InputConfig.h"
#include "audio/AudioConfig.h"
#include "../test_config.h"
#include "../mocks/MockDrawSurface.h"

using namespace pixelroot32::core;
using namespace pixelroot32::graphics;
using namespace pixelroot32::input;
using namespace pixelroot32::audio;

int MockDrawSurface::instances = 0;

void setUp(void) {
    // set stuff up here
}

void tearDown(void) {
    // clean stuff up here
}

// Mock Entity to track calls
class MockEntity : public Entity {
public:
    bool updated = false;
    bool drawn = false;
    unsigned long lastDt = 0;

    MockEntity() : Entity(0, 0, 10, 10, EntityType::GENERIC) {
        isVisible = true;
        isEnabled = true;
    }

    void update(unsigned long dt) override {
        updated = true;
        lastDt = dt;
    }

    void draw(Renderer& r) override {
        drawn = true;
    }
};

// Subclass Engine to access protected methods for testing
class TestEngine : public Engine {
public:
    TestEngine(const DisplayConfig& dc) : Engine(dc) {}
    
    void test_update() { update(); }
    void test_draw() { draw(); }
};

void test_engine_initialization(void) {
    // Use custom display to verify driver injection
    MockDrawSurface* mock = new MockDrawSurface();
    DisplayConfig config = PIXELROOT32_CUSTOM_DISPLAY(mock, 240, 240);
    Engine engine(config);
    
    engine.init();
    
    // Verify subsystems are accessible and initialized
    TEST_ASSERT_NOT_NULL(&engine.getRenderer());
    TEST_ASSERT_NOT_NULL(&engine.getInputManager());
    TEST_ASSERT_NOT_NULL(&engine.getAudioEngine());
    TEST_ASSERT_NOT_NULL(&engine.getMusicPlayer());
    
    // Default scene should be null
    TEST_ASSERT_NULL(engine.getCurrentScene());
}

void test_engine_scene_management(void) {
    MockDrawSurface* mock = new MockDrawSurface();
    DisplayConfig config = PIXELROOT32_CUSTOM_DISPLAY(mock, 240, 240);
    Engine engine(config);
    
    Scene* scene = new Scene();
    engine.setScene(scene);
    
    TEST_ASSERT_EQUAL_PTR(scene, engine.getCurrentScene());
    
    delete scene;
}

void test_engine_update_draw_propagation(void) {
    MockDrawSurface* mock = new MockDrawSurface();
    DisplayConfig config = PIXELROOT32_CUSTOM_DISPLAY(mock, 240, 240);
    TestEngine engine(config);
    engine.init();
    
    Scene* scene = new Scene();
    MockEntity* entity = new MockEntity();
    scene->addEntity(entity);
    
    engine.setScene(scene);
    
    // Test update propagation
    engine.test_update();
    TEST_ASSERT_TRUE(entity->updated);
    
    // Test draw propagation
    engine.test_draw();
    TEST_ASSERT_TRUE(entity->drawn);
    
    delete scene;
    delete entity;
}

void test_engine_graphics_ownership(void) {
    TEST_ASSERT_EQUAL(0, MockDrawSurface::instances);
    
    {
        MockDrawSurface* mock = new MockDrawSurface();
        DisplayConfig config = PIXELROOT32_CUSTOM_DISPLAY(mock, 240, 240);
        
        {
            Engine engine(config);
            TEST_ASSERT_EQUAL(1, MockDrawSurface::instances);
        }
        
        // Engine should have deleted the mock via Renderer
        TEST_ASSERT_EQUAL(0, MockDrawSurface::instances);
    }
}

int main(int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(test_engine_initialization);
    RUN_TEST(test_engine_scene_management);
    RUN_TEST(test_engine_update_draw_propagation);
    RUN_TEST(test_engine_graphics_ownership);
    return UNITY_END();
}
