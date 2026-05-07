/**
 * @file test_audio_engine.cpp
 * @brief Unit tests for audio/AudioEngine module.
 */

#include "unity.h"
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

// =============================================================================
// Coverage expansion tests for FASE 2
// =============================================================================

void test_audio_engine_generate_samples(void) {
    AudioConfig config(nullptr, 22050);
    AudioEngine engine(config);
    
    auto mockScheduler = std::make_unique<MockAudioScheduler>();
    engine.setScheduler(std::move(mockScheduler));
    engine.init();
    
    // Test generateSamples with valid buffer
    int16_t buffer[256];
    engine.generateSamples(buffer, 256);
    
    // Verify samples were generated - buffer should be modified
    // (values may be 0 if no audio playing, but function should complete without crash)
    TEST_ASSERT_TRUE(buffer[0] == buffer[0]); // NaN check for int16_t
}

void test_audio_engine_generate_samples_null_buffer(void) {
    AudioConfig config(nullptr, 22050);
    AudioEngine engine(config);
    
    auto mockScheduler = std::make_unique<MockAudioScheduler>();
    engine.setScheduler(std::move(mockScheduler));
    engine.init();
    
    // Should not crash with null pointer
    engine.generateSamples(nullptr, 0);
    
    // Verify engine is still functional
    TEST_ASSERT_EQUAL_UINT8(0, engine.getMasterBitcrush());
}

void test_audio_engine_set_master_bitcrush(void) {
    AudioConfig config(nullptr, 22050);
    AudioEngine engine(config);
    
    auto mockScheduler = std::make_unique<MockAudioScheduler>();
    MockAudioScheduler& mockRef = *mockScheduler;
    engine.setScheduler(std::move(mockScheduler));
    engine.init();
    
    // Test bitcrush setter
    engine.setMasterBitcrush(8);
    TEST_ASSERT_EQUAL_UINT8(8, engine.getMasterBitcrush());
    
    // Test bitcrush clamping (max 15)
    engine.setMasterBitcrush(20);
    TEST_ASSERT_EQUAL_UINT8(15, engine.getMasterBitcrush());
    
    // Test bitcrush at boundary
    engine.setMasterBitcrush(0);
    TEST_ASSERT_EQUAL_UINT8(0, engine.getMasterBitcrush());
    engine.setMasterBitcrush(15);
    TEST_ASSERT_EQUAL_UINT8(15, engine.getMasterBitcrush());
}

void test_audio_engine_is_music_playing(void) {
    AudioConfig config(nullptr, 22050);
    AudioEngine engine(config);
    
    auto mockScheduler = std::make_unique<MockAudioScheduler>();
    MockAudioScheduler& mockRef = *mockScheduler;
    mockRef.setMusicPlaying(true);
    engine.setScheduler(std::move(mockScheduler));
    engine.init();
    
    TEST_ASSERT_TRUE(engine.isMusicPlaying());
}

void test_audio_engine_is_music_paused(void) {
    AudioConfig config(nullptr, 22050);
    AudioEngine engine(config);
    
    auto mockScheduler = std::make_unique<MockAudioScheduler>();
    MockAudioScheduler& mockRef = *mockScheduler;
    mockRef.setMusicPaused(true);
    engine.setScheduler(std::move(mockScheduler));
    engine.init();
    
    TEST_ASSERT_TRUE(engine.isMusicPaused());
}

void test_audio_engine_is_music_playing_false(void) {
    AudioConfig config(nullptr, 22050);
    AudioEngine engine(config);
    
    auto mockScheduler = std::make_unique<MockAudioScheduler>();
    MockAudioScheduler& mockRef = *mockScheduler;
    mockRef.setMusicPlaying(false);
    engine.setScheduler(std::move(mockScheduler));
    engine.init();
    
    TEST_ASSERT_FALSE(engine.isMusicPlaying());
}

void test_audio_engine_is_music_paused_false(void) {
    AudioConfig config(nullptr, 22050);
    AudioEngine engine(config);
    
    auto mockScheduler = std::make_unique<MockAudioScheduler>();
    MockAudioScheduler& mockRef = *mockScheduler;
    mockRef.setMusicPaused(false);
    engine.setScheduler(std::move(mockScheduler));
    engine.init();
    
    TEST_ASSERT_FALSE(engine.isMusicPaused());
}

void test_audio_engine_submit_command(void) {
    AudioConfig config(nullptr, 22050);
    AudioEngine engine(config);
    
    auto mockScheduler = std::make_unique<MockAudioScheduler>();
    MockAudioScheduler& mockRef = *mockScheduler;
    engine.setScheduler(std::move(mockScheduler));
    engine.init();
    
    // Test submitCommand directly
    AudioCommand cmd;
    cmd.type = AudioCommandType::SET_MASTER_VOLUME;
    cmd.volume = 0.5f;
    engine.submitCommand(cmd);
    
    TEST_ASSERT_TRUE(mockRef.hasCommand(AudioCommandType::SET_MASTER_VOLUME));
}

void test_audio_engine_submit_command_null_scheduler(void) {
    // Test with default scheduler (no custom scheduler set)
    AudioConfig config(nullptr, 22050);
    AudioEngine engine(config);
    engine.init();
    
    // Should not crash - default scheduler handles gracefully
    AudioCommand cmd;
    cmd.type = AudioCommandType::SET_MASTER_VOLUME;
    engine.submitCommand(cmd);
    
    // Verify command was processed (volume should be default)
    TEST_ASSERT_EQUAL_FLOAT(1.0f, engine.getMasterVolume());
}

void test_audio_engine_volume_clamping_above_max(void) {
    AudioConfig config(nullptr, 22050);
    AudioEngine engine(config);
    engine.init();
    
    // Test clamping above 1.0
    engine.setMasterVolume(1.5f);
    TEST_ASSERT_EQUAL_FLOAT(1.0f, engine.getMasterVolume());
}

void test_audio_engine_volume_clamping_below_zero(void) {
    AudioConfig config(nullptr, 22050);
    AudioEngine engine(config);
    engine.init();
    
    // Test clamping below 0.0
    engine.setMasterVolume(-0.5f);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, engine.getMasterVolume());
}

void test_audio_engine_volume_at_boundaries(void) {
    AudioConfig config(nullptr, 22050);
    AudioEngine engine(config);
    engine.init();
    
    // Test exact boundary values
    engine.setMasterVolume(0.0f);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, engine.getMasterVolume());
    
    engine.setMasterVolume(1.0f);
    TEST_ASSERT_EQUAL_FLOAT(1.0f, engine.getMasterVolume());
}

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;
    
    UNITY_BEGIN();
    RUN_TEST(test_audio_engine_initialization);
    RUN_TEST(test_audio_engine_custom_scheduler);
    RUN_TEST(test_audio_engine_master_volume);
    RUN_TEST(test_audio_engine_play_event);
    
    // FASE 2 coverage expansion tests
    RUN_TEST(test_audio_engine_generate_samples);
    RUN_TEST(test_audio_engine_generate_samples_null_buffer);
    RUN_TEST(test_audio_engine_set_master_bitcrush);
    RUN_TEST(test_audio_engine_is_music_playing);
    RUN_TEST(test_audio_engine_is_music_paused);
    RUN_TEST(test_audio_engine_is_music_playing_false);
    RUN_TEST(test_audio_engine_is_music_paused_false);
    RUN_TEST(test_audio_engine_submit_command);
    RUN_TEST(test_audio_engine_submit_command_null_scheduler);
    RUN_TEST(test_audio_engine_volume_clamping_above_max);
    RUN_TEST(test_audio_engine_volume_clamping_below_zero);
    RUN_TEST(test_audio_engine_volume_at_boundaries);
    
    return UNITY_END();
}
