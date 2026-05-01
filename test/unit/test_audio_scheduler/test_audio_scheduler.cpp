#include <unity.h>
#include "audio/DefaultAudioScheduler.h"
#include "audio/AudioConfig.h"
#include "audio/AudioMusicTypes.h"
#include "audio/ApuCore.h"

using namespace pixelroot32::audio;

namespace {

bool bufferHasNonZero(const int16_t* p, int n) {
    for (int i = 0; i < n; ++i) {
        if (p[i] != 0) return true;
    }
    return false;
}

bool bufferAllSilence(const int16_t* p, int n) {
    for (int i = 0; i < n; ++i) {
        if (p[i] != 0) return false;
    }
    return true;
}

int32_t bufferMaxAbs(const int16_t* p, int n) {
    int32_t m = 0;
    for (int i = 0; i < n; ++i) {
        const int32_t v = (int32_t)p[i];
        const int32_t a = v < 0 ? -v : v;
        if (a > m) m = a;
    }
    return m;
}

// ApuCore::generateSamples returns immediately when stream is null, so commands
// would never run; use a scratch buffer to advance the audio graph in tests.
void generateDiscard(DefaultAudioScheduler& scheduler, int length) {
    constexpr int kMaxChunk = 512;
    int16_t sink[kMaxChunk];
    int remaining = length;
    while (remaining > 0) {
        const int chunk = remaining > kMaxChunk ? kMaxChunk : remaining;
        scheduler.generateSamples(sink, chunk);
        remaining -= chunk;
    }
}

} // namespace

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
    generateDiscard(scheduler, 256);
    // Null track: sequencer clears "playing" once it finds no active material.
    TEST_ASSERT_FALSE(scheduler.isMusicPlaying());
    TEST_ASSERT_FALSE(scheduler.isMusicPaused());
}

void test_music_stop_command(void) {
    DefaultAudioScheduler scheduler;
    AudioCommand playCmd{};
    playCmd.type = AudioCommandType::MUSIC_PLAY;
    scheduler.submitCommand(playCmd);
    AudioCommand stopCmd{};
    stopCmd.type = AudioCommandType::MUSIC_STOP;
    scheduler.submitCommand(stopCmd);
    generateDiscard(scheduler, 256);
    TEST_ASSERT_FALSE(scheduler.isMusicPlaying());
    TEST_ASSERT_FALSE(scheduler.isMusicPaused());
}

void test_music_pause_resume_command(void) {
    DefaultAudioScheduler scheduler;
    AudioCommand playCmd{};
    playCmd.type = AudioCommandType::MUSIC_PLAY;
    scheduler.submitCommand(playCmd);
    AudioCommand pauseCmd{};
    pauseCmd.type = AudioCommandType::MUSIC_PAUSE;
    scheduler.submitCommand(pauseCmd);
    generateDiscard(scheduler, 128);
    TEST_ASSERT_TRUE(scheduler.isMusicPlaying());
    TEST_ASSERT_TRUE(scheduler.isMusicPaused());
    AudioCommand resumeCmd{};
    resumeCmd.type = AudioCommandType::MUSIC_RESUME;
    scheduler.submitCommand(resumeCmd);
    generateDiscard(scheduler, 128);
    TEST_ASSERT_FALSE(scheduler.isMusicPaused());
    TEST_ASSERT_FALSE(scheduler.isMusicPlaying());
}

void test_music_set_tempo_command(void) {
    DefaultAudioScheduler scheduler;
    AudioCommand cmd{};
    cmd.type = AudioCommandType::MUSIC_SET_TEMPO;
    cmd.tempoFactor = 2.0f;
    scheduler.submitCommand(cmd);
    generateDiscard(scheduler, 256);
    TEST_ASSERT_EQUAL(0u, scheduler.core().getDroppedCommands());
    TEST_ASSERT_FALSE(scheduler.isMusicPaused());
}

void test_set_master_volume_clamping(void) {
    DefaultAudioScheduler scheduler;
    initScheduler(scheduler);

    AudioCommand volHi{};
    volHi.type = AudioCommandType::SET_MASTER_VOLUME;
    volHi.volume = 1.5f;
    scheduler.submitCommand(volHi);
    AudioCommand playA{};
    playA.type = AudioCommandType::PLAY_EVENT;
    playA.event.type = WaveType::PULSE;
    playA.event.frequency = 440.0f;
    playA.event.volume = 1.0f;
    playA.event.duration = 0.05f;
    playA.event.duty = 0.5f;
    scheduler.submitCommand(playA);
    int16_t bufA[256];
    scheduler.generateSamples(bufA, 256);
    const int32_t peakClampedHigh = bufferMaxAbs(bufA, 256);

    scheduler.core().reset();
    initScheduler(scheduler);

    AudioCommand volOk{};
    volOk.type = AudioCommandType::SET_MASTER_VOLUME;
    volOk.volume = 1.0f;
    scheduler.submitCommand(volOk);
    AudioCommand playB{};
    playB.type = AudioCommandType::PLAY_EVENT;
    playB.event.type = WaveType::PULSE;
    playB.event.frequency = 440.0f;
    playB.event.volume = 1.0f;
    playB.event.duration = 0.05f;
    playB.event.duty = 0.5f;
    scheduler.submitCommand(playB);
    int16_t bufB[256];
    scheduler.generateSamples(bufB, 256);
    const int32_t peakAtOne = bufferMaxAbs(bufB, 256);

    TEST_ASSERT_GREATER_THAN(500, peakClampedHigh);
    TEST_ASSERT_INT_WITHIN((int32_t)(peakAtOne * 0.15f) + 500, peakAtOne, peakClampedHigh);

    scheduler.core().reset();
    initScheduler(scheduler);
    AudioCommand volZero{};
    volZero.type = AudioCommandType::SET_MASTER_VOLUME;
    volZero.volume = -0.5f;
    scheduler.submitCommand(volZero);
    AudioCommand playC{};
    playC.type = AudioCommandType::PLAY_EVENT;
    playC.event.type = WaveType::PULSE;
    playC.event.frequency = 440.0f;
    playC.event.volume = 1.0f;
    playC.event.duration = 0.05f;
    playC.event.duty = 0.5f;
    scheduler.submitCommand(playC);
    int16_t bufC[256];
    scheduler.generateSamples(bufC, 256);
    TEST_ASSERT_LESS_OR_EQUAL(100, bufferMaxAbs(bufC, 256));
}

// AudioScheduler Error Handling Tests

void test_audio_scheduler_buffer_underrun_recovery(void) {
    DefaultAudioScheduler scheduler;
    initScheduler(scheduler);

    AudioCommand cmd{};
    cmd.type = AudioCommandType::PLAY_EVENT;
    cmd.event.type = WaveType::PULSE;
    cmd.event.frequency = 440.0f;
    cmd.event.volume = 1.0f;
    cmd.event.duration = 2.0f;
    cmd.event.duty = 0.5f;
    scheduler.submitCommand(cmd);

    // Buffer must fit largest generateSamples() length below (256).
    int16_t buffer[256];
    scheduler.generateSamples(buffer, 64);
    for (int i = 0; i < 10; i++) {
        scheduler.generateSamples(buffer, 1);
    }
    scheduler.generateSamples(buffer, 256);
    TEST_ASSERT_TRUE(bufferHasNonZero(buffer, 256));
    TEST_ASSERT_EQUAL(0u, scheduler.core().getDroppedCommands());
}

void test_audio_scheduler_initialization_failure_handling(void) {
    // Test scheduler behavior with various initialization scenarios
    DefaultAudioScheduler scheduler;
    
    // Test with zero sample rate (edge case)
    scheduler.init(nullptr, 0, pixelroot32::platforms::PlatformCapabilities{});
    
    // Should still be able to generate samples (graceful degradation)
    int16_t buffer[128];
    scheduler.generateSamples(buffer, 128);
    TEST_ASSERT_EQUAL(44100, scheduler.core().getSampleRate());

    // Test with very high sample rate
    scheduler.init(nullptr, 192000, pixelroot32::platforms::PlatformCapabilities{});
    scheduler.generateSamples(buffer, 128);
    TEST_ASSERT_EQUAL(192000, scheduler.core().getSampleRate());
    TEST_ASSERT_EQUAL(0u, scheduler.core().getDroppedCommands());
}

void test_audio_scheduler_shutdown_during_active_playback(void) {
    // Graceful shutdown while audio is actively playing (all four voice slots).
    DefaultAudioScheduler scheduler;
    initScheduler(scheduler);

    WaveType waveTypes[4] = {WaveType::PULSE, WaveType::PULSE, WaveType::TRIANGLE, WaveType::NOISE};
    for (int i = 0; i < 4; i++) {
        AudioCommand cmd{};
        cmd.type = AudioCommandType::PLAY_EVENT;
        cmd.event.type = waveTypes[i];
        cmd.event.frequency = 220.0f + i * 55.0f;
        cmd.event.volume = 0.8f;
        cmd.event.duration = 5.0f;
        cmd.event.duty = 0.5f;
        scheduler.submitCommand(cmd);
    }

    int16_t buffer[256];
    scheduler.generateSamples(buffer, 256);
    TEST_ASSERT_TRUE(bufferHasNonZero(buffer, 256));
    scheduler.stop();
    scheduler.generateSamples(buffer, 256);
    TEST_ASSERT_EQUAL(0u, scheduler.core().getDroppedCommands());
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

    // Null stream: early-out must not corrupt state; command stays queued until real output.
    scheduler.generateSamples(nullptr, 256);

    int16_t buffer[128];
    for (int i = 0; i < 128; ++i) {
        buffer[i] = (int16_t)0x2D2D;
    }
    scheduler.generateSamples(buffer, 0);
    for (int i = 0; i < 128; ++i) {
        TEST_ASSERT_EQUAL_INT16((int16_t)0x2D2D, buffer[i]);
    }

    scheduler.generateSamples(buffer, 128);
    TEST_ASSERT_TRUE(bufferHasNonZero(buffer, 128));
    TEST_ASSERT_EQUAL(0u, scheduler.core().getDroppedCommands());
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
    TEST_ASSERT_TRUE(bufferHasNonZero(buffer, 256));
    TEST_ASSERT_EQUAL(0u, scheduler.core().getDroppedCommands());
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
    TEST_ASSERT_EQUAL(0u, scheduler.core().getDroppedCommands());
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

    TEST_ASSERT_TRUE(bufferHasNonZero(buffer1, 64) || bufferHasNonZero(buffer2, 128)
                     || bufferHasNonZero(buffer3, 256));
    TEST_ASSERT_EQUAL(0u, scheduler.core().getDroppedCommands());
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
    TEST_ASSERT_TRUE(bufferHasNonZero(buffer, 1024));
    TEST_ASSERT_EQUAL(0u, scheduler.core().getDroppedCommands());
}

void test_audio_scheduler_pause_resume_functionality(void) {
    DefaultAudioScheduler scheduler;
    initScheduler(scheduler);

    AudioCommand playCmd{};
    playCmd.type = AudioCommandType::MUSIC_PLAY;
    playCmd.track = nullptr;
    scheduler.submitCommand(playCmd);

    AudioCommand pauseCmd{};
    pauseCmd.type = AudioCommandType::MUSIC_PAUSE;
    scheduler.submitCommand(pauseCmd);

    int16_t buffer[256];
    scheduler.generateSamples(buffer, 256);
    TEST_ASSERT_TRUE(scheduler.isMusicPlaying());
    TEST_ASSERT_TRUE(scheduler.isMusicPaused());

    AudioCommand resumeCmd{};
    resumeCmd.type = AudioCommandType::MUSIC_RESUME;
    scheduler.submitCommand(resumeCmd);
    scheduler.generateSamples(buffer, 256);
    TEST_ASSERT_FALSE(scheduler.isMusicPaused());
    TEST_ASSERT_FALSE(scheduler.isMusicPlaying());
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
    TEST_ASSERT_TRUE(bufferHasNonZero(buffer, 128));
    TEST_ASSERT_EQUAL(0u, scheduler.core().getDroppedCommands());
}

void test_audio_scheduler_adsr_envelope(void) {
    DefaultAudioScheduler scheduler;
    initScheduler(scheduler);
    
    AudioCommand cmd{};
    cmd.type = AudioCommandType::PLAY_EVENT;
    cmd.event.type = WaveType::PULSE;
    cmd.event.frequency = 440.0f;
    cmd.event.volume = 1.0f;
    cmd.event.duration = 0.05f;
    cmd.event.duty = 0.5f;
    cmd.event.preset = &INSTR_PULSE_LEAD;
    
    scheduler.submitCommand(cmd);
    
    int16_t buffer[256];
    scheduler.generateSamples(buffer, 256);
    
    bool hasNonZero = false;
    for (int i = 0; i < 256; i++) {
        if (buffer[i] != 0) { hasNonZero = true; break; }
    }
    TEST_ASSERT_TRUE(hasNonZero);
}

void test_audio_scheduler_lfo_modulation(void) {
    DefaultAudioScheduler scheduler;
    initScheduler(scheduler);
    
    AudioCommand cmd{};
    cmd.type = AudioCommandType::PLAY_EVENT;
    cmd.event.type = WaveType::TRIANGLE;
    cmd.event.frequency = 220.0f;
    cmd.event.volume = 0.8f;
    cmd.event.duration = 0.05f;
    cmd.event.duty = 0.5f;
    cmd.event.preset = &INSTR_TRIANGLE_LEAD;
    
    scheduler.submitCommand(cmd);
    
    int16_t buffer[512];
    scheduler.generateSamples(buffer, 512);
    
    bool hasNonZero = false;
    for (int i = 0; i < 512; i++) {
        if (buffer[i] != 0) { hasNonZero = true; break; }
    }
    TEST_ASSERT_TRUE(hasNonZero);
}

void test_audio_scheduler_percussion_preset(void) {
    DefaultAudioScheduler scheduler;
    initScheduler(scheduler);
    
    AudioCommand cmd{};
    cmd.type = AudioCommandType::PLAY_EVENT;
    cmd.event.type = WaveType::NOISE;
    cmd.event.frequency = 440.0f;
    cmd.event.volume = 0.5f;
    cmd.event.duration = 0.02f;
    cmd.event.duty = 0.0f;
    cmd.event.preset = &INSTR_SNARE;
    
    scheduler.submitCommand(cmd);
    
    int16_t buffer[256];
    scheduler.generateSamples(buffer, 256);
    
    bool hasNonZero = false;
    for (int i = 0; i < 256; i++) {
        if (buffer[i] != 0) { hasNonZero = true; break; }
    }
    TEST_ASSERT_TRUE(hasNonZero);
}

void test_audio_scheduler_duty_sweep(void) {
    DefaultAudioScheduler scheduler;
    initScheduler(scheduler);
    
    AudioCommand cmd{};
    cmd.type = AudioCommandType::PLAY_EVENT;
    cmd.event.type = WaveType::PULSE;
    cmd.event.frequency = 440.0f;
    cmd.event.volume = 0.5f;
    cmd.event.duration = 0.03f;
    cmd.event.duty = 0.5f;
    cmd.event.preset = &INSTR_PULSE_PAD;
    
    scheduler.submitCommand(cmd);
    
    int16_t buffer[512];
    scheduler.generateSamples(buffer, 512);
    
    bool hasNonZero = false;
    for (int i = 0; i < 512; i++) {
        if (buffer[i] != 0) { hasNonZero = true; break; }
    }
    TEST_ASSERT_TRUE(hasNonZero);
}

void test_apucore_reset(void) {
    DefaultAudioScheduler scheduler;
    initScheduler(scheduler);
    
    AudioCommand cmd{};
    cmd.type = AudioCommandType::PLAY_EVENT;
    cmd.event.type = WaveType::PULSE;
    cmd.event.frequency = 440.0f;
    cmd.event.volume = 1.0f;
    cmd.event.duration = 0.1f;
    cmd.event.duty = 0.5f;
    cmd.event.preset = nullptr;
    
    scheduler.submitCommand(cmd);
    generateDiscard(scheduler, 128);

    scheduler.core().reset();
    
    int16_t buffer[256];
    scheduler.generateSamples(buffer, 256);
    TEST_ASSERT_TRUE(bufferAllSilence(buffer, 256));
}

void test_apucore_sequencer_note_limit(void) {
    DefaultAudioScheduler scheduler;
    initScheduler(scheduler);
    
    scheduler.core().setSequencerNoteLimit(50);
    TEST_ASSERT_EQUAL(50, scheduler.core().getSequencerNoteLimit());
    
    scheduler.core().setSequencerNoteLimit(0);
    TEST_ASSERT_TRUE(scheduler.core().getSequencerNoteLimit() > 32);
    
    scheduler.core().setSequencerNoteLimit(2000);
    TEST_ASSERT_EQUAL(32, scheduler.core().getSequencerNoteLimit());
}

void test_apucore_profile_stats(void) {
    DefaultAudioScheduler scheduler;
    initScheduler(scheduler);
    
    ApuCore::ProfileEntry entries[ApuCore::PROFILE_RING_SIZE];
    uint8_t count = 0;
    scheduler.core().getAndResetProfileStats(entries, count);
    
    TEST_ASSERT_TRUE(count <= ApuCore::PROFILE_RING_SIZE);
}

static void fill_triangle_event(AudioEvent& e) {
    e.type = WaveType::TRIANGLE;
    e.frequency = 400.0f;
    e.volume = 1.0f;
    e.duration = 0.1f;
    e.duty = 0.5f;
    e.noisePeriod = 0;
    e.preset = nullptr;
    e.sweepEndHz = 0.0f;
    e.sweepDurationSec = 0.0f;
}

void test_frequency_sweep_changes_output(void) {
    int16_t withSweep[2048];
    int16_t noSweep[2048];
    DefaultAudioScheduler s1;
    DefaultAudioScheduler s2;
    initScheduler(s1);
    initScheduler(s2);

    AudioCommand c1{};
    c1.type = AudioCommandType::PLAY_EVENT;
    fill_triangle_event(c1.event);
    c1.event.sweepEndHz = 3200.0f;
    c1.event.sweepDurationSec = 0.08f;

    AudioCommand c2{};
    c2.type = AudioCommandType::PLAY_EVENT;
    fill_triangle_event(c2.event);

    s1.submitCommand(c1);
    s2.submitCommand(c2);
    s1.generateSamples(withSweep, 2048);
    s2.generateSamples(noSweep, 2048);

    int diff = 0;
    for (int i = 0; i < 2048; ++i) {
        if (withSweep[i] != noSweep[i]) diff++;
    }
    TEST_ASSERT_GREATER_THAN(200, diff);
}

void test_frequency_sweep_descending_changes_output(void) {
    int16_t withSweep[2048];
    int16_t noSweep[2048];
    DefaultAudioScheduler s1;
    DefaultAudioScheduler s2;
    initScheduler(s1);
    initScheduler(s2);

    AudioCommand c1{};
    c1.type = AudioCommandType::PLAY_EVENT;
    fill_triangle_event(c1.event);
    c1.event.frequency = 3200.0f;
    c1.event.sweepEndHz = 400.0f;
    c1.event.sweepDurationSec = 0.08f;

    AudioCommand c2{};
    c2.type = AudioCommandType::PLAY_EVENT;
    fill_triangle_event(c2.event);
    c2.event.frequency = 3200.0f;

    s1.submitCommand(c1);
    s2.submitCommand(c2);
    s1.generateSamples(withSweep, 2048);
    s2.generateSamples(noSweep, 2048);

    int diff = 0;
    for (int i = 0; i < 2048; ++i) {
        if (withSweep[i] != noSweep[i]) diff++;
    }
    TEST_ASSERT_GREATER_THAN(200, diff);
}

void test_sweep_longer_than_note_truncates_to_note_length(void) {
    int16_t longReq[1024];
    int16_t shortReq[1024];
    DefaultAudioScheduler sLong;
    DefaultAudioScheduler sShort;
    initScheduler(sLong);
    initScheduler(sShort);

    const float durSec = 0.02f;

    AudioCommand cLong{};
    cLong.type = AudioCommandType::PLAY_EVENT;
    fill_triangle_event(cLong.event);
    cLong.event.duration = durSec;
    cLong.event.sweepEndHz = 3200.0f;
    cLong.event.sweepDurationSec = 2.0f;

    AudioCommand cShort{};
    cShort.type = AudioCommandType::PLAY_EVENT;
    fill_triangle_event(cShort.event);
    cShort.event.duration = durSec;
    cShort.event.sweepEndHz = 3200.0f;
    cShort.event.sweepDurationSec = durSec;

    sLong.submitCommand(cLong);
    sShort.submitCommand(cShort);
    sLong.generateSamples(longReq, 1024);
    sShort.generateSamples(shortReq, 1024);

    for (int i = 0; i < 1024; ++i) {
        TEST_ASSERT_EQUAL(longReq[i], shortReq[i]);
    }
}

void test_sweep_duration_zero_matches_no_sweep(void) {
    int16_t a[1024];
    int16_t b[1024];
    DefaultAudioScheduler s1;
    DefaultAudioScheduler s2;
    initScheduler(s1);
    initScheduler(s2);

    AudioCommand c1{};
    c1.type = AudioCommandType::PLAY_EVENT;
    fill_triangle_event(c1.event);
    c1.event.sweepEndHz = 2000.0f;
    c1.event.sweepDurationSec = 0.0f;

    AudioCommand c2{};
    c2.type = AudioCommandType::PLAY_EVENT;
    fill_triangle_event(c2.event);

    s1.submitCommand(c1);
    s2.submitCommand(c2);
    s1.generateSamples(a, 1024);
    s2.generateSamples(b, 1024);
    for (int i = 0; i < 1024; ++i) {
        TEST_ASSERT_EQUAL(a[i], b[i]);
    }
}

void test_master_bitcrush_changes_output(void) {
    int16_t offBuf[512];
    int16_t onBuf[512];
    DefaultAudioScheduler sOff;
    DefaultAudioScheduler sOn;
    initScheduler(sOff);
    initScheduler(sOn);

    AudioCommand z{};
    z.type = AudioCommandType::SET_MASTER_BITCRUSH;
    z.masterBitcrushBits = 0;
    sOff.submitCommand(z);

    AudioCommand bc{};
    bc.type = AudioCommandType::SET_MASTER_BITCRUSH;
    bc.masterBitcrushBits = 4;
    sOn.submitCommand(bc);

    AudioCommand play{};
    play.type = AudioCommandType::PLAY_EVENT;
    play.event.type = WaveType::PULSE;
    play.event.frequency = 900.0f;
    play.event.volume = 1.0f;
    play.event.duration = 0.05f;
    play.event.duty = 0.5f;
    play.event.preset = nullptr;

    sOff.submitCommand(play);
    sOn.submitCommand(play);
    sOff.generateSamples(offBuf, 512);
    sOn.generateSamples(onBuf, 512);

    int diff = 0;
    for (int i = 0; i < 512; ++i) {
        if (offBuf[i] != onBuf[i]) diff++;
    }
    TEST_ASSERT_GREATER_THAN(50, diff);
}

void test_saw_wave_differs_from_pulse(void) {
    int16_t sawBuf[512];
    int16_t pulseBuf[512];
    DefaultAudioScheduler sSaw;
    DefaultAudioScheduler sPulse;
    initScheduler(sSaw);
    initScheduler(sPulse);

    AudioCommand cSaw{};
    cSaw.type = AudioCommandType::PLAY_EVENT;
    cSaw.event.type = WaveType::SAW;
    cSaw.event.frequency = 440.0f;
    cSaw.event.volume = 1.0f;
    cSaw.event.duration = 0.1f;
    cSaw.event.duty = 0.5f;

    AudioCommand cPulse{};
    cPulse.type = AudioCommandType::PLAY_EVENT;
    cPulse.event.type = WaveType::PULSE;
    cPulse.event.frequency = 440.0f;
    cPulse.event.volume = 1.0f;
    cPulse.event.duration = 0.1f;
    cPulse.event.duty = 0.5f;

    sSaw.submitCommand(cSaw);
    sPulse.submitCommand(cPulse);
    sSaw.generateSamples(sawBuf, 512);
    sPulse.generateSamples(pulseBuf, 512);

    int diff = 0;
    for (int i = 0; i < 512; ++i) {
        if (sawBuf[i] != pulseBuf[i]) diff++;
    }
    TEST_ASSERT_GREATER_THAN(100, diff);
}

void test_sine_wave_differs_from_triangle(void) {
    int16_t sineBuf[512];
    int16_t triBuf[512];
    DefaultAudioScheduler s1;
    DefaultAudioScheduler s2;
    initScheduler(s1);
    initScheduler(s2);

    AudioCommand c1{};
    c1.type = AudioCommandType::PLAY_EVENT;
    c1.event.type = WaveType::SINE;
    c1.event.frequency = 330.0f;
    c1.event.volume = 1.0f;
    c1.event.duration = 0.1f;
    c1.event.duty = 0.5f;

    AudioCommand c2{};
    c2.type = AudioCommandType::PLAY_EVENT;
    c2.event.type = WaveType::TRIANGLE;
    c2.event.frequency = 330.0f;
    c2.event.volume = 1.0f;
    c2.event.duration = 0.1f;
    c2.event.duty = 0.5f;

    s1.submitCommand(c1);
    s2.submitCommand(c2);
    s1.generateSamples(sineBuf, 512);
    s2.generateSamples(triBuf, 512);

    int diff = 0;
    for (int i = 0; i < 512; ++i) {
        if (sineBuf[i] != triBuf[i]) diff++;
    }
    TEST_ASSERT_GREATER_THAN(100, diff);
}

void test_master_bitcrush_zero_matches_default_baseline(void) {
    int16_t baseline[512];
    int16_t explicitZero[512];
    DefaultAudioScheduler sBase;
    DefaultAudioScheduler sZero;
    initScheduler(sBase);
    initScheduler(sZero);

    AudioCommand z{};
    z.type = AudioCommandType::SET_MASTER_BITCRUSH;
    z.masterBitcrushBits = 0;
    sZero.submitCommand(z);

    AudioCommand play{};
    play.type = AudioCommandType::PLAY_EVENT;
    play.event.type = WaveType::PULSE;
    play.event.frequency = 880.0f;
    play.event.volume = 1.0f;
    play.event.duration = 0.05f;
    play.event.duty = 0.5f;
    play.event.preset = nullptr;

    sBase.submitCommand(play);
    sZero.submitCommand(play);
    sBase.generateSamples(baseline, 512);
    sZero.generateSamples(explicitZero, 512);

    for (int i = 0; i < 512; ++i) {
        TEST_ASSERT_EQUAL(baseline[i], explicitZero[i]);
    }
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
    
    // Edge case tests
    RUN_TEST(test_audio_scheduler_zero_duration_sound);
    RUN_TEST(test_audio_scheduler_exact_buffer_boundary);
    RUN_TEST(test_audio_scheduler_rapid_schedules);
    RUN_TEST(test_audio_scheduler_pause_resume_functionality);
    RUN_TEST(test_audio_scheduler_volume_interpolation);
    
    // ADSR/LFO coverage tests
    RUN_TEST(test_audio_scheduler_adsr_envelope);
    RUN_TEST(test_audio_scheduler_lfo_modulation);
    RUN_TEST(test_audio_scheduler_percussion_preset);
    RUN_TEST(test_audio_scheduler_duty_sweep);
    
    // ApuCore internals coverage
    RUN_TEST(test_apucore_reset);
    RUN_TEST(test_apucore_sequencer_note_limit);
    RUN_TEST(test_apucore_profile_stats);
    RUN_TEST(test_frequency_sweep_changes_output);
    RUN_TEST(test_frequency_sweep_descending_changes_output);
    RUN_TEST(test_sweep_longer_than_note_truncates_to_note_length);
    RUN_TEST(test_sweep_duration_zero_matches_no_sweep);
    RUN_TEST(test_master_bitcrush_changes_output);
    RUN_TEST(test_saw_wave_differs_from_pulse);
    RUN_TEST(test_sine_wave_differs_from_triangle);
    RUN_TEST(test_master_bitcrush_zero_matches_default_baseline);

    // Error handling / stress (after stable ApuCore tests)
    RUN_TEST(test_audio_scheduler_buffer_underrun_recovery);
    RUN_TEST(test_audio_scheduler_shutdown_during_active_playback);
    RUN_TEST(test_audio_scheduler_null_buffer_handling);
    RUN_TEST(test_audio_scheduler_command_queue_overflow);

    // Init-failure must stay last (see comment in prior revision).
    RUN_TEST(test_audio_scheduler_initialization_failure_handling);

    return UNITY_END();
}
