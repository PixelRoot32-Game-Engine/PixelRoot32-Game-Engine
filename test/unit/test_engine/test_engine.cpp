#include <unity.h>
#include "../../test_config.h"
#include "core/Engine.h"
#include "graphics/DisplayConfig.h"
#include "graphics/BaseDrawSurface.h"
#include "input/InputConfig.h"

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
    return UNITY_END();
}
