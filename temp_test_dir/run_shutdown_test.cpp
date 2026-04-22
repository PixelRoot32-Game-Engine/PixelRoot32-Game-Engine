#define UNITY_INCLUDE_CONFIG_H
#include "unity.h"
#include "audio/DefaultAudioScheduler.h"
#include "audio/AudioConfig.h"
#include "audio/AudioMusicTypes.h"
#include "platforms/PlatformCapabilities.h"

using namespace pixelroot32::audio;

void setUp(void) {}
void tearDown(void) {}

void test_audio_scheduler_shutdown_during_active_playback(void) {
    // Test graceful shutdown when audio is actively playing
    DefaultAudioScheduler scheduler;
    
    // Submit 4 commands to use all available channels (auto-assigned, not set channelIndex)
    for (int i = 0; i < 4; i++) {
        AudioCommand cmd;
        cmd.type = AudioCommandType::PLAY_EVENT;
        cmd.event.type = WaveType::TRIANGLE;
        cmd.event.frequency = 220.0f + i * 55.0f;  // Vary frequency per channel
        cmd.event.volume = 0.8f;
        cmd.event.duration = 5.0f;  // Long duration
        scheduler.submitCommand(cmd);
    }
    
    // Generate some samples to start playback
    int16_t buffer[256];
    scheduler.generateSamples(buffer, 256);
    
    // Stop the scheduler while playback is active
    scheduler.stop();
    
    // Should be able to generate samples after stop without crashing
    scheduler.generateSamples(buffer, 256);
    
    // Verify system remains stable after stop (no crash is the main test)
    TEST_ASSERT_TRUE(true);  // No crash during shutdown with active playback
}

int main(int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(test_audio_scheduler_shutdown_during_active_playback);
    return UNITY_END();
}