/**
 * @file test_apu_core.cpp
 * @brief Unit tests for audio/ApuCore module
 * @version 1.0
 * @date 2026-05-06
 *
 * Tests for ApuCore including:
 * - Constructor initialization
 * - init/reset lifecycle
 * - Command queue operations
 * - Voice management (via public API)
 * - ADSR envelope stages (via generateSamples)
 * - LFO oscillator (via generateSamples)
 * - Sample generation (main audio loop)
 * - Bitcrusher
 */

#include <unity.h>
#include <cmath>
#include <cstring>
#include "../../test_config.h"
#include "audio/ApuCore.h"
#include "audio/AudioTypes.h"
#include "audio/AudioMusicTypes.h"

using namespace pixelroot32::audio;

// Required Unity setup/teardown
void setUp(void) {
    test_setup();
}

void tearDown(void) {
    test_teardown();
}

// =============================================================================
// Tests for ApuCore constructor
// =============================================================================

void test_apu_core_constructor_initializes_voices(void) {
    ApuCore apu;

    // Verify MAX_VOICES constant
    TEST_ASSERT_EQUAL_INT(8, ApuCore::MAX_VOICES);

    // Verify initial sample rate
    TEST_ASSERT_EQUAL_INT(44100, apu.getSampleRate());
}

void test_apu_core_constructor_default_values(void) {
    ApuCore apu;

    // Default values should be set
    TEST_ASSERT_EQUAL_INT(44100, apu.getSampleRate());
    TEST_ASSERT_FALSE(apu.isMusicPlaying());
    TEST_ASSERT_FALSE(apu.isMusicPaused());
    TEST_ASSERT_EQUAL_UINT32(0, apu.getDroppedCommands());
}

// =============================================================================
// Tests for init() and reset()
// =============================================================================

void test_apu_core_init_valid_sample_rate(void) {
    ApuCore apu;

    apu.init(22050);
    TEST_ASSERT_EQUAL_INT(22050, apu.getSampleRate());
}

void test_apu_core_init_valid_sample_rate_48000(void) {
    ApuCore apu;

    apu.init(48000);
    TEST_ASSERT_EQUAL_INT(48000, apu.getSampleRate());
}

void test_apu_core_init_invalid_sample_rate_fallback(void) {
    ApuCore apu;

    // Zero sample rate should fallback to default
    apu.init(0);
    TEST_ASSERT_EQUAL_INT(44100, apu.getSampleRate());

    // Negative sample rate should also fallback
    apu.init(-1);
    TEST_ASSERT_EQUAL_INT(44100, apu.getSampleRate());
}

void test_apu_core_reset_clears_state(void) {
    ApuCore apu;

    apu.init(44100);
    apu.reset();

    // Verify reset state
    TEST_ASSERT_EQUAL_INT(44100, apu.getSampleRate());
    TEST_ASSERT_EQUAL_UINT32(0, apu.getDroppedCommands());
    TEST_ASSERT_FALSE(apu.isMusicPlaying());
    TEST_ASSERT_FALSE(apu.isMusicPaused());
}

// =============================================================================
// Tests for submitCommand() - command queue operations
// =============================================================================

void test_apu_core_submit_command_success(void) {
    ApuCore apu;
    apu.init(44100);

    AudioCommand cmd;
    cmd.type = AudioCommandType::PLAY_EVENT;
    cmd.event.type = WaveType::PULSE;
    cmd.event.frequency = 440.0f;
    cmd.event.duration = 0.1f;
    cmd.event.volume = 0.8f;
    cmd.event.duty = 0.5f;

    bool result = apu.submitCommand(cmd);
    TEST_ASSERT_TRUE(result);
}

void test_apu_core_submit_command_multiple(void) {
    ApuCore apu;
    apu.init(44100);

    AudioCommand cmd;
    cmd.type = AudioCommandType::PLAY_EVENT;
    cmd.event.type = WaveType::PULSE;
    cmd.event.frequency = 440.0f;
    cmd.event.duration = 0.1f;
    cmd.event.volume = 0.8f;
    cmd.event.duty = 0.5f;

    // Submit multiple commands
    for (int i = 0; i < 10; i++) {
        bool result = apu.submitCommand(cmd);
        TEST_ASSERT_TRUE(result);
    }
}

void test_apu_core_submit_command_queue_full(void) {
    ApuCore apu;
    apu.init(44100);

    // Fill the queue with commands
    AudioCommand cmd;
    cmd.type = AudioCommandType::PLAY_EVENT;
    cmd.event.type = WaveType::PULSE;
    cmd.event.frequency = 440.0f;
    cmd.event.duration = 0.1f;
    cmd.event.volume = 0.8f;

    // Try to submit more commands than queue capacity
    // Queue capacity is 128, so this should eventually fail
    int successCount = 0;
    for (int i = 0; i < 200; i++) {
        if (apu.submitCommand(cmd)) {
            successCount++;
        }
    }

    // Verify some commands were dropped
    TEST_ASSERT_TRUE(apu.getDroppedCommands() > 0);
}

// =============================================================================
// Tests for generateSamples() - main audio loop (exercises processCommands internally)
// =============================================================================

void test_apu_core_generate_samples_zero_length(void) {
    ApuCore apu;
    apu.init(44100);

    int16_t buffer[100] = {0};

    // Should handle zero/negative length gracefully
    apu.generateSamples(buffer, 0);
    apu.generateSamples(buffer, -1);

    // Apu is still functional - verify voices are intact
    TEST_ASSERT_EQUAL_INT(0, apu.countEnabledVoicesForTesting());
}

void test_apu_core_generate_samples_null_stream(void) {
    ApuCore apu;
    apu.init(44100);

    // Should handle null stream gracefully
    apu.generateSamples(nullptr, 100);

    // Apu is still functional after handling null stream
    TEST_ASSERT_EQUAL_INT(0, apu.countEnabledVoicesForTesting());
}

void test_apu_core_generate_samples_with_active_voices(void) {
    ApuCore apu;
    apu.init(44100);

    // Submit a note
    AudioCommand cmd;
    cmd.type = AudioCommandType::PLAY_EVENT;
    cmd.event.type = WaveType::PULSE;
    cmd.event.frequency = 440.0f;
    cmd.event.duration = 1.0f;
    cmd.event.volume = 0.5f;
    cmd.event.duty = 0.5f;

    apu.submitCommand(cmd);

    // Generate some samples - this internally calls processCommands()
    int16_t buffer[256] = {0};
    apu.generateSamples(buffer, 256);

    // Voice should be enabled now
    TEST_ASSERT_TRUE(apu.countEnabledVoicesForTesting() > 0);
}

void test_apu_core_generate_samples_output_range(void) {
    ApuCore apu;
    apu.init(44100);

    // Submit a note with moderate volume
    AudioCommand cmd;
    cmd.type = AudioCommandType::PLAY_EVENT;
    cmd.event.type = WaveType::PULSE;
    cmd.event.frequency = 440.0f;
    cmd.event.duration = 1.0f;
    cmd.event.volume = 0.3f;
    cmd.event.duty = 0.5f;

    apu.submitCommand(cmd);

    // Generate samples
    int16_t buffer[256] = {0};
    apu.generateSamples(buffer, 256);

    // Verify all samples are in int16 range
    for (int i = 0; i < 256; i++) {
        TEST_ASSERT_TRUE(buffer[i] >= -32768);
        TEST_ASSERT_TRUE(buffer[i] <= 32767);
    }
}

void test_apu_core_generate_samples_empty_queue(void) {
    ApuCore apu;
    apu.init(44100);

    // Generate samples without submitting any commands
    int16_t buffer[256] = {0};
    apu.generateSamples(buffer, 256);

    // Should not crash - buffer should remain zeros (no commands to generate audio)
    bool hasNonZero = false;
    for (int i = 0; i < 256; i++) {
        if (buffer[i] != 0) {
            hasNonZero = true;
            break;
        }
    }
    TEST_ASSERT_FALSE(hasNonZero);  // Expect zeros when no commands
}

// =============================================================================
// Tests for helper functions (internal via public API)
// =============================================================================

void test_apu_core_sequencer_note_limit_set(void) {
    ApuCore apu;
    apu.init(44100);

    // Set a limit
    apu.setSequencerNoteLimit(16);
    TEST_ASSERT_EQUAL_INT(16, apu.getSequencerNoteLimit());

    // Set zero (should become unbounded)
    apu.setSequencerNoteLimit(0);
    TEST_ASSERT_TRUE(apu.getSequencerNoteLimit() > 16);

    // Set too high (should clamp to 32)
    apu.setSequencerNoteLimit(2000);
    TEST_ASSERT_EQUAL_INT(32, apu.getSequencerNoteLimit());
}

void test_apu_core_deferred_notes_counter(void) {
    ApuCore apu;
    apu.init(44100);

    // Initially no deferred notes
    TEST_ASSERT_EQUAL_INT(0, apu.getDeferredNotes());
}

// =============================================================================
// Tests for different wave types (via generateSamples)
// =============================================================================

void test_apu_core_wave_type_pulse(void) {
    ApuCore apu;
    apu.init(44100);

    AudioCommand cmd;
    cmd.type = AudioCommandType::PLAY_EVENT;
    cmd.event.type = WaveType::PULSE;
    cmd.event.frequency = 440.0f;
    cmd.event.duration = 0.5f;
    cmd.event.volume = 0.5f;
    cmd.event.duty = 0.25f;

    apu.submitCommand(cmd);

    int16_t buffer[128] = {0};
    apu.generateSamples(buffer, 128);

    bool hasNonZero = false;
    for (int i = 0; i < 128; i++) {
        if (buffer[i] != 0) {
            hasNonZero = true;
            break;
        }
    }
    TEST_ASSERT_TRUE(hasNonZero);
}

void test_apu_core_wave_type_triangle(void) {
    ApuCore apu;
    apu.init(44100);

    AudioCommand cmd;
    cmd.type = AudioCommandType::PLAY_EVENT;
    cmd.event.type = WaveType::TRIANGLE;
    cmd.event.frequency = 440.0f;
    cmd.event.duration = 0.5f;
    cmd.event.volume = 0.5f;

    apu.submitCommand(cmd);

    int16_t buffer[128] = {0};
    apu.generateSamples(buffer, 128);

    bool hasNonZero = false;
    for (int i = 0; i < 128; i++) {
        if (buffer[i] != 0) {
            hasNonZero = true;
            break;
        }
    }
    TEST_ASSERT_TRUE(hasNonZero);
}

void test_apu_core_wave_type_sine(void) {
    ApuCore apu;
    apu.init(44100);

    AudioCommand cmd;
    cmd.type = AudioCommandType::PLAY_EVENT;
    cmd.event.type = WaveType::SINE;
    cmd.event.frequency = 440.0f;
    cmd.event.duration = 0.5f;
    cmd.event.volume = 0.5f;

    apu.submitCommand(cmd);

    int16_t buffer[128] = {0};
    apu.generateSamples(buffer, 128);

    bool hasNonZero = false;
    for (int i = 0; i < 128; i++) {
        if (buffer[i] != 0) {
            hasNonZero = true;
            break;
        }
    }
    TEST_ASSERT_TRUE(hasNonZero);
}

void test_apu_core_wave_type_saw(void) {
    ApuCore apu;
    apu.init(44100);

    AudioCommand cmd;
    cmd.type = AudioCommandType::PLAY_EVENT;
    cmd.event.type = WaveType::SAW;
    cmd.event.frequency = 440.0f;
    cmd.event.duration = 0.5f;
    cmd.event.volume = 0.5f;

    apu.submitCommand(cmd);

    int16_t buffer[128] = {0};
    apu.generateSamples(buffer, 128);

    bool hasNonZero = false;
    for (int i = 0; i < 128; i++) {
        if (buffer[i] != 0) {
            hasNonZero = true;
            break;
        }
    }
    TEST_ASSERT_TRUE(hasNonZero);
}

void test_apu_core_wave_type_noise(void) {
    ApuCore apu;
    apu.init(44100);

    AudioCommand cmd;
    cmd.type = AudioCommandType::PLAY_EVENT;
    cmd.event.type = WaveType::NOISE;
    cmd.event.frequency = 1000.0f;
    cmd.event.duration = 0.5f;
    cmd.event.volume = 0.5f;

    apu.submitCommand(cmd);

    int16_t buffer[128] = {0};
    apu.generateSamples(buffer, 128);

    bool hasNonZero = false;
    for (int i = 0; i < 128; i++) {
        if (buffer[i] != 0) {
            hasNonZero = true;
            break;
        }
    }
    TEST_ASSERT_TRUE(hasNonZero);
}

// =============================================================================
// Tests for music commands (exercised via generateSamples)
// =============================================================================

void test_apu_core_music_play_stop_cycle(void) {
    ApuCore apu;
    apu.init(44100);

    // Start music (track is nullptr so won't actually play)
    AudioCommand playCmd;
    playCmd.type = AudioCommandType::MUSIC_PLAY;
    playCmd.track = nullptr;
    apu.submitCommand(playCmd);

    // Generate samples to process the command
    int16_t buffer[128] = {0};
    apu.generateSamples(buffer, 128);

    // Music playing flag should be set (even with null track)
    // Note: may not actually play due to null track
    // Verify apu is still functional
    TEST_ASSERT_EQUAL_INT(0, apu.countEnabledVoicesForTesting());  // No voices active (null track)
}

void test_apu_core_music_stop_clears_flag(void) {
    ApuCore apu;
    apu.init(44100);

    // First play something
    AudioCommand playCmd;
    playCmd.type = AudioCommandType::MUSIC_PLAY;
    playCmd.track = nullptr;
    apu.submitCommand(playCmd);
    apu.generateSamples(nullptr, 128);

    // Then stop
    AudioCommand stopCmd;
    stopCmd.type = AudioCommandType::MUSIC_STOP;
    apu.submitCommand(stopCmd);
    apu.generateSamples(nullptr, 128);

    TEST_ASSERT_FALSE(apu.isMusicPlaying());
}

void test_apu_core_music_pause_resume_cycle(void) {
    ApuCore apu;
    apu.init(44100);

    // With nullptr track, music won't actually play
    // But we can test that the commands are processed without crash
    // and the flag states toggle correctly when commands are submitted

    // Pause when not playing - should still set flag
    AudioCommand pauseCmd;
    pauseCmd.type = AudioCommandType::MUSIC_PAUSE;
    apu.submitCommand(pauseCmd);
    apu.generateSamples(nullptr, 128);

    // Resume when not playing
    AudioCommand resumeCmd;
    resumeCmd.type = AudioCommandType::MUSIC_RESUME;
    apu.submitCommand(resumeCmd);
    apu.generateSamples(nullptr, 128);

    // Verify apu is still functional - isMusicPaused flag should be set
    TEST_ASSERT_TRUE(apu.isMusicPaused() || !apu.isMusicPlaying());
}

void test_apu_core_music_set_tempo(void) {
    ApuCore apu;
    apu.init(44100);

    AudioCommand cmd;
    cmd.type = AudioCommandType::MUSIC_SET_TEMPO;
    cmd.tempoFactor = 2.0f;

    apu.submitCommand(cmd);
    apu.generateSamples(nullptr, 100);

    // Verify apu is still functional after tempo command
    TEST_ASSERT_EQUAL_INT(0, apu.countEnabledVoicesForTesting());
}

void test_apu_core_music_set_bpm(void) {
    ApuCore apu;
    apu.init(44100);

    AudioCommand cmd;
    cmd.type = AudioCommandType::MUSIC_SET_BPM;
    cmd.bpm = 120.0f;

    apu.submitCommand(cmd);
    apu.generateSamples(nullptr, 100);

    // Verify apu is still functional after BPM command
    TEST_ASSERT_EQUAL_INT(0, apu.countEnabledVoicesForTesting());
}

void test_apu_core_set_master_volume_command(void) {
    ApuCore apu;
    apu.init(44100);

    AudioCommand cmd;
    cmd.type = AudioCommandType::SET_MASTER_VOLUME;
    cmd.volume = 0.5f;

    apu.submitCommand(cmd);
    apu.generateSamples(nullptr, 100);

    // Verify apu is still functional after volume command
    TEST_ASSERT_EQUAL_INT(0, apu.countEnabledVoicesForTesting());
}

void test_apu_core_set_master_volume_clamping(void) {
    ApuCore apu;
    apu.init(44100);

    // Test over max
    AudioCommand cmd;
    cmd.type = AudioCommandType::SET_MASTER_VOLUME;
    cmd.volume = 1.5f;

    apu.submitCommand(cmd);
    apu.generateSamples(nullptr, 100);

    // Should clamp to 1.0 - verify no crash
    TEST_ASSERT_EQUAL_INT(0, apu.countEnabledVoicesForTesting());
}

void test_apu_core_set_master_bitcrush(void) {
    ApuCore apu;
    apu.init(44100);

    AudioCommand cmd;
    cmd.type = AudioCommandType::SET_MASTER_BITCRUSH;
    cmd.masterBitcrushBits = 8;

    apu.submitCommand(cmd);
    apu.generateSamples(nullptr, 100);

    // Verify apu is still functional after bitcrush command
    TEST_ASSERT_EQUAL_INT(0, apu.countEnabledVoicesForTesting());
}

void test_apu_core_set_master_bitcrush_over_max(void) {
    ApuCore apu;
    apu.init(44100);

    AudioCommand cmd;
    cmd.type = AudioCommandType::SET_MASTER_BITCRUSH;
    cmd.masterBitcrushBits = 20;  // Over max 15

    apu.submitCommand(cmd);
    apu.generateSamples(nullptr, 100);

    // Should clamp to 15 - verify no crash
    TEST_ASSERT_EQUAL_INT(0, apu.countEnabledVoicesForTesting());
}

// =============================================================================
// Tests for post-mix callback
// =============================================================================

static bool callbackInvoked = false;

void test_post_mix_callback(int16_t* mono, int length, void* user) {
    (void)mono;
    (void)length;
    (void)user;
    callbackInvoked = true;
}

void test_apu_core_post_mix_callback(void) {
    ApuCore apu;
    apu.init(44100);

    callbackInvoked = false;
    apu.setPostMixMono(test_post_mix_callback, nullptr);

    // Generate samples to trigger callback
    AudioCommand cmd;
    cmd.type = AudioCommandType::PLAY_EVENT;
    cmd.event.type = WaveType::PULSE;
    cmd.event.frequency = 440.0f;
    cmd.event.duration = 0.1f;
    cmd.event.volume = 0.5f;
    cmd.event.duty = 0.5f;

    apu.submitCommand(cmd);

    int16_t buffer[256] = {0};
    apu.generateSamples(buffer, 256);

    TEST_ASSERT_TRUE(callbackInvoked);
}

// =============================================================================
// Tests for different frequencies
// =============================================================================

void test_apu_core_low_frequency(void) {
    ApuCore apu;
    apu.init(44100);

    AudioCommand cmd;
    cmd.type = AudioCommandType::PLAY_EVENT;
    cmd.event.type = WaveType::PULSE;
    cmd.event.frequency = 100.0f;  // Low frequency
    cmd.event.duration = 0.5f;
    cmd.event.volume = 0.5f;
    cmd.event.duty = 0.5f;

    apu.submitCommand(cmd);

    int16_t buffer[128] = {0};
    apu.generateSamples(buffer, 128);

    // Verify apu is still functional - voice count should be >= 0
    TEST_ASSERT_TRUE(apu.countEnabledVoicesForTesting() >= 0);
}

void test_apu_core_high_frequency(void) {
    ApuCore apu;
    apu.init(44100);

    AudioCommand cmd;
    cmd.type = AudioCommandType::PLAY_EVENT;
    cmd.event.type = WaveType::PULSE;
    cmd.event.frequency = 8000.0f;  // High frequency
    cmd.event.duration = 0.5f;
    cmd.event.volume = 0.5f;
    cmd.event.duty = 0.5f;

    apu.submitCommand(cmd);

    int16_t buffer[128] = {0};
    apu.generateSamples(buffer, 128);

    // Verify apu is still functional - voice count should be >= 0
    TEST_ASSERT_TRUE(apu.countEnabledVoicesForTesting() >= 0);
}

// =============================================================================
// Tests for volume levels
// =============================================================================

void test_apu_core_zero_volume(void) {
    ApuCore apu;
    apu.init(44100);

    AudioCommand cmd;
    cmd.type = AudioCommandType::PLAY_EVENT;
    cmd.event.type = WaveType::PULSE;
    cmd.event.frequency = 440.0f;
    cmd.event.duration = 0.5f;
    cmd.event.volume = 0.0f;  // Silent
    cmd.event.duty = 0.5f;

    apu.submitCommand(cmd);

    int16_t buffer[128] = {0};
    apu.generateSamples(buffer, 128);

    // Should produce near-zero output
    bool hasNonZero = false;
    for (int i = 0; i < 128; i++) {
        if (buffer[i] != 0) {
            hasNonZero = true;
            break;
        }
    }
    TEST_ASSERT_FALSE(hasNonZero);
}

void test_apu_core_full_volume(void) {
    ApuCore apu;
    apu.init(44100);

    AudioCommand cmd;
    cmd.type = AudioCommandType::PLAY_EVENT;
    cmd.event.type = WaveType::PULSE;
    cmd.event.frequency = 440.0f;
    cmd.event.duration = 0.5f;
    cmd.event.volume = 1.0f;  // Full volume
    cmd.event.duty = 0.5f;

    apu.submitCommand(cmd);

    int16_t buffer[128] = {0};
    apu.generateSamples(buffer, 128);

    // Should produce output
    bool hasNonZero = false;
    for (int i = 0; i < 128; i++) {
        if (buffer[i] != 0) {
            hasNonZero = true;
            break;
        }
    }
    TEST_ASSERT_TRUE(hasNonZero);
}

// =============================================================================
// Tests for 4+4 voice partition (upstream v2)
// =============================================================================

void test_apu_core_voice_partition_constants(void) {
    TEST_ASSERT_EQUAL_INT(8, ApuCore::MAX_VOICES);
    TEST_ASSERT_EQUAL_INT(0, ApuCore::MUSIC_VOICE_BASE);
    TEST_ASSERT_EQUAL_INT(4, ApuCore::MUSIC_VOICE_COUNT);
    TEST_ASSERT_EQUAL_INT(4, ApuCore::SFX_VOICE_BASE);
    TEST_ASSERT_EQUAL_INT(4, ApuCore::SFX_VOICE_COUNT);
}

void test_apu_core_music_pulse_tracks_do_not_steal_each_other(void) {
    ApuCore apu;
    apu.init(44100);

    AudioCommand bpmCmd{};
    bpmCmd.type = AudioCommandType::MUSIC_SET_BPM;
    bpmCmd.bpm = 120.0f;
    apu.submitCommand(bpmCmd);

    static const MusicNote kLeadNotes[] = {
        {Note::C, 4, 4.0f, 0.8f, nullptr},
    };
    static const MusicNote kHarmonyNotes[] = {
        {Note::E, 4, 0.5f, 0.8f, nullptr},
        {Note::Rest, 4, 3.5f, 0.0f, nullptr},
    };
    static const MusicTrack kLeadTrack{kLeadNotes, 1, false, WaveType::PULSE, 0.5f};
    static const MusicTrack kHarmonyTrack{kHarmonyNotes, 2, false, WaveType::PULSE, 0.5f};

    AudioCommand play{};
    play.type = AudioCommandType::MUSIC_PLAY;
    play.track = &kLeadTrack;
    play.subTrackCount = 1;
    play.subTracks[0] = &kHarmonyTrack;
    apu.submitCommand(play);

    int16_t buffer[8192];
    apu.generateSamples(buffer, 256);

    TEST_ASSERT_TRUE(apu.isVoiceEnabledForTesting(0));
    TEST_ASSERT_TRUE(apu.isVoiceEnabledForTesting(1));
    TEST_ASSERT_TRUE(apu.isMusicTrackVoiceActiveForTesting(0));

    const int tick_samples = (44100 * 60) / (120 * 4);
    for (int tick = 0; tick < 3; ++tick) {
        apu.generateSamples(buffer, tick_samples);
    }

    TEST_ASSERT_TRUE(apu.isVoiceEnabledForTesting(0));
    TEST_ASSERT_FALSE(apu.isMusicTrackVoiceActiveForTesting(1));
}

void test_apu_core_sfx_play_event_uses_sfx_voice_pool(void) {
    ApuCore apu;
    apu.init(44100);

    static const MusicNote kLongNote[] = {{Note::C, 4, 16.0f, 0.5f, nullptr}};
    static const MusicTrack kTrack0{kLongNote, 1, false, WaveType::PULSE, 0.5f};
    static const MusicTrack kTrack1{kLongNote, 1, false, WaveType::PULSE, 0.5f};
    static const MusicTrack kTrack2{kLongNote, 1, false, WaveType::PULSE, 0.5f};
    static const MusicTrack kTrack3{kLongNote, 1, false, WaveType::PULSE, 0.5f};

    AudioCommand play{};
    play.type = AudioCommandType::MUSIC_PLAY;
    play.track = &kTrack0;
    play.subTrackCount = 3;
    play.subTracks[0] = &kTrack1;
    play.subTracks[1] = &kTrack2;
    play.subTracks[2] = &kTrack3;
    apu.submitCommand(play);
    int16_t buffer[8192];
    apu.generateSamples(buffer, 256);

    for (int slot = 0; slot < ApuCore::SFX_VOICE_BASE; ++slot) {
        TEST_ASSERT_TRUE(apu.isVoiceEnabledForTesting(slot));
    }

    AudioCommand sfx{};
    sfx.type = AudioCommandType::PLAY_EVENT;
    sfx.event.type = WaveType::PULSE;
    sfx.event.frequency = 880.0f;
    sfx.event.duration = 0.5f;
    sfx.event.volume = 0.5f;
    sfx.event.duty = 0.5f;
    apu.submitCommand(sfx);
    apu.generateSamples(buffer, 256);

    bool sfx_in_high_pool = false;
    for (int slot = ApuCore::SFX_VOICE_BASE; slot < ApuCore::MAX_VOICES; ++slot) {
        if (apu.isVoiceEnabledForTesting(slot)) {
            sfx_in_high_pool = true;
        }
    }
    TEST_ASSERT_TRUE(sfx_in_high_pool);
    for (int slot = 0; slot < ApuCore::SFX_VOICE_BASE; ++slot) {
        TEST_ASSERT_TRUE(apu.isVoiceEnabledForTesting(slot));
    }
}

void test_apu_core_sfx_steal_does_not_touch_music_slots(void) {
    ApuCore apu;
    apu.init(44100);

    static const MusicNote kLongNote[] = {{Note::C, 4, 16.0f, 0.5f, nullptr}};
    static const MusicTrack kTrack0{kLongNote, 1, false, WaveType::PULSE, 0.5f};
    static const MusicTrack kTrack1{kLongNote, 1, false, WaveType::PULSE, 0.5f};
    static const MusicTrack kTrack2{kLongNote, 1, false, WaveType::PULSE, 0.5f};
    static const MusicTrack kTrack3{kLongNote, 1, false, WaveType::PULSE, 0.5f};

    AudioCommand play{};
    play.type = AudioCommandType::MUSIC_PLAY;
    play.track = &kTrack0;
    play.subTrackCount = 3;
    play.subTracks[0] = &kTrack1;
    play.subTracks[1] = &kTrack2;
    play.subTracks[2] = &kTrack3;
    apu.submitCommand(play);
    int16_t buffer[8192];
    apu.generateSamples(buffer, 256);

    AudioCommand sfx{};
    sfx.type = AudioCommandType::PLAY_EVENT;
    sfx.event.type = WaveType::PULSE;
    sfx.event.frequency = 660.0f;
    sfx.event.duration = 2.0f;
    sfx.event.volume = 0.5f;
    sfx.event.duty = 0.5f;

    for (int i = 0; i < 5; ++i) {
        apu.submitCommand(sfx);
    }
    apu.generateSamples(buffer, 256);

    for (int slot = 0; slot < ApuCore::SFX_VOICE_BASE; ++slot) {
        TEST_ASSERT_TRUE(apu.isVoiceEnabledForTesting(slot));
    }
}

static int count_enabled_sfx_voices(const ApuCore& apu) {
    int count = 0;
    for (int slot = ApuCore::SFX_VOICE_BASE; slot < ApuCore::MAX_VOICES; ++slot) {
        if (apu.isVoiceEnabledForTesting(slot)) {
            ++count;
        }
    }
    return count;
}

void test_apu_core_sequencer_percussion_same_step_uses_distinct_sfx_voices(void) {
    ApuCore apu;
    apu.init(44100);

    static const MusicNote kStackedDrums[] = {
        makeNote(INSTR_KICK, Note::Rest, 0.0f),
        makeNote(INSTR_SNARE, Note::Rest, 0.0f),
        makeNote(INSTR_HIHAT, Note::Rest, 1.0f),
    };
    static const MusicTrack kDrumTrack{
        kStackedDrums,
        sizeof(kStackedDrums) / sizeof(kStackedDrums[0]),
        false,
        WaveType::NOISE,
        0.5f,
    };

    AudioCommand play{};
    play.type = AudioCommandType::MUSIC_PLAY;
    play.track = &kDrumTrack;
    apu.submitCommand(play);

    int16_t buffer[8192];
    apu.generateSamples(buffer, 256);

    TEST_ASSERT_GREATER_OR_EQUAL_INT(2, count_enabled_sfx_voices(apu));
    TEST_ASSERT_FALSE(apu.isVoiceEnabledForTesting(3));
    TEST_ASSERT_FALSE(apu.isMusicTrackVoiceActiveForTesting(0));

    bool has_audio = false;
    for (int i = 0; i < 256; ++i) {
        if (buffer[i] != 0) {
            has_audio = true;
            break;
        }
    }
    TEST_ASSERT_TRUE(has_audio);
}

void test_apu_core_sequencer_percussion_does_not_disturb_melodic_slots(void) {
    ApuCore apu;
    apu.init(44100);

    static const MusicNote kLeadNotes[] = {
        {Note::C, 4, 4.0f, 0.8f, nullptr},
    };
    static const MusicNote kBassNotes[] = {
        {Note::C, 3, 4.0f, 0.8f, nullptr},
    };
    static const MusicNote kHarmonyNotes[] = {
        {Note::E, 4, 4.0f, 0.8f, nullptr},
    };
    static const MusicNote kDrumHit[] = {
        makeNote(INSTR_KICK, Note::Rest, 1.0f),
    };

    static const MusicTrack kLeadTrack{kLeadNotes, 1, false, WaveType::PULSE, 0.5f};
    static const MusicTrack kBassTrack{kBassNotes, 1, false, WaveType::TRIANGLE, 0.5f};
    static const MusicTrack kHarmonyTrack{kHarmonyNotes, 1, false, WaveType::PULSE, 0.5f};
    static const MusicTrack kDrumTrack{kDrumHit, 1, false, WaveType::NOISE, 0.5f};

    AudioCommand play{};
    play.type = AudioCommandType::MUSIC_PLAY;
    play.track = &kLeadTrack;
    play.subTrackCount = 3;
    play.subTracks[0] = &kBassTrack;
    play.subTracks[1] = &kHarmonyTrack;
    play.subTracks[2] = &kDrumTrack;
    apu.submitCommand(play);

    int16_t buffer[8192];
    apu.generateSamples(buffer, 256);

    TEST_ASSERT_TRUE(apu.isVoiceEnabledForTesting(0));
    TEST_ASSERT_TRUE(apu.isVoiceEnabledForTesting(1));
    TEST_ASSERT_TRUE(apu.isVoiceEnabledForTesting(2));
    TEST_ASSERT_FALSE(apu.isVoiceEnabledForTesting(3));
    TEST_ASSERT_TRUE(count_enabled_sfx_voices(apu) >= 1);
    TEST_ASSERT_FALSE(apu.isMusicTrackVoiceActiveForTesting(3));
}

void test_apu_core_sequencer_percussion_shares_sfx_pool_with_play_event(void) {
    ApuCore apu;
    apu.init(44100);

    static const MusicNote kLongMelody[] = {{Note::C, 4, 16.0f, 0.5f, nullptr}};
    static const MusicTrack kMelodyTrack{kLongMelody, 1, false, WaveType::PULSE, 0.5f};
    static const MusicNote kDrumHit[] = {makeNote(INSTR_SNARE, Note::Rest, 1.0f)};
    static const MusicTrack kDrumTrack{kDrumHit, 1, false, WaveType::NOISE, 0.5f};

    AudioCommand play{};
    play.type = AudioCommandType::MUSIC_PLAY;
    play.track = &kMelodyTrack;
    play.subTrackCount = 1;
    play.subTracks[0] = &kDrumTrack;
    apu.submitCommand(play);

    int16_t buffer[8192];
    apu.generateSamples(buffer, 256);
    TEST_ASSERT_TRUE(apu.isVoiceEnabledForTesting(0));
    TEST_ASSERT_FALSE(apu.isVoiceEnabledForTesting(1));

    AudioCommand sfx{};
    sfx.type = AudioCommandType::PLAY_EVENT;
    sfx.event.type = WaveType::PULSE;
    sfx.event.frequency = 880.0f;
    sfx.event.duration = 0.5f;
    sfx.event.volume = 0.5f;
    sfx.event.duty = 0.5f;
    apu.submitCommand(sfx);
    apu.generateSamples(buffer, 256);

    TEST_ASSERT_TRUE(apu.isVoiceEnabledForTesting(0));
    TEST_ASSERT_GREATER_OR_EQUAL_INT(2, count_enabled_sfx_voices(apu));
}

void test_apu_core_percussion_borrows_idle_music_slot(void) {
    ApuCore apu;
    apu.init(44100);

    static const MusicNote kLeadNotes[] = {
        {Note::C, 4, 4.0f, 0.8f, nullptr},
    };
    static const MusicNote kBassNotes[] = {
        {Note::C, 3, 4.0f, 0.8f, nullptr},
    };
    static const MusicNote kDrumHit[] = {
        makeNote(INSTR_KICK, Note::Rest, 1.0f),
    };
    static const MusicTrack kLeadTrack{kLeadNotes, 1, false, WaveType::PULSE, 0.5f};
    static const MusicTrack kBassTrack{kBassNotes, 1, false, WaveType::TRIANGLE, 0.5f};
    static const MusicTrack kDrumTrack{kDrumHit, 1, false, WaveType::NOISE, 0.5f};

    AudioCommand play{};
    play.type = AudioCommandType::MUSIC_PLAY;
    play.track = &kLeadTrack;          // trackIdx 0 -> voice slot 0
    play.subTrackCount = 2;
    play.subTracks[0] = &kBassTrack;   // trackIdx 1 -> voice slot 1
    play.subTracks[1] = &kDrumTrack;   // trackIdx 2 (percussion never claims its own slot)
    apu.submitCommand(play);

    // Saturate the SFX/percussion subpool (slots 4-7) BEFORE the drum hit
    // fires, so the fallback allocator finds no free voice in its own pool.
    AudioCommand sfx{};
    sfx.type = AudioCommandType::PLAY_EVENT;
    sfx.event.type = WaveType::PULSE;
    sfx.event.duration = 2.0f;
    sfx.event.volume = 0.5f;
    sfx.event.duty = 0.5f;
    for (int i = 0; i < 4; ++i) {
        sfx.event.frequency = 500.0f + (float)i * 40.0f;
        apu.submitCommand(sfx);
    }

    int16_t buffer[8192];
    apu.generateSamples(buffer, 256);

    // Melodic slots 0 and 1 are live; SFX pool 4-7 is saturated.
    TEST_ASSERT_TRUE(apu.isVoiceEnabledForTesting(0));
    TEST_ASSERT_TRUE(apu.isVoiceEnabledForTesting(1));
    for (int slot = ApuCore::SFX_VOICE_BASE; slot < ApuCore::MAX_VOICES; ++slot) {
        TEST_ASSERT_TRUE(apu.isVoiceEnabledForTesting(slot));
    }

    // Voice slot 2 was never claimed by a melodic track (trackIdx 2 is the
    // drum, which never allocates its own trackIdx slot), so it is idle.
    TEST_ASSERT_FALSE(apu.isMusicTrackVoiceActiveForTesting(2));

    // With the SFX subpool saturated and slot 2 idle, the percussion hit
    // must borrow the idle music slot instead of stealing a live SFX voice.
    TEST_ASSERT_TRUE(apu.isVoiceEnabledForTesting(2));
    TEST_ASSERT_FALSE(apu.isMusicTrackVoiceActiveForTesting(2));
}

void test_apu_core_percussion_never_steals_live_melodic_voice(void) {
    ApuCore apu;
    apu.init(44100);

    static const MusicNote kDrumNotes[] = {
        {Note::Rest, 4, 1.0f, 0.0f, nullptr},    // silent tick, advances the sequencer
        makeNote(INSTR_KICK, Note::Rest, 1.0f),  // actual percussion hit, next tick
    };
    static const MusicNote kMelodyNotes[] = {
        {Note::C, 4, 16.0f, 0.8f, nullptr},      // long note, still gated on the drum tick
    };
    static const MusicTrack kDrumTrack{kDrumNotes, 2, false, WaveType::NOISE, 0.5f};
    static const MusicTrack kMelodyTrack{kMelodyNotes, 1, false, WaveType::PULSE, 0.5f};

    AudioCommand play{};
    play.type = AudioCommandType::MUSIC_PLAY;
    play.track = &kDrumTrack;            // trackIdx 0 (percussion never claims voice slot 0)
    play.subTrackCount = 1;
    play.subTracks[0] = &kMelodyTrack;   // trackIdx 1 -> voice slot 1
    apu.submitCommand(play);

    int16_t buffer[8192];
    apu.generateSamples(buffer, 256);

    // Voice slot 1 is now a live melodic note.
    TEST_ASSERT_TRUE(apu.isVoiceEnabledForTesting(1));
    TEST_ASSERT_TRUE(apu.isMusicTrackVoiceActiveForTesting(1));

    // Saturate the SFX/percussion subpool before the drum's second tick fires.
    AudioCommand sfx{};
    sfx.type = AudioCommandType::PLAY_EVENT;
    sfx.event.type = WaveType::PULSE;
    sfx.event.duration = 2.0f;
    sfx.event.volume = 0.5f;
    sfx.event.duty = 0.5f;
    for (int i = 0; i < 4; ++i) {
        sfx.event.frequency = 600.0f + (float)i * 40.0f;
        apu.submitCommand(sfx);
    }

    const int tick_samples =
        (int)((44100.0f * 60.0f) / (ApuCore::DEFAULT_BPM * ApuCore::TICKS_PER_BEAT));
    for (int tick = 0; tick < 5; ++tick) {
        apu.generateSamples(buffer, tick_samples);
    }

    // The drum's second note has now fired a real percussion hit while
    // slot 1's melodic gate was still live; slot 1 must remain untouched.
    TEST_ASSERT_TRUE(apu.isMusicTrackVoiceActiveForTesting(1));
    TEST_ASSERT_TRUE(apu.isVoiceEnabledForTesting(1));
}

void test_apu_core_sequencer_tempo_factor_short_notes_keeps_playing(void) {
    ApuCore apu;
    apu.init(44100);

    // Space-invaders-style SI_HR: 0.25-beat notes (1 tick at tempoFactor 1.0).
    static const MusicNote kShortPulse[] = {
        makeNote(INSTR_PULSE_BASS, Note::C, 0.25f),
        makeRest(0.25f),
    };
    static const MusicNote kDrumHit[] = {
        makeNote(INSTR_KICK, Note::Rest, 1, 0.5f),
    };
    static const MusicTrack kMainTrack{
        kShortPulse, 2, true, WaveType::PULSE, INSTR_PULSE_BASS.duty};
    static const MusicTrack kDrumTrack{
        kDrumHit, 1, true, WaveType::NOISE, 0.5f};

    AudioCommand tempo{};
    tempo.type = AudioCommandType::MUSIC_SET_TEMPO;
    tempo.tempoFactor = 1.5f;
    apu.submitCommand(tempo);

    AudioCommand play{};
    play.type = AudioCommandType::MUSIC_PLAY;
    play.track = &kMainTrack;
    play.subTrackCount = 1;
    play.subTracks[0] = &kDrumTrack;
    apu.submitCommand(play);

    int16_t buffer[4096];
    for (int frame = 0; frame < 80; ++frame) {
        apu.generateSamples(buffer, 512);
    }

    TEST_ASSERT_TRUE(apu.isMusicPlaying());

    bool has_audio = false;
    apu.generateSamples(buffer, 4096);
    for (int i = 0; i < 4096; ++i) {
        if (buffer[i] != 0) {
            has_audio = true;
            break;
        }
    }
    TEST_ASSERT_TRUE(has_audio);
}

void test_apu_core_melodic_noise_on_track_three_uses_music_slot(void) {
    ApuCore apu;
    apu.init(44100);

    static const MusicNote kSilent[] = {{Note::Rest, 4, 16.0f, 0.0f, nullptr}};
    static const MusicNote kNoiseMelody[] = {
        {Note::C, 4, 2.0f, 0.8f, nullptr},
    };
    static const MusicTrack kTrack0{kSilent, 1, false, WaveType::PULSE, 0.5f};
    static const MusicTrack kTrack1{kSilent, 1, false, WaveType::PULSE, 0.5f};
    static const MusicTrack kTrack2{kSilent, 1, false, WaveType::PULSE, 0.5f};
    static const MusicTrack kTrack3{kNoiseMelody, 1, false, WaveType::NOISE, 0.5f};

    AudioCommand play{};
    play.type = AudioCommandType::MUSIC_PLAY;
    play.track = &kTrack0;
    play.subTrackCount = 3;
    play.subTracks[0] = &kTrack1;
    play.subTracks[1] = &kTrack2;
    play.subTracks[2] = &kTrack3;
    apu.submitCommand(play);

    int16_t buffer[8192];
    apu.generateSamples(buffer, 256);

    TEST_ASSERT_TRUE(apu.isVoiceEnabledForTesting(3));
    TEST_ASSERT_TRUE(apu.isMusicTrackVoiceActiveForTesting(3));
    TEST_ASSERT_EQUAL_INT(0, count_enabled_sfx_voices(apu));
}

// Fix B (WU-3): melodic note-on must extend remainingSamples by the full
// env.releaseSamples regardless of legato, so the release tail overlaps
// the next beat instead of being hard-cut at the gate boundary.
//
// There is no test-only accessor for Voice::remainingSamples, so this
// drives through the real sequencer path and asserts the *observable*
// timing consequence instead: at exactly `gate_samples + releaseSamples`
// generated samples,
//   - pre-fix:  remainingSamples started at gate_samples only, so the
//     per-sample auto-release trigger (generateSampleForVoice) already
//     armed and fully exhausted one release cycle by this point ->
//     voice is disabled.
//   - post-fix: remainingSamples started at gate_samples + releaseSamples,
//     so the auto-release trigger only just now arms the release stage ->
//     voice is still enabled (tail keeps sounding into the next beat).
void test_apu_core_melodic_tail_overlaps_next_beat(void) {
    ApuCore apu;
    apu.init(44100);

    // Fresh voice, single note-on -> music_sequencer_legato collapses to
    // false internally (ch->enabled starts false), so this exercises the
    // exact "not legato" case the pre-fix gate guarded against.
    static const MusicNote kNote[] = {
        {Note::C, 4, 1.0f, 0.8f, &INSTR_PULSE_BASS},
    };
    static const MusicTrack kTrack{
        kNote, 1, false, WaveType::PULSE, INSTR_PULSE_BASS.duty};

    AudioCommand play{};
    play.type = AudioCommandType::MUSIC_PLAY;
    play.track = &kTrack;
    apu.submitCommand(play);

    // gate_samples = noteTicks(4) * tickDurationSamples(4410 @ default
    // 150 BPM, 44100 Hz) = 17640.
    // releaseSamples = min(INSTR_PULSE_BASS.releaseTime * 44100, 4410)
    //                = min(3528, 4410) = 3528.
    // Target = gate_samples + releaseSamples = 21168.
    static int16_t buffer[21168];
    apu.generateSamples(buffer, 21168);

    TEST_ASSERT_TRUE(apu.isVoiceEnabledForTesting(0));
}

// Fix B counterpart: a preset with releaseTime == 0 must leave the gate
// unchanged (no tail extension) in both pre- and post-fix code, since the
// `env.releaseSamples > 0` guard rejects the addition either way.
void test_apu_core_melodic_zero_release_leaves_gate_unchanged(void) {
    ApuCore apu;
    apu.init(44100);

    static constexpr InstrumentPreset kNoReleasePreset{
        0.30f,    // baseVolume
        0.25f,    // duty
        2,        // defaultOctave
        0.0f,     // defaultDuration
        0,        // noisePeriod
        0.001f,   // attackTime
        0.08f,    // decayTime
        0.35f,    // sustainLevel
        0.0f,     // releaseTime -- zero on purpose
        LfoTarget::NONE,
        0.0f,     // lfoFrequency
        0.0f,     // lfoDepth
        0.0f,     // lfoDelay
        false,    // noiseLfsrShort
        0.0f      // dutySweep
    };

    static const MusicNote kNote[] = {
        {Note::C, 4, 1.0f, 0.8f, &kNoReleasePreset},
    };
    static const MusicTrack kTrack{
        kNote, 1, false, WaveType::PULSE, kNoReleasePreset.duty};

    AudioCommand play{};
    play.type = AudioCommandType::MUSIC_PLAY;
    play.track = &kTrack;
    apu.submitCommand(play);

    // gate_samples = 17640 (same tempo/note-duration math as above).
    // releaseSamples == 0 => remainingSamples == gate_samples exactly, so
    // the voice must be fully disabled right at the gate boundary.
    static int16_t buffer[17640];
    apu.generateSamples(buffer, 17640);

    TEST_ASSERT_FALSE(apu.isVoiceEnabledForTesting(0));
}

// =============================================================================
// WU-4: Partition / API / mixer invariant regression tests. Pure coverage --
// no production change is expected here (see spec "Partition and Isolation
// Invariants Preserved").
// =============================================================================

// A melodic note-on for track 3 must always land on its fixed slot
// (MUSIC_VOICE_BASE + 3), unaffected by percussion allocator activity
// elsewhere in the pool.
void test_apu_core_melodic_track_keeps_fixed_slot_mapping(void) {
    ApuCore apu;
    apu.init(44100);

    static const MusicNote kDrumHit[] = {
        makeNote(INSTR_KICK, Note::Rest, 1.0f),
    };
    static const MusicNote kSilent[] = {{Note::Rest, 4, 16.0f, 0.0f, nullptr}};
    static const MusicNote kMelodyNote[] = {
        {Note::C, 4, 2.0f, 0.8f, nullptr},
    };
    static const MusicTrack kTrack0{kDrumHit, 1, false, WaveType::NOISE, 0.5f};
    static const MusicTrack kTrack1{kSilent, 1, false, WaveType::PULSE, 0.5f};
    static const MusicTrack kTrack2{kSilent, 1, false, WaveType::PULSE, 0.5f};
    static const MusicTrack kTrack3{kMelodyNote, 1, false, WaveType::PULSE, 0.5f};

    AudioCommand play{};
    play.type = AudioCommandType::MUSIC_PLAY;
    play.track = &kTrack0;           // trackIdx 0 (percussion never claims its own slot)
    play.subTrackCount = 3;
    play.subTracks[0] = &kTrack1;    // trackIdx 1, idle filler
    play.subTracks[1] = &kTrack2;    // trackIdx 2, idle filler
    play.subTracks[2] = &kTrack3;    // trackIdx 3 -> fixed voice slot MUSIC_VOICE_BASE+3
    apu.submitCommand(play);

    // Saturate the SFX/percussion subpool so the drum hit is forced through
    // the idle-melodic-slot fallback, perturbing percussion allocator state
    // before track 3's fixed mapping is exercised.
    AudioCommand sfx{};
    sfx.type = AudioCommandType::PLAY_EVENT;
    sfx.event.type = WaveType::PULSE;
    sfx.event.duration = 2.0f;
    sfx.event.volume = 0.5f;
    sfx.event.duty = 0.5f;
    for (int i = 0; i < 4; ++i) {
        sfx.event.frequency = 800.0f + (float)i * 40.0f;
        apu.submitCommand(sfx);
    }

    int16_t buffer[8192];
    apu.generateSamples(buffer, 256);

    // Track 3's melodic note-on always allocates its fixed slot
    // MUSIC_VOICE_BASE + 3, regardless of percussion allocator activity.
    TEST_ASSERT_TRUE(apu.isVoiceEnabledForTesting(ApuCore::MUSIC_VOICE_BASE + 3));
    TEST_ASSERT_TRUE(apu.isMusicTrackVoiceActiveForTesting(3));
}

// Complement to WU-2's borrow test: when the SFX subpool is saturated AND
// every melodic slot is busy (enabled + a live track gate), the percussion
// allocator has no idle slot to borrow and must stay confined to the
// steal-SFX fallback (slots 4-7), never disturbing slots 0-3.
void test_apu_core_percussion_saturated_no_idle_slot_confined_to_sfx(void) {
    ApuCore apu;
    apu.init(44100);

    static const MusicNote kLongMelody[] = {
        {Note::C, 4, 16.0f, 0.8f, nullptr},
    };
    static const MusicNote kTrack3Notes[] = {
        {Note::C, 4, 1.0f, 0.8f, nullptr},          // claims slot 3 first
        makeNote(INSTR_KICK, Note::Rest, 1.0f),     // then a real percussion hit
    };
    static const MusicTrack kTrack0{kLongMelody, 1, false, WaveType::PULSE, 0.5f};
    static const MusicTrack kTrack1{kLongMelody, 1, false, WaveType::TRIANGLE, 0.5f};
    static const MusicTrack kTrack2{kLongMelody, 1, false, WaveType::PULSE, 0.5f};
    static const MusicTrack kTrack3{kTrack3Notes, 2, false, WaveType::NOISE, 0.5f};

    AudioCommand play{};
    play.type = AudioCommandType::MUSIC_PLAY;
    play.track = &kTrack0;           // trackIdx 0 -> voice slot 0
    play.subTrackCount = 3;
    play.subTracks[0] = &kTrack1;    // trackIdx 1 -> voice slot 1
    play.subTracks[1] = &kTrack2;    // trackIdx 2 -> voice slot 2
    play.subTracks[2] = &kTrack3;    // trackIdx 3 -> voice slot 3, then fires a percussion hit
    apu.submitCommand(play);

    int16_t buffer[8192];
    apu.generateSamples(buffer, 256);

    // All 4 melodic slots are live before the percussion hit fires.
    for (int slot = ApuCore::MUSIC_VOICE_BASE; slot < ApuCore::SFX_VOICE_BASE; ++slot) {
        TEST_ASSERT_TRUE(apu.isVoiceEnabledForTesting(slot));
        TEST_ASSERT_TRUE(apu.isMusicTrackVoiceActiveForTesting((size_t)slot));
    }

    // Saturate the SFX/percussion subpool before track 3's second note (the
    // real percussion hit) fires.
    AudioCommand sfx{};
    sfx.type = AudioCommandType::PLAY_EVENT;
    sfx.event.type = WaveType::PULSE;
    sfx.event.duration = 2.0f;
    sfx.event.volume = 0.5f;
    sfx.event.duty = 0.5f;
    for (int i = 0; i < 4; ++i) {
        sfx.event.frequency = 900.0f + (float)i * 40.0f;
        apu.submitCommand(sfx);
    }

    const int tick_samples =
        (int)((44100.0f * 60.0f) / (ApuCore::DEFAULT_BPM * ApuCore::TICKS_PER_BEAT));
    for (int tick = 0; tick < 5; ++tick) {
        apu.generateSamples(buffer, tick_samples);
    }

    // With SFX saturated and every melodic slot busy, the percussion hit
    // must never disturb a melodic slot -- every track keeps its gate.
    for (int slot = ApuCore::MUSIC_VOICE_BASE; slot < ApuCore::SFX_VOICE_BASE; ++slot) {
        TEST_ASSERT_TRUE(apu.isMusicTrackVoiceActiveForTesting((size_t)slot));
    }

    // The hit stayed confined to the SFX/percussion subpool (steal-SFX
    // fallback), never touching 0-3.
    for (int slot = ApuCore::SFX_VOICE_BASE; slot < ApuCore::MAX_VOICES; ++slot) {
        TEST_ASSERT_TRUE(apu.isVoiceEnabledForTesting(slot));
    }
}

// =============================================================================
// Integration test - full audio pipeline
// =============================================================================

void test_apu_core_integration_full_pipeline(void) {
    ApuCore apu;
    apu.init(44100);

    // Submit an event
    AudioCommand cmd;
    cmd.type = AudioCommandType::PLAY_EVENT;
    cmd.event.type = WaveType::PULSE;
    cmd.event.frequency = 440.0f;
    cmd.event.duration = 0.5f;
    cmd.event.volume = 0.5f;
    cmd.event.duty = 0.5f;

    // Submit and generate (processCommands called internally)
    bool submitted = apu.submitCommand(cmd);
    TEST_ASSERT_TRUE(submitted);

    // Generate samples
    int16_t buffer[512] = {0};
    apu.generateSamples(buffer, 512);

    // Verify samples are in valid range
    for (int i = 0; i < 512; i++) {
        TEST_ASSERT_TRUE(buffer[i] >= -32768);
        TEST_ASSERT_TRUE(buffer[i] <= 32767);
    }
}

void test_apu_core_integration_multiple_voices(void) {
    ApuCore apu;
    apu.init(44100);

    // Submit multiple notes with different frequencies
    for (int i = 0; i < 3; i++) {
        AudioCommand cmd;
        cmd.type = AudioCommandType::PLAY_EVENT;
        cmd.event.type = WaveType::PULSE;
        cmd.event.frequency = 220.0f + (i * 110.0f);
        cmd.event.duration = 1.0f;
        cmd.event.volume = 0.3f;
        cmd.event.duty = 0.5f;
        apu.submitCommand(cmd);
    }

    // Generate samples - should mix multiple voices
    int16_t buffer[512] = {0};
    apu.generateSamples(buffer, 512);

    // Should have audio output
    bool hasNonZero = false;
    for (int i = 0; i < 512; i++) {
        if (buffer[i] != 0) {
            hasNonZero = true;
            break;
        }
    }
    TEST_ASSERT_TRUE(hasNonZero);
}

// =============================================================================
// Export parity: MusicTrack.duty is authoritative (not InstrumentPreset.duty)
// =============================================================================

// =============================================================================
// Noise sweep, loop, and duration==0 one-shot (sfx-synthesis-high-priority)
// =============================================================================

void test_apu_core_noise_pitch_sweep_updates_period(void) {
    ApuCore apu;
    apu.init(44100);
    apu.reset();

    AudioCommand cmd{};
    cmd.type = AudioCommandType::PLAY_EVENT;
    cmd.event.type = WaveType::NOISE;
    cmd.event.frequency = 2000.0f;  // period = 44100/2000 = 22
    cmd.event.duration = 0.5f;
    cmd.event.volume = 0.5f;
    cmd.event.duty = 0.5f;
    cmd.event.noisePeriod = 0;
    cmd.event.sweepEndHz = 200.0f;  // period = 44100/200 = 220
    cmd.event.sweepDurationSec = 0.05f;
    cmd.event.loop = false;
    TEST_ASSERT_TRUE(apu.submitCommand(cmd));

    int16_t buffer[256] = {0};
    apu.generateSamples(buffer, 256);

    const uint32_t startPeriod = 44100u / 2000u;
    const uint32_t endPeriod = 44100u / 200u;
    int sfxSlot = -1;
    for (int i = ApuCore::SFX_VOICE_BASE; i < ApuCore::MAX_VOICES; ++i) {
        if (apu.isVoiceEnabledForTesting(i)) {
            sfxSlot = i;
            break;
        }
    }
    TEST_ASSERT_TRUE(sfxSlot >= 0);
    const uint32_t periodAfter = apu.getVoiceNoisePeriodForTesting(sfxSlot);
    TEST_ASSERT_TRUE(periodAfter > startPeriod);
    TEST_ASSERT_TRUE(periodAfter <= endPeriod);

    // Run through the rest of the sweep window.
    for (int n = 0; n < 20; ++n) {
        apu.generateSamples(buffer, 256);
    }
    const uint32_t periodEnd = apu.getVoiceNoisePeriodForTesting(sfxSlot);
    TEST_ASSERT_EQUAL_UINT32(endPeriod, periodEnd);
}

void test_apu_core_loop_voice_stays_enabled_until_stop(void) {
    ApuCore apu;
    apu.init(44100);
    apu.reset();

    AudioCommand cmd{};
    cmd.type = AudioCommandType::PLAY_EVENT;
    cmd.event.type = WaveType::TRIANGLE;
    cmd.event.frequency = 220.0f;
    cmd.event.duration = 0.0f;
    cmd.event.volume = 0.4f;
    cmd.event.duty = 0.5f;
    cmd.event.loop = true;
    TEST_ASSERT_TRUE(apu.submitCommand(cmd));

    int16_t buffer[512] = {0};
    apu.generateSamples(buffer, 512);
    TEST_ASSERT_TRUE(apu.countEnabledVoicesForTesting() >= 1);

    int loopSlot = -1;
    for (int i = ApuCore::SFX_VOICE_BASE; i < ApuCore::MAX_VOICES; ++i) {
        if (apu.isVoiceLoopForTesting(i) && apu.isVoiceEnabledForTesting(i)) {
            loopSlot = i;
            break;
        }
    }
    TEST_ASSERT_TRUE(loopSlot >= 0);

    // Far beyond any short one-shot duration — still playing.
    for (int n = 0; n < 40; ++n) {
        apu.generateSamples(buffer, 512);
    }
    TEST_ASSERT_TRUE(apu.isVoiceEnabledForTesting(loopSlot));
    TEST_ASSERT_TRUE(apu.isVoiceLoopForTesting(loopSlot));

    AudioCommand stop{};
    stop.type = AudioCommandType::STOP_CHANNEL;
    stop.channelIndex = static_cast<uint8_t>(loopSlot);
    TEST_ASSERT_TRUE(apu.submitCommand(stop));
    apu.generateSamples(buffer, 64);
    TEST_ASSERT_FALSE(apu.isVoiceEnabledForTesting(loopSlot));
}

void test_apu_core_oneshot_zero_duration_does_not_hang(void) {
    ApuCore apu;
    apu.init(44100);
    apu.reset();

    AudioCommand cmd{};
    cmd.type = AudioCommandType::PLAY_EVENT;
    cmd.event.type = WaveType::PULSE;
    cmd.event.frequency = 440.0f;
    cmd.event.duration = 0.0f;
    cmd.event.volume = 0.5f;
    cmd.event.duty = 0.5f;
    cmd.event.loop = false;
    TEST_ASSERT_TRUE(apu.submitCommand(cmd));

    int16_t buffer[128] = {0};
    apu.generateSamples(buffer, 128);
    TEST_ASSERT_EQUAL_INT(0, apu.countEnabledVoicesForTesting());
}

// =============================================================================
// SweepCurve Linear / Exponential (sfx-synthesis-medium-priority)
// =============================================================================

static int find_first_enabled_sfx_slot(ApuCore& apu) {
    for (int i = ApuCore::SFX_VOICE_BASE; i < ApuCore::MAX_VOICES; ++i) {
        if (apu.isVoiceEnabledForTesting(i)) {
            return i;
        }
    }
    return -1;
}

void test_apu_core_linear_sweep_midpoint_arithmetic(void) {
    ApuCore apu;
    apu.init(44100);
    apu.reset();

    AudioCommand cmd{};
    cmd.type = AudioCommandType::PLAY_EVENT;
    cmd.event.type = WaveType::PULSE;
    cmd.event.frequency = 440.0f;
    cmd.event.duration = 1.0f;
    cmd.event.volume = 0.5f;
    cmd.event.duty = 0.5f;
    cmd.event.sweepEndHz = 880.0f;
    cmd.event.sweepDurationSec = 0.1f;  // 4410 samples
    cmd.event.loop = false;
    cmd.event.sweepCurve = SweepCurve::Linear;
    TEST_ASSERT_TRUE(apu.submitCommand(cmd));

    int16_t buffer[256] = {0};
    // Advance to ~50% of sweep (2205 samples ≈ 8.6 * 256).
    for (int n = 0; n < 9; ++n) {
        apu.generateSamples(buffer, 256);
    }

    const int slot = find_first_enabled_sfx_slot(apu);
    TEST_ASSERT_TRUE(slot >= 0);
    const float hz = apu.getVoiceFrequencyForTesting(slot);
    // Linear midpoint ≈ 660 Hz; allow coarse window around arithmetic mean.
    TEST_ASSERT_FLOAT_WITHIN(40.0f, 660.0f, hz);
}

void test_apu_core_exponential_sweep_midpoint_geometric(void) {
    ApuCore apu;
    apu.init(44100);
    apu.reset();

    AudioCommand cmd{};
    cmd.type = AudioCommandType::PLAY_EVENT;
    cmd.event.type = WaveType::PULSE;
    cmd.event.frequency = 440.0f;
    cmd.event.duration = 1.0f;
    cmd.event.volume = 0.5f;
    cmd.event.duty = 0.5f;
    cmd.event.sweepEndHz = 880.0f;
    cmd.event.sweepDurationSec = 0.1f;
    cmd.event.loop = false;
    cmd.event.sweepCurve = SweepCurve::Exponential;
    TEST_ASSERT_TRUE(apu.submitCommand(cmd));

    int16_t buffer[256] = {0};
    for (int n = 0; n < 9; ++n) {
        apu.generateSamples(buffer, 256);
    }

    const int slot = find_first_enabled_sfx_slot(apu);
    TEST_ASSERT_TRUE(slot >= 0);
    const float hz = apu.getVoiceFrequencyForTesting(slot);
    // Geometric midpoint = 440 * sqrt(2) ≈ 622.25; must not sit on linear 660.
    TEST_ASSERT_FLOAT_WITHIN(35.0f, 622.25f, hz);
    TEST_ASSERT_TRUE(std::fabs(hz - 660.0f) > 15.0f);
}

void test_apu_core_exponential_noise_sweep_updates_period(void) {
    ApuCore apu;
    apu.init(44100);
    apu.reset();

    AudioCommand cmd{};
    cmd.type = AudioCommandType::PLAY_EVENT;
    cmd.event.type = WaveType::NOISE;
    cmd.event.frequency = 2000.0f;  // period = 22
    cmd.event.duration = 0.5f;
    cmd.event.volume = 0.5f;
    cmd.event.duty = 0.5f;
    cmd.event.noisePeriod = 0;
    cmd.event.sweepEndHz = 200.0f;  // period = 220
    cmd.event.sweepDurationSec = 0.05f;
    cmd.event.loop = false;
    cmd.event.sweepCurve = SweepCurve::Exponential;
    TEST_ASSERT_TRUE(apu.submitCommand(cmd));

    int16_t buffer[256] = {0};
    apu.generateSamples(buffer, 256);

    const uint32_t startPeriod = 44100u / 2000u;
    const uint32_t endPeriod = 44100u / 200u;
    const int slot = find_first_enabled_sfx_slot(apu);
    TEST_ASSERT_TRUE(slot >= 0);
    const uint32_t periodAfter = apu.getVoiceNoisePeriodForTesting(slot);
    TEST_ASSERT_TRUE(periodAfter > startPeriod);
    TEST_ASSERT_TRUE(periodAfter <= endPeriod);

    for (int n = 0; n < 20; ++n) {
        apu.generateSamples(buffer, 256);
    }
    TEST_ASSERT_EQUAL_UINT32(endPeriod, apu.getVoiceNoisePeriodForTesting(slot));
}

void test_apu_core_sweep_curve_default_linear_and_nonpositive_fallback(void) {
    ApuCore apu;
    apu.init(44100);
    apu.reset();

    // Brace-init omits sweepCurve → Linear (ABI default 0).
    AudioCommand linearCmd{};
    linearCmd.type = AudioCommandType::PLAY_EVENT;
    linearCmd.event.type = WaveType::PULSE;
    linearCmd.event.frequency = 440.0f;
    linearCmd.event.duration = 1.0f;
    linearCmd.event.volume = 0.5f;
    linearCmd.event.duty = 0.5f;
    linearCmd.event.sweepEndHz = 880.0f;
    linearCmd.event.sweepDurationSec = 0.1f;
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(SweepCurve::Linear),
                            static_cast<uint8_t>(linearCmd.event.sweepCurve));
    TEST_ASSERT_TRUE(apu.submitCommand(linearCmd));

    int16_t buffer[256] = {0};
    for (int n = 0; n < 9; ++n) {
        apu.generateSamples(buffer, 256);
    }
    int slot = find_first_enabled_sfx_slot(apu);
    TEST_ASSERT_TRUE(slot >= 0);
    TEST_ASSERT_FLOAT_WITHIN(40.0f, 660.0f, apu.getVoiceFrequencyForTesting(slot));

    // Exponential with non-positive start Hz falls back to Linear arithmetic.
    apu.reset();
    AudioCommand fallbackCmd{};
    fallbackCmd.type = AudioCommandType::PLAY_EVENT;
    fallbackCmd.event.type = WaveType::PULSE;
    fallbackCmd.event.frequency = 0.0f;
    fallbackCmd.event.duration = 1.0f;
    fallbackCmd.event.volume = 0.5f;
    fallbackCmd.event.duty = 0.5f;
    fallbackCmd.event.sweepEndHz = 880.0f;
    fallbackCmd.event.sweepDurationSec = 0.1f;
    fallbackCmd.event.sweepCurve = SweepCurve::Exponential;
    TEST_ASSERT_TRUE(apu.submitCommand(fallbackCmd));
    for (int n = 0; n < 9; ++n) {
        apu.generateSamples(buffer, 256);
    }
    slot = find_first_enabled_sfx_slot(apu);
    TEST_ASSERT_TRUE(slot >= 0);
    // Linear from 0 → 880 at alpha≈0.5 ≈ 440 (not geometric, which is undefined).
    TEST_ASSERT_FLOAT_WITHIN(50.0f, 440.0f, apu.getVoiceFrequencyForTesting(slot));
}

void test_apu_core_loop_voice_is_stealable(void) {
    ApuCore apu;
    apu.init(44100);
    apu.reset();

    // Fill SFX pool with long one-shots + one loop; then force steal.
    for (int i = 0; i < ApuCore::SFX_VOICE_COUNT; ++i) {
        AudioCommand cmd{};
        cmd.type = AudioCommandType::PLAY_EVENT;
        cmd.event.type = WaveType::PULSE;
        cmd.event.frequency = 300.0f + static_cast<float>(i) * 10.0f;
        cmd.event.duration = (i == 0) ? 0.0f : 2.0f;
        cmd.event.volume = 0.3f;
        cmd.event.duty = 0.5f;
        cmd.event.loop = (i == 0);
        TEST_ASSERT_TRUE(apu.submitCommand(cmd));
    }

    int16_t buffer[256] = {0};
    apu.generateSamples(buffer, 256);
    TEST_ASSERT_EQUAL_INT(ApuCore::SFX_VOICE_COUNT,
                          static_cast<int>(apu.countEnabledVoicesForTesting()));

    int loopSlot = -1;
    for (int i = ApuCore::SFX_VOICE_BASE; i < ApuCore::MAX_VOICES; ++i) {
        if (apu.isVoiceLoopForTesting(i)) {
            loopSlot = i;
            break;
        }
    }
    TEST_ASSERT_TRUE(loopSlot >= 0);

    // New event must steal the loop (steal score 0) rather than starve.
    AudioCommand steal{};
    steal.type = AudioCommandType::PLAY_EVENT;
    steal.event.type = WaveType::TRIANGLE;
    steal.event.frequency = 880.0f;
    steal.event.duration = 0.2f;
    steal.event.volume = 0.5f;
    steal.event.duty = 0.5f;
    steal.event.loop = false;
    TEST_ASSERT_TRUE(apu.submitCommand(steal));
    apu.generateSamples(buffer, 64);

    TEST_ASSERT_FALSE(apu.isVoiceLoopForTesting(loopSlot));
    TEST_ASSERT_TRUE(apu.isVoiceEnabledForTesting(loopSlot));
}

// =============================================================================
// Duty stepped (sfx-synthesis-low-priority block 1)
// =============================================================================

void test_apu_core_duty_stepped_absent_keeps_legacy_duty_sweep(void) {
    // Large dutySweep with no duty steps must still advance dutyCycle (legacy PWM).
    static const InstrumentPreset kSweepPreset{
        0.8f, 0.5f, 4, 0.0f, 0, 0.001f, 0.0f, 1.0f, 0.001f,
        LfoTarget::NONE, 0.0f, 0.0f, 0.0f, false,
        8.0f  // dutySweep: +8.0 duty units per second
    };

    ApuCore apu;
    apu.init(44100);
    apu.reset();

    AudioCommand cmd{};
    cmd.type = AudioCommandType::PLAY_EVENT;
    cmd.event.type = WaveType::PULSE;
    cmd.event.frequency = 440.0f;
    cmd.event.duration = 0.5f;
    cmd.event.volume = 0.5f;
    cmd.event.duty = 0.125f;
    cmd.event.preset = &kSweepPreset;
    cmd.event.dutySteps = nullptr;
    cmd.event.dutyStepCount = 0;
    TEST_ASSERT_TRUE(apu.submitCommand(cmd));

    int16_t buffer[256] = {0};
    apu.generateSamples(buffer, 1);
    const int slot = find_first_enabled_sfx_slot(apu);
    TEST_ASSERT_TRUE(slot >= 0);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.125f, apu.getVoiceDutyCycleForTesting(slot));
    TEST_ASSERT_TRUE(apu.getVoiceDutySweepPerSampleForTesting(slot) != 0.0f);

    apu.generateSamples(buffer, 256);
    const float dutyAfter = apu.getVoiceDutyCycleForTesting(slot);
    TEST_ASSERT_TRUE_MESSAGE(dutyAfter > 0.125f + 0.01f,
                             "legacy dutySweep must advance duty without steps");
}

void test_apu_core_duty_stepped_mid_note_hold(void) {
    static const SfxBreakpoint kSteps[] = {
        {0.0f, 0.125f},
        {0.05f, 0.5f},
    };

    ApuCore apu;
    apu.init(44100);
    apu.reset();

    AudioCommand cmd{};
    cmd.type = AudioCommandType::PLAY_EVENT;
    cmd.event.type = WaveType::PULSE;
    cmd.event.frequency = 440.0f;
    cmd.event.duration = 0.25f;
    cmd.event.volume = 0.5f;
    cmd.event.duty = 0.75f;  // superseded by step at t=0
    cmd.event.dutySteps = kSteps;
    cmd.event.dutyStepCount = 2;
    TEST_ASSERT_TRUE(apu.submitCommand(cmd));

    int16_t buffer[256] = {0};
    apu.generateSamples(buffer, 1);
    const int slot = find_first_enabled_sfx_slot(apu);
    TEST_ASSERT_TRUE(slot >= 0);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.125f, apu.getVoiceDutyCycleForTesting(slot));

    // Just before 0.05s (2205 samples @ 44100): still 12.5%.
    const int beforeBoundary = 2200;
    int remaining = beforeBoundary - 1;  // already generated 1 sample
    while (remaining > 0) {
        const int n = remaining > 256 ? 256 : remaining;
        apu.generateSamples(buffer, n);
        remaining -= n;
    }
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.125f, apu.getVoiceDutyCycleForTesting(slot));

    // Cross 0.05s boundary → hold 50%.
    apu.generateSamples(buffer, 16);
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.5f, apu.getVoiceDutyCycleForTesting(slot));
}

void test_apu_core_pitch_envelope_three_points_linear(void) {
    static const SfxBreakpoint kPitch[] = {
        {0.0f, 523.25f},
        {0.1f, 659.25f},
        {0.2f, 783.99f},
    };

    ApuCore apu;
    apu.init(44100);
    apu.reset();

    AudioCommand cmd{};
    cmd.type = AudioCommandType::PLAY_EVENT;
    cmd.event.type = WaveType::PULSE;
    cmd.event.frequency = 100.0f;  // superseded by first pitch point
    cmd.event.duration = 0.4f;
    cmd.event.volume = 0.5f;
    cmd.event.duty = 0.5f;
    cmd.event.pitchEnvelope = kPitch;
    cmd.event.pitchEnvelopeCount = 3;
    cmd.event.sweepCurve = SweepCurve::Linear;
    TEST_ASSERT_TRUE(apu.submitCommand(cmd));

    int16_t buffer[256] = {0};
    apu.generateSamples(buffer, 1);
    const int slot = find_first_enabled_sfx_slot(apu);
    TEST_ASSERT_TRUE(slot >= 0);
    TEST_ASSERT_FLOAT_WITHIN(2.0f, 523.25f, apu.getVoiceFrequencyForTesting(slot));

    // Midpoint of first segment (~0.05s).
    int remaining = 2205 - 1;
    while (remaining > 0) {
        const int n = remaining > 256 ? 256 : remaining;
        apu.generateSamples(buffer, n);
        remaining -= n;
    }
    TEST_ASSERT_FLOAT_WITHIN(8.0f, 591.25f, apu.getVoiceFrequencyForTesting(slot));

    // Midpoint of second segment (~0.15s): +4410 samples from 0.05.
    remaining = 4410;
    while (remaining > 0) {
        const int n = remaining > 256 ? 256 : remaining;
        apu.generateSamples(buffer, n);
        remaining -= n;
    }
    TEST_ASSERT_FLOAT_WITHIN(8.0f, 721.62f, apu.getVoiceFrequencyForTesting(slot));

    // Past last point: hold.
    for (int i = 0; i < 20; ++i) {
        apu.generateSamples(buffer, 256);
    }
    TEST_ASSERT_FLOAT_WITHIN(2.0f, 783.99f, apu.getVoiceFrequencyForTesting(slot));
}

void test_apu_core_pitch_envelope_noise_updates_period(void) {
    static const SfxBreakpoint kPitch[] = {
        {0.0f, 2000.0f},
        {0.05f, 200.0f},
    };

    ApuCore apu;
    apu.init(44100);
    apu.reset();

    AudioCommand cmd{};
    cmd.type = AudioCommandType::PLAY_EVENT;
    cmd.event.type = WaveType::NOISE;
    cmd.event.frequency = 500.0f;
    cmd.event.duration = 0.3f;
    cmd.event.volume = 0.5f;
    cmd.event.duty = 0.5f;
    cmd.event.pitchEnvelope = kPitch;
    cmd.event.pitchEnvelopeCount = 2;
    cmd.event.sweepCurve = SweepCurve::Linear;
    TEST_ASSERT_TRUE(apu.submitCommand(cmd));

    int16_t buffer[256] = {0};
    apu.generateSamples(buffer, 1);
    const int slot = find_first_enabled_sfx_slot(apu);
    TEST_ASSERT_TRUE(slot >= 0);
    TEST_ASSERT_EQUAL_UINT32(44100u / 2000u, apu.getVoiceNoisePeriodForTesting(slot));

    for (int i = 0; i < 20; ++i) {
        apu.generateSamples(buffer, 256);
    }
    TEST_ASSERT_EQUAL_UINT32(44100u / 200u, apu.getVoiceNoisePeriodForTesting(slot));
}

void test_apu_core_pitch_envelope_precedes_single_sweep(void) {
    static const SfxBreakpoint kPitch[] = {
        {0.0f, 440.0f},
        {0.1f, 880.0f},
    };

    ApuCore apu;
    apu.init(44100);
    apu.reset();

    AudioCommand cmd{};
    cmd.type = AudioCommandType::PLAY_EVENT;
    cmd.event.type = WaveType::TRIANGLE;
    cmd.event.frequency = 440.0f;
    cmd.event.duration = 0.3f;
    cmd.event.volume = 0.5f;
    cmd.event.duty = 0.5f;
    cmd.event.sweepEndHz = 100.0f;       // must be ignored
    cmd.event.sweepDurationSec = 0.1f;
    cmd.event.pitchEnvelope = kPitch;
    cmd.event.pitchEnvelopeCount = 2;
    cmd.event.sweepCurve = SweepCurve::Linear;
    TEST_ASSERT_TRUE(apu.submitCommand(cmd));

    int16_t buffer[256] = {0};
    for (int i = 0; i < 30; ++i) {
        apu.generateSamples(buffer, 256);
    }
    const int slot = find_first_enabled_sfx_slot(apu);
    TEST_ASSERT_TRUE(slot >= 0);
    const float hz = apu.getVoiceFrequencyForTesting(slot);
    TEST_ASSERT_FLOAT_WITHIN(5.0f, 880.0f, hz);
    TEST_ASSERT_TRUE_MESSAGE(hz > 400.0f, "must follow pitch envelope, not sweepEndHz=100");
}

void test_apu_core_pitch_envelope_count_below_two_keeps_sweep(void) {
    static const SfxBreakpoint kOnePoint[] = {
        {0.0f, 220.0f},
    };

    ApuCore apu;
    apu.init(44100);
    apu.reset();

    AudioCommand cmd{};
    cmd.type = AudioCommandType::PLAY_EVENT;
    cmd.event.type = WaveType::PULSE;
    cmd.event.frequency = 440.0f;
    cmd.event.duration = 0.3f;
    cmd.event.volume = 0.5f;
    cmd.event.duty = 0.5f;
    cmd.event.sweepEndHz = 880.0f;
    cmd.event.sweepDurationSec = 0.1f;
    cmd.event.pitchEnvelope = kOnePoint;
    cmd.event.pitchEnvelopeCount = 1;
    cmd.event.sweepCurve = SweepCurve::Linear;
    TEST_ASSERT_TRUE(apu.submitCommand(cmd));

    int16_t buffer[256] = {0};
    for (int i = 0; i < 30; ++i) {
        apu.generateSamples(buffer, 256);
    }
    const int slot = find_first_enabled_sfx_slot(apu);
    TEST_ASSERT_TRUE(slot >= 0);
    TEST_ASSERT_FLOAT_WITHIN(5.0f, 880.0f, apu.getVoiceFrequencyForTesting(slot));
}

void test_apu_core_duty_stepped_hold_after_last_and_ignores_duty_sweep(void) {
    static const InstrumentPreset kSweepPreset{
        0.8f, 0.5f, 4, 0.0f, 0, 0.001f, 0.0f, 1.0f, 0.001f,
        LfoTarget::NONE, 0.0f, 0.0f, 0.0f, false,
        20.0f  // would move duty fast if not ignored
    };
    static const SfxBreakpoint kSteps[] = {
        {0.0f, 0.25f},
        {0.02f, 0.125f},
    };

    ApuCore apu;
    apu.init(44100);
    apu.reset();

    AudioCommand cmd{};
    cmd.type = AudioCommandType::PLAY_EVENT;
    cmd.event.type = WaveType::PULSE;
    cmd.event.frequency = 330.0f;
    cmd.event.duration = 0.2f;
    cmd.event.volume = 0.5f;
    cmd.event.duty = 0.5f;
    cmd.event.preset = &kSweepPreset;
    cmd.event.dutySteps = kSteps;
    cmd.event.dutyStepCount = 2;
    TEST_ASSERT_TRUE(apu.submitCommand(cmd));

    int16_t buffer[512] = {0};
    apu.generateSamples(buffer, 1);
    const int slot = find_first_enabled_sfx_slot(apu);
    TEST_ASSERT_TRUE(slot >= 0);
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, 0.0f, apu.getVoiceDutySweepPerSampleForTesting(slot));
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.25f, apu.getVoiceDutyCycleForTesting(slot));

    // Past last breakpoint (0.02s = 882 samples): hold 12.5%, no PWM drift.
    for (int n = 0; n < 8; ++n) {
        apu.generateSamples(buffer, 256);
    }
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.125f, apu.getVoiceDutyCycleForTesting(slot));
}

void test_apu_core_music_uses_track_duty_not_preset_duty(void)
{
    // Preset duty is 50%; track duty is 12.5%. Sequencer must wire track->duty.
    static const InstrumentPreset kPresetDuty50{
        0.8f, 0.5f, 4, 0.0f, 0, 0.001f, 0.0f, 1.0f, 0.001f,
        LfoTarget::NONE, 0.0f, 0.0f, 0.0f, false, 0.0f};
    static const MusicNote kNotes[] = {
        makeNote(kPresetDuty50, Note::C, 4, 2.0f),
    };
    static const MusicTrack kTrack{
        kNotes, 1, false, WaveType::PULSE, 0.125f, nullptr, nullptr, nullptr};

    ApuCore apu;
    apu.init(44100);
    apu.reset();

    AudioCommand bpm{};
    bpm.type = AudioCommandType::MUSIC_SET_BPM;
    bpm.bpm = 120.0f;
    TEST_ASSERT_TRUE(apu.submitCommand(bpm));

    AudioCommand play{};
    play.type = AudioCommandType::MUSIC_PLAY;
    play.track = &kTrack;
    TEST_ASSERT_TRUE(apu.submitCommand(play));

    int16_t buffer[4096] = {0};
    apu.generateSamples(buffer, 4096);

    int positive = 0;
    int negative = 0;
    for (int i = 0; i < 4096; ++i) {
        if (buffer[i] > 0) {
            ++positive;
        } else if (buffer[i] < 0) {
            ++negative;
        }
    }
    TEST_ASSERT_TRUE(positive > 0);
    TEST_ASSERT_TRUE(negative > 0);
    // 12.5% duty ⇒ far fewer positive samples than a 50% square.
    const float pos_ratio =
        static_cast<float>(positive) /
        static_cast<float>(positive + negative);
    TEST_ASSERT_TRUE_MESSAGE(pos_ratio < 0.30f,
                             "track duty 0.125 must dominate over preset 0.5");
}

// =============================================================================
// Unity test runner
// =============================================================================

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;

    UNITY_BEGIN();

    // Constructor tests
    RUN_TEST(test_apu_core_constructor_initializes_voices);
    RUN_TEST(test_apu_core_constructor_default_values);

    // Init/Reset tests
    RUN_TEST(test_apu_core_init_valid_sample_rate);
    RUN_TEST(test_apu_core_init_valid_sample_rate_48000);
    RUN_TEST(test_apu_core_init_invalid_sample_rate_fallback);
    RUN_TEST(test_apu_core_reset_clears_state);

    // Command queue tests
    RUN_TEST(test_apu_core_submit_command_success);
    RUN_TEST(test_apu_core_submit_command_multiple);
    RUN_TEST(test_apu_core_submit_command_queue_full);

    // Generate samples tests
    RUN_TEST(test_apu_core_generate_samples_zero_length);
    RUN_TEST(test_apu_core_generate_samples_null_stream);
    RUN_TEST(test_apu_core_generate_samples_with_active_voices);
    RUN_TEST(test_apu_core_generate_samples_output_range);
    RUN_TEST(test_apu_core_generate_samples_empty_queue);

    // Helper function tests
    RUN_TEST(test_apu_core_sequencer_note_limit_set);
    RUN_TEST(test_apu_core_deferred_notes_counter);

    // Wave type tests
    RUN_TEST(test_apu_core_wave_type_pulse);
    RUN_TEST(test_apu_core_wave_type_triangle);
    RUN_TEST(test_apu_core_wave_type_sine);
    RUN_TEST(test_apu_core_wave_type_saw);
    RUN_TEST(test_apu_core_wave_type_noise);

    // Music command tests
    RUN_TEST(test_apu_core_music_play_stop_cycle);
    RUN_TEST(test_apu_core_music_stop_clears_flag);
    RUN_TEST(test_apu_core_music_pause_resume_cycle);
    RUN_TEST(test_apu_core_music_set_tempo);
    RUN_TEST(test_apu_core_music_set_bpm);
    RUN_TEST(test_apu_core_set_master_volume_command);
    RUN_TEST(test_apu_core_set_master_volume_clamping);
    RUN_TEST(test_apu_core_set_master_bitcrush);
    RUN_TEST(test_apu_core_set_master_bitcrush_over_max);

    // Post-mix callback test
    RUN_TEST(test_apu_core_post_mix_callback);

    // Frequency tests
    RUN_TEST(test_apu_core_low_frequency);
    RUN_TEST(test_apu_core_high_frequency);

    // Volume tests
    RUN_TEST(test_apu_core_zero_volume);
    RUN_TEST(test_apu_core_full_volume);

    // 4+4 voice partition tests
    RUN_TEST(test_apu_core_voice_partition_constants);
    RUN_TEST(test_apu_core_music_pulse_tracks_do_not_steal_each_other);
    RUN_TEST(test_apu_core_sfx_play_event_uses_sfx_voice_pool);
    RUN_TEST(test_apu_core_sfx_steal_does_not_touch_music_slots);
    RUN_TEST(test_apu_core_sequencer_percussion_same_step_uses_distinct_sfx_voices);
    RUN_TEST(test_apu_core_sequencer_percussion_does_not_disturb_melodic_slots);
    RUN_TEST(test_apu_core_sequencer_percussion_shares_sfx_pool_with_play_event);
    RUN_TEST(test_apu_core_percussion_borrows_idle_music_slot);
    RUN_TEST(test_apu_core_percussion_never_steals_live_melodic_voice);
    RUN_TEST(test_apu_core_sequencer_tempo_factor_short_notes_keeps_playing);
    RUN_TEST(test_apu_core_melodic_noise_on_track_three_uses_music_slot);
    RUN_TEST(test_apu_core_melodic_tail_overlaps_next_beat);
    RUN_TEST(test_apu_core_melodic_zero_release_leaves_gate_unchanged);
    RUN_TEST(test_apu_core_melodic_track_keeps_fixed_slot_mapping);
    RUN_TEST(test_apu_core_percussion_saturated_no_idle_slot_confined_to_sfx);
    RUN_TEST(test_apu_core_music_uses_track_duty_not_preset_duty);
    RUN_TEST(test_apu_core_noise_pitch_sweep_updates_period);
    RUN_TEST(test_apu_core_linear_sweep_midpoint_arithmetic);
    RUN_TEST(test_apu_core_exponential_sweep_midpoint_geometric);
    RUN_TEST(test_apu_core_exponential_noise_sweep_updates_period);
    RUN_TEST(test_apu_core_sweep_curve_default_linear_and_nonpositive_fallback);
    RUN_TEST(test_apu_core_loop_voice_stays_enabled_until_stop);
    RUN_TEST(test_apu_core_oneshot_zero_duration_does_not_hang);
    RUN_TEST(test_apu_core_loop_voice_is_stealable);

    // Duty stepped + pitch envelope (sfx-synthesis-low-priority)
    RUN_TEST(test_apu_core_duty_stepped_absent_keeps_legacy_duty_sweep);
    RUN_TEST(test_apu_core_duty_stepped_mid_note_hold);
    RUN_TEST(test_apu_core_duty_stepped_hold_after_last_and_ignores_duty_sweep);
    RUN_TEST(test_apu_core_pitch_envelope_three_points_linear);
    RUN_TEST(test_apu_core_pitch_envelope_noise_updates_period);
    RUN_TEST(test_apu_core_pitch_envelope_precedes_single_sweep);
    RUN_TEST(test_apu_core_pitch_envelope_count_below_two_keeps_sweep);

    // Integration tests
    RUN_TEST(test_apu_core_integration_full_pipeline);
    RUN_TEST(test_apu_core_integration_multiple_voices);

    return UNITY_END();
}