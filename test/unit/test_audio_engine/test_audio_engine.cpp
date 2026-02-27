/**
 * @file test_audio_engine.cpp
 * @brief Unit tests for audio/AudioEngine module.
 */

#include <unity.h>
#include "../../test_config.h"
#include "audio/AudioEngine.h"
#include "audio/AudioConfig.h"
#include "mocks/MockAudioScheduler.h"
#include <memory>

using namespace pixelroot32::audio;

void setUp(void) {
    test_setup();
}

void tearDown(void) {
    test_teardown();
}

void test_audio_engine_initialization(void) {
    AudioConfig config(nullptr, 22050);
    AudioEngine engine(config);
    
    // Engine should have a default scheduler after init
    engine.init();
    
    // Test master volume defaults
    TEST_ASSERT_EQUAL_FLOAT(1.0f, engine.getMasterVolume());
}

void test_audio_engine_custom_scheduler(void) {
    AudioConfig config(nullptr, 22050);
    AudioEngine engine(config);
    
    auto mockScheduler = std::make_unique<MockAudioScheduler>();
    MockAudioScheduler& mockRef = *mockScheduler;
    
    engine.setScheduler(std::move(mockScheduler));
    engine.init();
    
    // Submit a command and check if it reached the mock scheduler
    AudioCommand cmd;
    cmd.type = AudioCommandType::PLAY_EVENT;
    engine.submitCommand(cmd);
    
    TEST_ASSERT_TRUE(mockRef.hasCommand(AudioCommandType::PLAY_EVENT));
}

void test_audio_engine_master_volume(void) {
    AudioConfig config(nullptr, 22050);
    AudioEngine engine(config);
    engine.init();
    
    engine.setMasterVolume(0.5f);
    TEST_ASSERT_EQUAL_FLOAT(0.5f, engine.getMasterVolume());
    
    engine.setMasterVolume(1.5f); // Should clamp to 1.0f
    TEST_ASSERT_EQUAL_FLOAT(1.0f, engine.getMasterVolume()); 
}

void test_audio_engine_play_event(void) {
    AudioConfig config(nullptr, 22050);
    AudioEngine engine(config);
    
    auto mockScheduler = std::make_unique<MockAudioScheduler>();
    MockAudioScheduler& mockRef = *mockScheduler;
    engine.setScheduler(std::move(mockScheduler));
    engine.init();
    
    AudioEvent event;
    event.type = WaveType::PULSE;
    event.frequency = 440.0f;
    
    engine.playEvent(event);
    
    // playEvent should submit a PLAY_EVENT command
    TEST_ASSERT_TRUE(mockRef.hasCommand(AudioCommandType::PLAY_EVENT));
}

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;
    
    UNITY_BEGIN();
    RUN_TEST(test_audio_engine_initialization);
    RUN_TEST(test_audio_engine_custom_scheduler);
    RUN_TEST(test_audio_engine_master_volume);
    RUN_TEST(test_audio_engine_play_event);
    return UNITY_END();
}
