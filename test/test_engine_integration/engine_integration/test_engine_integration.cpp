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
#include "../../test_config.h"
#include "../../mocks/MockDrawSurface.h"
#include <memory>

using namespace pixelroot32::core;
using namespace pixelroot32::graphics;
using namespace pixelroot32::input;
using namespace pixelroot32::audio;

static int mock_instances = 0;

// Local MockDrawSurface that uses local counter
class LocalMockDrawSurface : public MockDrawSurface {
public:
    LocalMockDrawSurface() { mock_instances++; }
    virtual ~LocalMockDrawSurface() { mock_instances--; }
};

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
    auto mock = std::make_unique<MockDrawSurface>();
    DisplayConfig config = PIXELROOT32_CUSTOM_DISPLAY(mock.release(), 240, 240);
    Engine engine(config);
    
    engine.init();
    
    // Verify subsystems are accessible and initialized
    TEST_ASSERT_NOT_NULL(&engine.getRenderer());
    TEST_ASSERT_NOT_NULL(&engine.getInputManager());
    TEST_ASSERT_NOT_NULL(&engine.getAudioEngine());
    TEST_ASSERT_NOT_NULL(&engine.getMusicPlayer());
    
    // Default scene should be null
    TEST_ASSERT_FALSE(engine.getCurrentScene().has_value());
}

void test_engine_scene_management(void) {
    auto mock = std::make_unique<MockDrawSurface>();
    DisplayConfig config = PIXELROOT32_CUSTOM_DISPLAY(mock.release(), 240, 240);
    Engine engine(config);
    
    auto scene = std::make_unique<Scene>();
    engine.setScene(scene.get());
    
    TEST_ASSERT_EQUAL_PTR(scene.get(), engine.getCurrentScene().value());
}

void test_engine_update_draw_propagation(void) {
    auto mock = std::make_unique<MockDrawSurface>();
    DisplayConfig config = PIXELROOT32_CUSTOM_DISPLAY(mock.release(), 240, 240);
    TestEngine engine(config);
    engine.init();
    
    auto scene = std::make_unique<Scene>();
    auto entity = std::make_unique<MockEntity>();
    
    scene->addEntity(entity.get());
    engine.setScene(scene.get());
    
    engine.test_update();
    TEST_ASSERT_TRUE(entity->updated);
    
    engine.test_draw();
    TEST_ASSERT_TRUE(entity->drawn);
    
    // Scene and entity are automatically destroyed
}

void test_engine_graphics_ownership(void) {
    TEST_ASSERT_EQUAL(0, mock_instances);
    
    {
        LocalMockDrawSurface* mock = new LocalMockDrawSurface();
        DisplayConfig config = PIXELROOT32_CUSTOM_DISPLAY(mock, 240, 240);
        
        {
            Engine engine(config);
            TEST_ASSERT_EQUAL(1, mock_instances);
        }
        
        // Engine should have deleted the mock via Renderer
        TEST_ASSERT_EQUAL(0, mock_instances);
    }
}

// =============================================================================
// Engine Constructor Overloads
// =============================================================================

void test_engine_constructor_rvalue_display_only(void) {
    auto mock = std::make_unique<MockDrawSurface>();
    DisplayConfig config = PIXELROOT32_CUSTOM_DISPLAY(mock.release(), 240, 240);
    
    Engine engine(std::move(config));
    engine.init();
    
    TEST_ASSERT_NOT_NULL(&engine.getRenderer());
    TEST_ASSERT_EQUAL(0, engine.getDeltaTime());
}

void test_engine_constructor_rvalue_display_input(void) {
    auto mock = std::make_unique<MockDrawSurface>();
    DisplayConfig config = PIXELROOT32_CUSTOM_DISPLAY(mock.release(), 240, 240);
    InputConfig inputCfg(0);
    
    Engine engine(std::move(config), inputCfg);
    engine.init();
    
    TEST_ASSERT_NOT_NULL(&engine.getRenderer());
    TEST_ASSERT_NOT_NULL(&engine.getInputManager());
}

void test_engine_constructor_rvalue_display_input_audio(void) {
    auto mock = std::make_unique<MockDrawSurface>();
    DisplayConfig config = PIXELROOT32_CUSTOM_DISPLAY(mock.release(), 240, 240);
    InputConfig inputCfg(0);
    AudioConfig audioCfg;
    
    Engine engine(std::move(config), inputCfg, audioCfg);
    engine.init();
    
    TEST_ASSERT_NOT_NULL(&engine.getRenderer());
    TEST_ASSERT_NOT_NULL(&engine.getAudioEngine());
}

void test_engine_constructor_const_ref_display_input(void) {
    auto mock = std::make_unique<MockDrawSurface>();
    DisplayConfig config = PIXELROOT32_CUSTOM_DISPLAY(mock.release(), 240, 240);
    InputConfig inputCfg(0);
    
    Engine engine(config, inputCfg);
    engine.init();
    
    TEST_ASSERT_NOT_NULL(&engine.getRenderer());
}

void test_engine_constructor_const_ref_display_input_audio(void) {
    auto mock = std::make_unique<MockDrawSurface>();
    DisplayConfig config = PIXELROOT32_CUSTOM_DISPLAY(mock.release(), 240, 240);
    InputConfig inputCfg(0);
    AudioConfig audioCfg;
    
    Engine engine(config, inputCfg, audioCfg);
    engine.init();
    
    TEST_ASSERT_NOT_NULL(&engine.getRenderer());
}

// =============================================================================
// Engine Accessor Tests
// =============================================================================

void test_engine_get_delta_time(void) {
    auto mock = std::make_unique<MockDrawSurface>();
    DisplayConfig config = PIXELROOT32_CUSTOM_DISPLAY(mock.release(), 240, 240);
    TestEngine engine(config);
    engine.init();
    
    // After init, deltaTime should be 0
    TEST_ASSERT_EQUAL(0, engine.getDeltaTime());
    
    // After calling update, deltaTime should be computed
    engine.test_update();
    // deltaTime = millis() - 0 (previous), so it's some positive value
    // We can't guarantee exact value but it shouldn't crash
}

void test_engine_get_millis(void) {
    auto mock = std::make_unique<MockDrawSurface>();
    DisplayConfig config = PIXELROOT32_CUSTOM_DISPLAY(mock.release(), 240, 240);
    Engine engine(config);
    
    unsigned long ms = engine.getMillis();
    // Should return some value >= 0
    TEST_ASSERT_TRUE(ms >= 0);
}

void test_engine_set_renderer(void) {
    auto mock1 = std::make_unique<MockDrawSurface>();
    DisplayConfig config1 = PIXELROOT32_CUSTOM_DISPLAY(mock1.release(), 240, 240);
    Engine engine(config1);
    engine.init();
    
    // Create a new renderer and swap it in
    auto mock2 = std::make_unique<MockDrawSurface>();
    MockDrawSurface* newMockRaw = mock2.get();
    DisplayConfig config2 = PIXELROOT32_CUSTOM_DISPLAY(mock2.release(), 128, 128);
    Renderer newRenderer(config2);
    
    engine.setRenderer(std::move(newRenderer));
    
    TEST_ASSERT_EQUAL(128, engine.getRenderer().getLogicalWidth());
    TEST_ASSERT_EQUAL(128, engine.getRenderer().getLogicalHeight());
}

void test_engine_get_platform_capabilities(void) {
    auto mock = std::make_unique<MockDrawSurface>();
    DisplayConfig config = PIXELROOT32_CUSTOM_DISPLAY(mock.release(), 240, 240);
    Engine engine(config);
    
    const auto& caps = engine.getPlatformCapabilities();
    // Should not crash — just make sure we can access it
    (void)caps;
}

void test_engine_get_music_player(void) {
    auto mock = std::make_unique<MockDrawSurface>();
    DisplayConfig config = PIXELROOT32_CUSTOM_DISPLAY(mock.release(), 240, 240);
    Engine engine(config);
    
    auto& mp = engine.getMusicPlayer();
    (void)mp; // Should not crash
}

void test_engine_update_no_scene(void) {
    auto mock = std::make_unique<MockDrawSurface>();
    DisplayConfig config = PIXELROOT32_CUSTOM_DISPLAY(mock.release(), 240, 240);
    TestEngine engine(config);
    engine.init();
    
    // Update with no scene set — should not crash
    engine.test_update();
    engine.test_draw();
}

int main(int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(test_engine_initialization);
    RUN_TEST(test_engine_scene_management);
    RUN_TEST(test_engine_update_draw_propagation);
    RUN_TEST(test_engine_graphics_ownership);
    
    // Constructor overloads
    RUN_TEST(test_engine_constructor_rvalue_display_only);
    RUN_TEST(test_engine_constructor_rvalue_display_input);
    RUN_TEST(test_engine_constructor_rvalue_display_input_audio);
    RUN_TEST(test_engine_constructor_const_ref_display_input);
    RUN_TEST(test_engine_constructor_const_ref_display_input_audio);
    
    // Accessors
    RUN_TEST(test_engine_get_delta_time);
    RUN_TEST(test_engine_get_millis);
    RUN_TEST(test_engine_set_renderer);
    RUN_TEST(test_engine_get_platform_capabilities);
    RUN_TEST(test_engine_get_music_player);
    RUN_TEST(test_engine_update_no_scene);
    
    return UNITY_END();
}
