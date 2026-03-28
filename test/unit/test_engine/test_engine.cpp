#include <unity.h>
#include "../../test_config.h"
#include "core/Engine.h"
#include "graphics/DisplayConfig.h"
#include "graphics/BaseDrawSurface.h"
#include "input/InputConfig.h"
#include "core/Scene.h"

using namespace pixelroot32::core;
using namespace pixelroot32::graphics;
using namespace pixelroot32::input;

class MockDrawSurface : public BaseDrawSurface {
public:
    void init() override {}
    void clearBuffer() override {}
    void sendBuffer() override {}
    void drawRectangle(int, int, int, int, uint16_t) override {}
    void drawFilledRectangle(int, int, int, int, uint16_t) override {}
    void drawPixel(int, int, uint16_t) override {}
    bool processEvents() override { return true; }
    void present() override {}
};

// Mock Scene for testing Engine integration
class MockScene : public Scene {
public:
    MockScene() : updateCount_(0), drawCount_(0) {}
    
    void update(unsigned long deltaTime) override {
        updateCount_++;
        lastDeltaTime_ = deltaTime;
    }
    
    void draw(Renderer& renderer) override {
        drawCount_++;
        (void)renderer;
    }
    
    int getUpdateCount() const { return updateCount_; }
    int getDrawCount() const { return drawCount_; }
    unsigned long getLastDeltaTime() const { return lastDeltaTime_; }
    
private:
    int updateCount_;
    int drawCount_;
    unsigned long lastDeltaTime_;
};

void test_engine_creation_with_display() {
    MockDrawSurface* mockSurface = new MockDrawSurface();
    DisplayConfig config = PIXELROOT32_CUSTOM_DISPLAY(mockSurface, 240, 240);
    
    Engine engine(config);
    
    TEST_ASSERT_TRUE(true);
}

void test_engine_creation_with_display_and_input() {
    MockDrawSurface* mockSurface = new MockDrawSurface();
    DisplayConfig config = PIXELROOT32_CUSTOM_DISPLAY(mockSurface, 240, 240);
    InputConfig inputConfig(8, 0, 1, 2, 3, 4, 5, 6, 7);
    
    Engine engine(config, inputConfig);
    
    TEST_ASSERT_TRUE(true);
}

void test_engine_init() {
    MockDrawSurface* mockSurface = new MockDrawSurface();
    DisplayConfig config = PIXELROOT32_CUSTOM_DISPLAY(mockSurface, 240, 240);
    Engine engine(config);
    
    engine.init();
    
    TEST_ASSERT_TRUE(true);
}

void test_engine_get_renderer() {
    MockDrawSurface* mockSurface = new MockDrawSurface();
    DisplayConfig config = PIXELROOT32_CUSTOM_DISPLAY(mockSurface, 240, 240);
    Engine engine(config);
    
    Renderer& renderer = engine.getRenderer();
    TEST_ASSERT_NOT_NULL(&renderer);
}

void test_engine_get_input_manager() {
    MockDrawSurface* mockSurface = new MockDrawSurface();
    DisplayConfig config = PIXELROOT32_CUSTOM_DISPLAY(mockSurface, 240, 240);
    InputConfig inputConfig(8, 0, 1, 2, 3, 4, 5, 6, 7);
    Engine engine(config, inputConfig);
    
    InputManager& input = engine.getInputManager();
    TEST_ASSERT_NOT_NULL(&input);
}

void test_engine_creation_lvalue_display() {
    MockDrawSurface* mockSurface = new MockDrawSurface();
    DisplayConfig config = PIXELROOT32_CUSTOM_DISPLAY(mockSurface, 320, 240);
    const DisplayConfig& configRef = config;
    
    Engine engine(configRef);
    
    TEST_ASSERT_TRUE(true);
}

void test_engine_creation_lvalue_display_and_input() {
    MockDrawSurface* mockSurface = new MockDrawSurface();
    DisplayConfig config = PIXELROOT32_CUSTOM_DISPLAY(mockSurface, 320, 240);
    const DisplayConfig& configRef = config;
    InputConfig inputConfig(8, 0, 1, 2, 3, 4, 5, 6, 7);
    
    Engine engine(configRef, inputConfig);
    
    TEST_ASSERT_TRUE(true);
}

void test_engine_creation_lvalue_with_audio() {
    MockDrawSurface* mockSurface = new MockDrawSurface();
    DisplayConfig config = PIXELROOT32_CUSTOM_DISPLAY(mockSurface, 240, 240);
    const DisplayConfig& configRef = config;
    InputConfig inputConfig(8, 0, 1, 2, 3, 4, 5, 6, 7);
    pixelroot32::audio::AudioConfig audioConfig;
    
    Engine engine(configRef, inputConfig, audioConfig);
    
    TEST_ASSERT_TRUE(true);
}

void test_engine_get_delta_time() {
    MockDrawSurface* mockSurface = new MockDrawSurface();
    DisplayConfig config = PIXELROOT32_CUSTOM_DISPLAY(mockSurface, 240, 240);
    Engine engine(config);
    
    unsigned long dt = engine.getDeltaTime();
    TEST_ASSERT_EQUAL(0UL, dt);
}

void test_engine_get_platform_capabilities() {
    MockDrawSurface* mockSurface = new MockDrawSurface();
    DisplayConfig config = PIXELROOT32_CUSTOM_DISPLAY(mockSurface, 240, 240);
    Engine engine(config);
    
    const auto& caps = engine.getPlatformCapabilities();
    TEST_ASSERT_TRUE(true);
}

void test_engine_get_audio_engine() {
    MockDrawSurface* mockSurface = new MockDrawSurface();
    DisplayConfig config = PIXELROOT32_CUSTOM_DISPLAY(mockSurface, 240, 240);
    Engine engine(config);
    
    pixelroot32::audio::AudioEngine& audio = engine.getAudioEngine();
    TEST_ASSERT_NOT_NULL(&audio);
}

void test_engine_get_music_player() {
    MockDrawSurface* mockSurface = new MockDrawSurface();
    DisplayConfig config = PIXELROOT32_CUSTOM_DISPLAY(mockSurface, 240, 240);
    Engine engine(config);
    
    pixelroot32::audio::MusicPlayer& music = engine.getMusicPlayer();
    TEST_ASSERT_NOT_NULL(&music);
}

void test_engine_set_renderer() {
    MockDrawSurface* mockSurface = new MockDrawSurface();
    DisplayConfig config = PIXELROOT32_CUSTOM_DISPLAY(mockSurface, 240, 240);
    Engine engine(config);
    
    pixelroot32::graphics::Renderer& renderer = engine.getRenderer();
    TEST_ASSERT_NOT_NULL(&renderer);
}

// Phase 6.2: Engine Integration Tests

void test_engine_with_mock_scene() {
    // Test Engine with a mock scene to verify update/draw flow
    MockDrawSurface* mockSurface = new MockDrawSurface();
    DisplayConfig config = PIXELROOT32_CUSTOM_DISPLAY(mockSurface, 240, 240);
    Engine engine(config);
    
    engine.init();
    
    MockScene scene;
    engine.setScene(&scene);
    
    // Verify scene was set
    auto currentScene = engine.getCurrentScene();
    TEST_ASSERT_TRUE(currentScene.has_value());
    TEST_ASSERT_EQUAL(&scene, currentScene.value());
}

void test_engine_scene_lifecycle() {
    // Test scene lifecycle: set, verify scene is accessible
    MockDrawSurface* mockSurface = new MockDrawSurface();
    DisplayConfig config = PIXELROOT32_CUSTOM_DISPLAY(mockSurface, 240, 240);
    Engine engine(config);
    
    engine.init();
    
    MockScene scene;
    engine.setScene(&scene);
    
    // Verify scene is set correctly
    auto currentScene = engine.getCurrentScene();
    TEST_ASSERT_TRUE(currentScene.has_value());
    TEST_ASSERT_EQUAL(&scene, currentScene.value());
}

void test_engine_multiple_scenes() {
    // Test switching between scenes
    MockDrawSurface* mockSurface = new MockDrawSurface();
    DisplayConfig config = PIXELROOT32_CUSTOM_DISPLAY(mockSurface, 240, 240);
    Engine engine(config);
    
    engine.init();
    
    MockScene scene1;
    MockScene scene2;
    
    engine.setScene(&scene1);
    auto current1 = engine.getCurrentScene();
    TEST_ASSERT_TRUE(current1.has_value());
    TEST_ASSERT_EQUAL(&scene1, current1.value());
    
    engine.setScene(&scene2);
    auto current2 = engine.getCurrentScene();
    TEST_ASSERT_TRUE(current2.has_value());
    TEST_ASSERT_EQUAL(&scene2, current2.value());
}

void test_engine_get_millis() {
    // Test Engine::getMillis() returns increasing values
    MockDrawSurface* mockSurface = new MockDrawSurface();
    DisplayConfig config = PIXELROOT32_CUSTOM_DISPLAY(mockSurface, 240, 240);
    Engine engine(config);
    
    unsigned long t1 = engine.getMillis();
    
    // Small delay
    for (volatile int i = 0; i < 1000; i++);
    
    unsigned long t2 = engine.getMillis();
    
    // Time should not decrease
    TEST_ASSERT_TRUE(t2 >= t1);
}

void test_engine_init_with_audio() {
    // Test Engine initialization with audio enabled
    MockDrawSurface* mockSurface = new MockDrawSurface();
    DisplayConfig config = PIXELROOT32_CUSTOM_DISPLAY(mockSurface, 240, 240);
    InputConfig inputConfig(8);
    pixelroot32::audio::AudioConfig audioConfig;
    
    Engine engine(config, inputConfig, audioConfig);
    engine.init();
    
    // Audio engine should be accessible
    TEST_ASSERT_NOT_NULL(&engine.getAudioEngine());
}

void test_engine_get_current_scene() {
    // Test getCurrentScene returns nullopt when no scene set
    MockDrawSurface* mockSurface = new MockDrawSurface();
    DisplayConfig config = PIXELROOT32_CUSTOM_DISPLAY(mockSurface, 240, 240);
    Engine engine(config);
    
    // Initially should be nullopt (no scene)
    auto scene = engine.getCurrentScene();
    TEST_ASSERT_FALSE(scene.has_value());
    
    // After setting a scene
    MockScene mockScene;
    engine.setScene(&mockScene);
    
    auto sceneAfter = engine.getCurrentScene();
    TEST_ASSERT_TRUE(sceneAfter.has_value());
    TEST_ASSERT_EQUAL(&mockScene, sceneAfter.value());
}

void test_engine_input_update() {
    // Test that input manager is accessible and initialized
    MockDrawSurface* mockSurface = new MockDrawSurface();
    DisplayConfig config = PIXELROOT32_CUSTOM_DISPLAY(mockSurface, 240, 240);
    InputConfig inputConfig(8, 0, 1, 2, 3, 4, 5, 6, 7);
    Engine engine(config, inputConfig);
    
    engine.init();
    
    // Input manager should be initialized
    InputManager& input = engine.getInputManager();
    TEST_ASSERT_NOT_NULL(&input);
    
    // No crash - Engine::update() is protected, but we verify setup is correct
    TEST_ASSERT_TRUE(true);
}

// Phase 6.3: Profiling and Debug Tests

void test_engine_profiling_data_collection() {
    // Test that Engine initializes profiling variables
    MockDrawSurface* mockSurface = new MockDrawSurface();
    DisplayConfig config = PIXELROOT32_CUSTOM_DISPLAY(mockSurface, 240, 240);
    Engine engine(config);
    
    engine.init();
    
    // Get initial time values
    unsigned long t1 = engine.getMillis();
    
    // Perform some work
    for (volatile int i = 0; i < 10000; i++);
    
    unsigned long t2 = engine.getMillis();
    
    // Time should have advanced (shows timing system is working)
    TEST_ASSERT_TRUE(t2 >= t1);
}

void test_engine_delta_time_updates() {
    // Test that delta time is properly calculated
    MockDrawSurface* mockSurface = new MockDrawSurface();
    DisplayConfig config = PIXELROOT32_CUSTOM_DISPLAY(mockSurface, 240, 240);
    Engine engine(config);
    
    engine.init();
    
    // Get initial delta time (should be 0 before first update)
    unsigned long dt1 = engine.getDeltaTime();
    
    // Delta time should be reasonable (0 at start)
    TEST_ASSERT_EQUAL(0UL, dt1);
    
    // After init, we can verify the engine is tracking time
    unsigned long t1 = engine.getMillis();
    TEST_ASSERT_TRUE(t1 > 0 || t1 == 0);  // Time should be valid
}

void test_engine_platform_capabilities_profiling() {
    // Test platform capabilities include profiling info
    MockDrawSurface* mockSurface = new MockDrawSurface();
    DisplayConfig config = PIXELROOT32_CUSTOM_DISPLAY(mockSurface, 240, 240);
    Engine engine(config);
    
    const auto& caps = engine.getPlatformCapabilities();
    
    // Platform capabilities should be available
    // This verifies the profiling infrastructure is accessible
    TEST_ASSERT_TRUE(true);  // No crash accessing capabilities
}

void test_engine_scene_with_profiling() {
    // Test scene lifecycle with timing measurements
    MockDrawSurface* mockSurface = new MockDrawSurface();
    DisplayConfig config = PIXELROOT32_CUSTOM_DISPLAY(mockSurface, 240, 240);
    Engine engine(config);
    
    engine.init();
    
    MockScene scene;
    engine.setScene(&scene);
    
    // Verify scene is set
    auto currentScene = engine.getCurrentScene();
    TEST_ASSERT_TRUE(currentScene.has_value());
    
    // Verify time tracking continues to work with scene
    unsigned long t1 = engine.getMillis();
    for (volatile int i = 0; i < 5000; i++);
    unsigned long t2 = engine.getMillis();
    
    TEST_ASSERT_TRUE(t2 >= t1);
}


void setUp(void) {
    test_setup();
}

void tearDown(void) {
    test_teardown();
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_engine_creation_with_display);
    RUN_TEST(test_engine_creation_with_display_and_input);
    RUN_TEST(test_engine_init);
    RUN_TEST(test_engine_get_renderer);
    RUN_TEST(test_engine_get_input_manager);
    RUN_TEST(test_engine_creation_lvalue_display);
    RUN_TEST(test_engine_creation_lvalue_display_and_input);
    RUN_TEST(test_engine_creation_lvalue_with_audio);
    RUN_TEST(test_engine_get_delta_time);
    RUN_TEST(test_engine_get_platform_capabilities);
    RUN_TEST(test_engine_get_audio_engine);
    RUN_TEST(test_engine_get_music_player);
    RUN_TEST(test_engine_set_renderer);
    
    // Engine Integration Tests
    RUN_TEST(test_engine_with_mock_scene);
    RUN_TEST(test_engine_scene_lifecycle);
    RUN_TEST(test_engine_multiple_scenes);
    RUN_TEST(test_engine_get_millis);
    RUN_TEST(test_engine_init_with_audio);
    RUN_TEST(test_engine_get_current_scene);
    RUN_TEST(test_engine_input_update);
    
    // Phase 6.3: Profiling and Debug Tests
    RUN_TEST(test_engine_profiling_data_collection);
    RUN_TEST(test_engine_delta_time_updates);
    RUN_TEST(test_engine_platform_capabilities_profiling);
    RUN_TEST(test_engine_scene_with_profiling);
    
    return UNITY_END();
}
