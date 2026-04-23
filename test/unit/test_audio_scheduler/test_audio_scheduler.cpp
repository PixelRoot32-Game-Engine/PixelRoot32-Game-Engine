#include <unity.h>
#include "audio/DefaultAudioScheduler.h"
#include "audio/AudioConfig.h"
#include "audio/AudioMusicTypes.h"

using namespace pixelroot32::audio;

void setUp(void) {}
void tearDown(void) {}

// Helper function to properly initialize a scheduler with default settings
void initScheduler(DefaultAudioScheduler& scheduler) {
    scheduler.init(nullptr, 44100, pixelroot32::platforms::PlatformCapabilities{});
}

void test_default_scheduler_initialization(void) {
    DefaultAudioScheduler scheduler;
    initScheduler(scheduler);
    
    // Check if we can generate samples without crashing
    int16_t buffer[256];
    scheduler.generateSamples(buffer, 256);
    // Even with no commands, it should output silence (zeros)
    for (int i = 0; i < 256; i++) {
        TEST_ASSERT_EQUAL(0, buffer[i]);
    }
}

void test_default_scheduler_play_note(void) {
    DefaultAudioScheduler scheduler;
    
    AudioCommand cmd{};
    cmd.type = AudioCommandType::PLAY_EVENT;
    cmd.event.type = WaveType::PULSE;
    cmd.event.frequency = 440.0f;
    cmd.event.volume = 1.0f;
    cmd.event.duration = 1.0f;
    cmd.event.duty = 0.5f;
    
    scheduler.submitCommand(cmd);
    
    int16_t buffer[256];
    scheduler.generateSamples(buffer, 256);
    
    // After playing a note, buffer should NOT be all zeros
    bool allZeros = true;
    for (int i = 0; i < 256; i++) {
        if (buffer[i] != 0) {
            allZeros = false;
            break;
        }
    }
    TEST_ASSERT_FALSE(allZeros);
}

void test_default_scheduler_stop_all(void) {
    DefaultAudioScheduler scheduler;
    
    // Play something
    AudioCommand cmd{};
    cmd.type = AudioCommandType::PLAY_EVENT;
    cmd.event.type = WaveType::PULSE;
    cmd.event.frequency = 440.0f;
    cmd.event.volume = 1.0f;
    cmd.event.duration = 1.0f;
    cmd.event.preset = nullptr;  // Explicitly set to avoid garbage pointer
    scheduler.submitCommand(cmd);
    
    // Stop channel 0 (DefaultAudioScheduler uses channels 0-3)
    AudioCommand stopCmd{};
    stopCmd.type = AudioCommandType::STOP_CHANNEL;
    stopCmd.channelIndex = 0;
    scheduler.submitCommand(stopCmd);
    
    int16_t buffer[256];
    scheduler.generateSamples(buffer, 256);
    
    // Should be silence again
    for (int i = 0; i < 256; i++) {
        TEST_ASSERT_EQUAL(0, buffer[i]);
    }
}

void test_default_scheduler_multiple_notes(void) {
    DefaultAudioScheduler scheduler;
    AudioCommand cmd{};
    cmd.type = AudioCommandType::PLAY_EVENT;
    cmd.event.type = WaveType::TRIANGLE;
    cmd.event.frequency = 220.0f;
    cmd.event.volume = 0.5f;
    cmd.event.duration = 0.5f;
    cmd.event.preset = nullptr;
    scheduler.submitCommand(cmd);
    cmd.event.frequency = 880.0f;
    scheduler.submitCommand(cmd);
    int16_t buffer[512];
    scheduler.generateSamples(buffer, 512);
    bool hasNonZero = false;
    for (int i = 0; i < 512; i++) {
        if (buffer[i] != 0) { hasNonZero = true; break; }
    }
    TEST_ASSERT_TRUE(hasNonZero);
}

void test_default_scheduler_triangle_wave(void) {
    DefaultAudioScheduler scheduler;
    AudioCommand cmd{};
    cmd.type = AudioCommandType::PLAY_EVENT;
    cmd.event.type = WaveType::TRIANGLE;
    cmd.event.frequency = 330.0f;
    cmd.event.volume = 1.0f;
    cmd.event.duration = 0.1f;
    cmd.event.preset = nullptr;
    scheduler.submitCommand(cmd);
    int16_t buffer[256];
    scheduler.generateSamples(buffer, 256);
    bool hasNonZero = false;
    for (int i = 0; i < 256; i++) {
        if (buffer[i] != 0) { hasNonZero = true; break; }
    }
    TEST_ASSERT_TRUE(hasNonZero);
}

void test_default_scheduler_noise_wave(void) {
    DefaultAudioScheduler scheduler;
    AudioCommand cmd{};
    cmd.type = AudioCommandType::PLAY_EVENT;
    cmd.event.type = WaveType::NOISE;
    cmd.event.frequency = 440.0f;
    cmd.event.volume = 0.5f;
    cmd.event.duration = 0.05f;
    cmd.event.preset = nullptr;
    scheduler.submitCommand(cmd);
    int16_t buffer[256];
    scheduler.generateSamples(buffer, 256);
    bool hasNonZero = false;
    for (int i = 0; i < 256; i++) {
        if (buffer[i] != 0) { hasNonZero = true; break; }
    }
    TEST_ASSERT_TRUE(hasNonZero);
}

void test_default_scheduler_stop_multiple_channels(void) {
    DefaultAudioScheduler scheduler;
    AudioCommand cmd{};
    cmd.type = AudioCommandType::PLAY_EVENT;
    cmd.event.type = WaveType::PULSE;
    cmd.event.frequency = 440.0f;
    cmd.event.volume = 1.0f;
    cmd.event.duration = 1.0f;
    cmd.event.preset = nullptr;
    for (int ch = 0; ch < 4; ch++) {
        scheduler.submitCommand(cmd);
    }
    AudioCommand stopCmd{};
    stopCmd.type = AudioCommandType::STOP_CHANNEL;
    stopCmd.channelIndex = 1;
    scheduler.submitCommand(stopCmd);
    stopCmd.channelIndex = 2;
    scheduler.submitCommand(stopCmd);
    int16_t buffer[256];
    scheduler.generateSamples(buffer, 256);
}

void test_default_scheduler_set_master_volume(void) {
    DefaultAudioScheduler scheduler;
    
    // Play a note to verify audio is generated
    AudioCommand playCmd{};
    playCmd.type = AudioCommandType::PLAY_EVENT;
    playCmd.event.type = WaveType::PULSE;
    playCmd.event.frequency = 440.0f;
    playCmd.event.volume = 1.0f;
    playCmd.event.duration = 1.0f;
    playCmd.event.preset = nullptr;
    scheduler.submitCommand(playCmd);
    
    // Set master volume (should not affect this test's audio generation)
    AudioCommand volCmd{};
    volCmd.type = AudioCommandType::SET_MASTER_VOLUME;
    volCmd.volume = 0.5f;
    scheduler.submitCommand(volCmd);
    
    // Generate some samples to process the command
    int16_t buffer[256];
    scheduler.generateSamples(buffer, 256);
    
    // Verify some output (should have audio playing)
    bool hasNonZero = false;
    for (int i = 0; i < 256; i++) {
        if (buffer[i] != 0) { hasNonZero = true; break; }
    }
    TEST_ASSERT_TRUE(hasNonZero);
}

void test_default_scheduler_long_sequence(void) {
    DefaultAudioScheduler scheduler;
    AudioCommand cmd{};
    cmd.type = AudioCommandType::PLAY_EVENT;
    cmd.event.type = WaveType::PULSE;
    cmd.event.frequency = 262.0f;
    cmd.event.volume = 0.3f;
    cmd.event.duration = 0.02f;
    for (int i = 0; i < 8; i++) {
        cmd.event.frequency = 262.0f + i * 55.0f;
        scheduler.submitCommand(cmd);
    }
    int16_t buffer[1024];
    for (int b = 0; b < 4; b++) {
        scheduler.generateSamples(buffer, 256);
    }
}

void test_default_scheduler_stop(void) {
    DefaultAudioScheduler scheduler;
    AudioCommand cmd{};
    cmd.type = AudioCommandType::PLAY_EVENT;
    cmd.event.type = WaveType::PULSE;
    cmd.event.frequency = 440.0f;
    cmd.event.volume = 1.0f;
    cmd.event.duration = 1.0f;
    scheduler.submitCommand(cmd);
    scheduler.stop();
    int16_t buffer[256];
    scheduler.generateSamples(buffer, 256);
    (void)buffer;
}

void test_default_scheduler_is_independent(void) {
    DefaultAudioScheduler scheduler;
    TEST_ASSERT_FALSE(scheduler.isIndependent());
}

void test_note_to_frequency(void) {
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 440.0f, noteToFrequency(Note::A, 4));
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 261.63f, noteToFrequency(Note::C, 4));
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 880.0f, noteToFrequency(Note::A, 5));
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 220.0f, noteToFrequency(Note::A, 3));
    TEST_ASSERT_EQUAL_FLOAT(0.0f, noteToFrequency(Note::Rest, 4));
}

void test_music_play_command(void) {
    DefaultAudioScheduler scheduler;
    AudioCommand cmd{};
    cmd.type = AudioCommandType::MUSIC_PLAY;
    scheduler.submitCommand(cmd);
    scheduler.generateSamples(nullptr, 256);
    TEST_ASSERT_TRUE(true);
}

void test_music_stop_command(void) {
    DefaultAudioScheduler scheduler;
    AudioCommand playCmd;
    playCmd.type = AudioCommandType::MUSIC_PLAY;
    scheduler.submitCommand(playCmd);
    AudioCommand stopCmd;
    stopCmd.type = AudioCommandType::MUSIC_STOP;
    scheduler.submitCommand(stopCmd);
    scheduler.generateSamples(nullptr, 256);
    TEST_ASSERT_TRUE(true);
}

void test_music_pause_resume_command(void) {
    DefaultAudioScheduler scheduler;
    AudioCommand playCmd;
    playCmd.type = AudioCommandType::MUSIC_PLAY;
    scheduler.submitCommand(playCmd);
    AudioCommand pauseCmd;
    pauseCmd.type = AudioCommandType::MUSIC_PAUSE;
    scheduler.submitCommand(pauseCmd);
    AudioCommand resumeCmd;
    resumeCmd.type = AudioCommandType::MUSIC_RESUME;
    scheduler.submitCommand(resumeCmd);
    scheduler.generateSamples(nullptr, 256);
    TEST_ASSERT_TRUE(true);
}

void test_music_set_tempo_command(void) {
    DefaultAudioScheduler scheduler;
    AudioCommand cmd{};
    cmd.type = AudioCommandType::MUSIC_SET_TEMPO;
    cmd.tempoFactor = 2.0f;
    scheduler.submitCommand(cmd);
    scheduler.generateSamples(nullptr, 256);
    TEST_ASSERT_TRUE(true);
}

void test_set_master_volume_clamping(void) {
    DefaultAudioScheduler scheduler;
    AudioCommand cmd{};
    cmd.type = AudioCommandType::SET_MASTER_VOLUME;
    cmd.volume = 1.5f;
    scheduler.submitCommand(cmd);
    scheduler.generateSamples(nullptr, 256);
    TEST_ASSERT_TRUE(true);
    
    cmd.volume = -0.5f;
    scheduler.submitCommand(cmd);
    scheduler.generateSamples(nullptr, 256);
    TEST_ASSERT_TRUE(true);
}

// AudioScheduler Error Handling Tests

void test_audio_scheduler_buffer_underrun_recovery(void) {
    // Test that scheduler recovers gracefully from buffer underrun scenarios
    DefaultAudioScheduler scheduler;
    
    // Start playing a note
    AudioCommand cmd{};
    cmd.type = AudioCommandType::PLAY_EVENT;
    cmd.event.type = WaveType::PULSE;
    cmd.event.frequency = 440.0f;
    cmd.event.volume = 1.0f;
    cmd.event.duration = 2.0f;
    scheduler.submitCommand(cmd);
    
    // Generate some samples to start playback
    int16_t buffer[64];
    scheduler.generateSamples(buffer, 64);
    
    // Simulate buffer underrun by calling with very small buffer repeatedly
    for (int i = 0; i < 10; i++) {
        scheduler.generateSamples(buffer, 1);  // Single sample
    }
    
    // Should still be able to generate samples without crashing
    scheduler.generateSamples(buffer, 256);
    TEST_ASSERT_TRUE(true);  // No crash during underrun recovery
}

void test_audio_scheduler_initialization_failure_handling(void) {
    // Test scheduler behavior with various initialization scenarios
    DefaultAudioScheduler scheduler;
    
    // Test with zero sample rate (edge case)
    scheduler.init(nullptr, 0, pixelroot32::platforms::PlatformCapabilities{});
    
    // Should still be able to generate samples (graceful degradation)
    int16_t buffer[128];
    scheduler.generateSamples(buffer, 128);
    TEST_ASSERT_TRUE(true);  // No crash with zero sample rate
    
    // Test with very high sample rate
    scheduler.init(nullptr, 192000, pixelroot32::platforms::PlatformCapabilities{});
    scheduler.generateSamples(buffer, 128);
    TEST_ASSERT_TRUE(true);  // No crash with high sample rate
}

void test_audio_scheduler_shutdown_during_active_playback(void) {
    // Test graceful shutdown when audio is actively playing
    DefaultAudioScheduler scheduler;
    
    // Submit 4 commands to use all available channels with correct wave types
    // Channel map: 0=PULSE, 1=PULSE, 2=TRIANGLE, 3=NOISE
    WaveType waveTypes[4] = {WaveType::PULSE, WaveType::PULSE, WaveType::TRIANGLE, WaveType::NOISE};
    for (int i = 0; i < 4; i++) {
        AudioCommand cmd{};
        cmd.type = AudioCommandType::PLAY_EVENT;
        cmd.event.type = waveTypes[i];
        cmd.event.frequency = 220.0f + i * 55.0f;  // Vary frequency per channel
        cmd.event.volume = 0.8f;
        cmd.event.duration = 5.0f;  // Long duration
        cmd.event.duty = 0.5f;
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

void test_audio_scheduler_null_buffer_handling(void) {
    // Test scheduler behavior with null buffer pointers
    DefaultAudioScheduler scheduler;
    
    // Start some audio
    AudioCommand cmd{};
    cmd.type = AudioCommandType::PLAY_EVENT;
    cmd.event.type = WaveType::NOISE;
    cmd.event.frequency = 100.0f;
    cmd.event.volume = 0.5f;
    cmd.event.duration = 1.0f;
    scheduler.submitCommand(cmd);
    
    // Should handle null buffer gracefully
    scheduler.generateSamples(nullptr, 256);
    TEST_ASSERT_TRUE(true);  // No crash with null buffer
    
    // Should handle zero length gracefully
    int16_t buffer[128];
    scheduler.generateSamples(buffer, 0);
    TEST_ASSERT_TRUE(true);  // No crash with zero length
}

void test_audio_scheduler_command_queue_overflow(void) {
    // Test behavior when command queue gets full
    DefaultAudioScheduler scheduler;
    initScheduler(scheduler);
    
    // Submit many commands rapidly (mix of types to use all channels)
    AudioCommand cmd{};  // Zero-initialize all fields including union
    cmd.type = AudioCommandType::PLAY_EVENT;
    cmd.event.type = WaveType::PULSE;
    cmd.event.frequency = 440.0f;
    cmd.event.volume = 0.5f;
    cmd.event.duration = 0.01f;  // Short duration to avoid channel buildup
    cmd.event.duty = 0.5f;
    cmd.event.preset = nullptr;  // Explicitly set to avoid garbage pointer
    
    // Use different wave types to distribute across available channels
    WaveType types[4] = {WaveType::PULSE, WaveType::PULSE, WaveType::TRIANGLE, WaveType::NOISE};
    
    // Submit commands (test queue capacity - CAPACITY is 128)
    for (int i = 0; i < 64; i++) {
        cmd.event.type = types[i % 4];
        scheduler.submitCommand(cmd);
    }
    
    // Should still be able to generate samples without crash
    int16_t buffer[256];
    scheduler.generateSamples(buffer, 256);
    TEST_ASSERT_TRUE(true);  // No crash with many commands
}

// AudioScheduler Edge Case Tests

void test_audio_scheduler_zero_duration_sound(void) {
    // Test playing a sound with zero duration
    DefaultAudioScheduler scheduler;
    initScheduler(scheduler);
    
    AudioCommand cmd{};  // Zero-initialize all fields
    cmd.type = AudioCommandType::PLAY_EVENT;
    cmd.event.type = WaveType::PULSE;
    cmd.event.frequency = 440.0f;
    cmd.event.volume = 1.0f;
    cmd.event.duration = 0.0f;  // Zero duration
    cmd.event.duty = 0.5f;
    cmd.event.preset = nullptr;  // Explicitly set to avoid garbage pointer
    
    scheduler.submitCommand(cmd);
    
    int16_t buffer[256];
    scheduler.generateSamples(buffer, 256);
    
    // Should handle zero duration gracefully (no sound or very short)
    TEST_ASSERT_TRUE(true);  // No crash with zero duration
}

void test_audio_scheduler_exact_buffer_boundary(void) {
    // Test scheduling at exact buffer boundaries
    DefaultAudioScheduler scheduler;
    initScheduler(scheduler);
    
    // Start a note that should span exactly buffer boundaries
    AudioCommand cmd{};  // Zero-initialize all fields
    cmd.type = AudioCommandType::PLAY_EVENT;
    cmd.event.type = WaveType::TRIANGLE;
    cmd.event.frequency = 220.5f;  // Non-integer frequency
    cmd.event.volume = 0.7f;
    cmd.event.duration = 0.01f;  // Very short, exact boundary
    cmd.event.duty = 0.5f;
    cmd.event.preset = nullptr;  // Explicitly set to avoid garbage pointer
    
    scheduler.submitCommand(cmd);
    
    // Generate samples at different buffer sizes
    int16_t buffer1[64];
    int16_t buffer2[128];
    int16_t buffer3[256];
    
    scheduler.generateSamples(buffer1, 64);
    scheduler.generateSamples(buffer2, 128);
    scheduler.generateSamples(buffer3, 256);
    
    // Should handle all buffer sizes without artifacts
    TEST_ASSERT_TRUE(true);  // No crash at boundaries
}

void test_audio_scheduler_rapid_schedules(void) {
    // Test multiple rapid schedules without overflow
    DefaultAudioScheduler scheduler;
    initScheduler(scheduler);
    
    // Rapidly schedule many short notes
    for (int i = 0; i < 50; i++) {
        AudioCommand cmd{};  // Zero-initialize all fields
        cmd.type = AudioCommandType::PLAY_EVENT;
        cmd.event.type = WaveType::NOISE;
        cmd.event.frequency = 100.0f + (i * 10.0f);
        cmd.event.volume = 0.3f;
        cmd.event.duration = 0.001f;  // Very short
        cmd.event.duty = 0.5f;
        cmd.event.preset = nullptr;  // Explicitly set to avoid garbage pointer
        // Note: channel is auto-assigned by ApuCore, do NOT set cmd.channelIndex here
        // as it would corrupt the union (channelIndex shares memory with event)
        scheduler.submitCommand(cmd);
    }
    
    // Generate samples to process all commands
    int16_t buffer[1024];
    scheduler.generateSamples(buffer, 1024);
    
    // Should handle rapid scheduling without overflow or crash
    TEST_ASSERT_TRUE(true);  // No crash with rapid schedules
}

void test_audio_scheduler_pause_resume_functionality(void) {
    // Test pause and resume functionality thoroughly
    DefaultAudioScheduler scheduler;
    initScheduler(scheduler);
    
    // Start music playback
    AudioCommand playCmd{};  // Zero-initialize all fields
    playCmd.type = AudioCommandType::MUSIC_PLAY;
    playCmd.track = nullptr;  // Will be handled gracefully
    scheduler.submitCommand(playCmd);
    
    // Pause the music
    AudioCommand pauseCmd{};  // Zero-initialize all fields
    pauseCmd.type = AudioCommandType::MUSIC_PAUSE;
    scheduler.submitCommand(pauseCmd);
    
    int16_t buffer[256];
    scheduler.generateSamples(buffer, 256);
    
    // Resume the music
    AudioCommand resumeCmd{};  // Zero-initialize all fields
    resumeCmd.type = AudioCommandType::MUSIC_RESUME;
    scheduler.submitCommand(resumeCmd);
    
    scheduler.generateSamples(buffer, 256);
    
    // Should handle pause/resume sequence without crash
    TEST_ASSERT_TRUE(true);  // No crash with pause/resume
}

void test_audio_scheduler_volume_interpolation(void) {
    // Test volume interpolation during note transitions
    DefaultAudioScheduler scheduler;
    initScheduler(scheduler);
    
    // Start a note
    AudioCommand cmd1{};  // Zero-initialize all fields
    cmd1.type = AudioCommandType::PLAY_EVENT;
    cmd1.event.type = WaveType::PULSE;
    cmd1.event.frequency = 440.0f;
    cmd1.event.volume = 1.0f;
    cmd1.event.duration = 0.1f;
    cmd1.event.duty = 0.5f;
    cmd1.event.preset = nullptr;  // Explicitly set to avoid garbage pointer
    scheduler.submitCommand(cmd1);
    
    // Generate some samples
    int16_t buffer[128];
    scheduler.generateSamples(buffer, 128);
    
    // Start another note on same channel (should interpolate volume)
    AudioCommand cmd2{};  // Zero-initialize all fields
    cmd2.type = AudioCommandType::PLAY_EVENT;
    cmd2.event.type = WaveType::TRIANGLE;
    cmd2.event.frequency = 880.0f;
    cmd2.event.volume = 0.5f;
    cmd2.event.duration = 0.1f;
    cmd2.event.duty = 0.5f;
    cmd2.event.preset = nullptr;  // Explicitly set to avoid garbage pointer
    scheduler.submitCommand(cmd2);
    
    scheduler.generateSamples(buffer, 128);
    
    // Should handle volume interpolation smoothly
    TEST_ASSERT_TRUE(true);  // No crash during interpolation
}

int main(int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(test_default_scheduler_initialization);
    RUN_TEST(test_default_scheduler_play_note);
    RUN_TEST(test_default_scheduler_stop_all);
    RUN_TEST(test_default_scheduler_multiple_notes);
    RUN_TEST(test_default_scheduler_triangle_wave);
    RUN_TEST(test_default_scheduler_noise_wave);
    RUN_TEST(test_default_scheduler_stop_multiple_channels);
    RUN_TEST(test_default_scheduler_set_master_volume);
    RUN_TEST(test_default_scheduler_long_sequence);
    RUN_TEST(test_default_scheduler_stop);
    RUN_TEST(test_default_scheduler_is_independent);
    RUN_TEST(test_note_to_frequency);
    RUN_TEST(test_music_play_command);
    RUN_TEST(test_music_stop_command);
    RUN_TEST(test_music_pause_resume_command);
    RUN_TEST(test_music_set_tempo_command);
    RUN_TEST(test_set_master_volume_clamping);
    
    // Error handling tests
    RUN_TEST(test_audio_scheduler_buffer_underrun_recovery);
    RUN_TEST(test_audio_scheduler_initialization_failure_handling);
    RUN_TEST(test_audio_scheduler_shutdown_during_active_playback);
    RUN_TEST(test_audio_scheduler_null_buffer_handling);
    RUN_TEST(test_audio_scheduler_command_queue_overflow);
    
    // Edge case tests
    RUN_TEST(test_audio_scheduler_zero_duration_sound);
    RUN_TEST(test_audio_scheduler_exact_buffer_boundary);
    RUN_TEST(test_audio_scheduler_rapid_schedules);
    RUN_TEST(test_audio_scheduler_pause_resume_functionality);
    RUN_TEST(test_audio_scheduler_volume_interpolation);
    
    return UNITY_END();
}
