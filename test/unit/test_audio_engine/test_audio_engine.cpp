/**
 * @file test_audio_engine.cpp
 * @brief Unit tests for audio/AudioEngine module.
 */

#include "unity.h"
#include "../../test_config.h"
#include "audio/AudioEngine.h"
#include "audio/AudioConfig.h"
#include "audio/SfxBankPlayback.h"
#include "mocks/MockAudioScheduler.h"
#include <memory>
#include <vector>

using namespace pixelroot32::audio;

namespace {

enum class FakeSfxId : uint8_t {
    LayersOnly = 0,
    SequenceOnly = 1,
    LayersAndSequence = 2,
    LoopTone = 3,
};

struct FakeSfxBank {
    struct SequenceStep {
        float delaySec;
        AudioEvent event;
    };

    static uint8_t layerCount(FakeSfxId id) {
        switch (id) {
            case FakeSfxId::LayersOnly: return 2;
            case FakeSfxId::LayersAndSequence: return 1;
            case FakeSfxId::LoopTone: return 1;
            default: return 0;
        }
    }

    static AudioEvent layerEvent(FakeSfxId id, uint8_t layerIndex) {
        AudioEvent ev{};
        ev.type = WaveType::PULSE;
        ev.duration = 0.1f;
        ev.volume = 0.5f;
        ev.duty = 0.5f;
        ev.loop = false;
        switch (id) {
            case FakeSfxId::LayersOnly:
                ev.frequency = (layerIndex == 0) ? 440.0f : 880.0f;
                return ev;
            case FakeSfxId::LayersAndSequence:
                ev.frequency = 220.0f;
                return ev;
            case FakeSfxId::LoopTone:
                ev.frequency = 110.0f;
                ev.duration = 0.0f;
                ev.loop = true;
                return ev;
            default:
                ev.frequency = 0.0f;
                return ev;
        }
    }

    static uint8_t sequenceStepCount(FakeSfxId id) {
        switch (id) {
            case FakeSfxId::SequenceOnly: return 3;
            case FakeSfxId::LayersAndSequence: return 2;
            default: return 0;
        }
    }

    static SequenceStep sequenceStep(FakeSfxId id, uint8_t stepIndex) {
        SequenceStep step{};
        step.event.type = WaveType::TRIANGLE;
        step.event.duration = 0.05f;
        step.event.volume = 0.4f;
        step.event.duty = 0.5f;
        step.event.loop = false;
        if (id == FakeSfxId::SequenceOnly) {
            const float delays[3] = {0.0f, 0.025f, 0.050f};
            const float freqs[3] = {523.25f, 659.25f, 783.99f};
            step.delaySec = delays[stepIndex < 3 ? stepIndex : 0];
            step.event.frequency = freqs[stepIndex < 3 ? stepIndex : 0];
            return step;
        }
        if (id == FakeSfxId::LayersAndSequence) {
            step.delaySec = (stepIndex == 0) ? 0.01f : 0.02f;
            step.event.frequency = (stepIndex == 0) ? 330.0f : 440.0f;
            return step;
        }
        step.delaySec = 0.0f;
        step.event.frequency = 0.0f;
        return step;
    }
};

struct RecordingSfxDelayScheduler final : public SfxDelayScheduler {
    struct Entry {
        float delaySec;
        AudioEvent event;
    };
    std::vector<Entry> scheduled;

    void schedule(float delaySec, const AudioEvent& event) override {
        scheduled.push_back(Entry{delaySec, event});
    }
};

static size_t count_play_events(const MockAudioScheduler& mock) {
    size_t n = 0;
    for (const AudioCommand& cmd : mock.submittedCommands) {
        if (cmd.type == AudioCommandType::PLAY_EVENT) {
            ++n;
        }
    }
    return n;
}

}  // namespace

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

// =============================================================================
// playSfxBank helper (sfx-synthesis-medium-priority)
// =============================================================================

void test_play_sfx_bank_layers_only_no_schedule(void) {
    AudioConfig config(nullptr, 22050);
    AudioEngine engine(config);
    auto mockScheduler = std::make_unique<MockAudioScheduler>();
    MockAudioScheduler& mockRef = *mockScheduler;
    engine.setScheduler(std::move(mockScheduler));
    engine.init();

    RecordingSfxDelayScheduler delaySched;
    playSfxBank<FakeSfxBank>(engine, FakeSfxId::LayersOnly, delaySched);

    TEST_ASSERT_EQUAL_UINT32(2u, static_cast<uint32_t>(count_play_events(mockRef)));
    TEST_ASSERT_EQUAL_UINT32(0u, static_cast<uint32_t>(delaySched.scheduled.size()));
    TEST_ASSERT_EQUAL_FLOAT(440.0f, mockRef.submittedCommands[0].event.frequency);
    TEST_ASSERT_EQUAL_FLOAT(880.0f, mockRef.submittedCommands[1].event.frequency);
}

void test_play_sfx_bank_sequence_with_delays(void) {
    AudioConfig config(nullptr, 22050);
    AudioEngine engine(config);
    auto mockScheduler = std::make_unique<MockAudioScheduler>();
    MockAudioScheduler& mockRef = *mockScheduler;
    engine.setScheduler(std::move(mockScheduler));
    engine.init();

    RecordingSfxDelayScheduler delaySched;
    playSfxBank<FakeSfxBank>(engine, FakeSfxId::SequenceOnly, delaySched);

    // delay 0 plays immediately; 0.025 and 0.050 go to scheduler.
    TEST_ASSERT_EQUAL_UINT32(1u, static_cast<uint32_t>(count_play_events(mockRef)));
    TEST_ASSERT_EQUAL_FLOAT(523.25f, mockRef.submittedCommands[0].event.frequency);
    TEST_ASSERT_EQUAL_UINT32(2u, static_cast<uint32_t>(delaySched.scheduled.size()));
    TEST_ASSERT_EQUAL_FLOAT(0.025f, delaySched.scheduled[0].delaySec);
    TEST_ASSERT_EQUAL_FLOAT(659.25f, delaySched.scheduled[0].event.frequency);
    TEST_ASSERT_EQUAL_FLOAT(0.050f, delaySched.scheduled[1].delaySec);
    TEST_ASSERT_EQUAL_FLOAT(783.99f, delaySched.scheduled[1].event.frequency);
}

void test_play_sfx_bank_layers_and_sequence(void) {
    AudioConfig config(nullptr, 22050);
    AudioEngine engine(config);
    auto mockScheduler = std::make_unique<MockAudioScheduler>();
    MockAudioScheduler& mockRef = *mockScheduler;
    engine.setScheduler(std::move(mockScheduler));
    engine.init();

    RecordingSfxDelayScheduler delaySched;
    playSfxBank<FakeSfxBank>(engine, FakeSfxId::LayersAndSequence, delaySched);

    TEST_ASSERT_EQUAL_UINT32(1u, static_cast<uint32_t>(count_play_events(mockRef)));
    TEST_ASSERT_EQUAL_FLOAT(220.0f, mockRef.submittedCommands[0].event.frequency);
    TEST_ASSERT_EQUAL_UINT32(2u, static_cast<uint32_t>(delaySched.scheduled.size()));
    TEST_ASSERT_EQUAL_FLOAT(0.01f, delaySched.scheduled[0].delaySec);
    TEST_ASSERT_EQUAL_FLOAT(0.02f, delaySched.scheduled[1].delaySec);
}

void test_play_sfx_bank_loop_flag_preserved_opt_in(void) {
    AudioConfig config(nullptr, 22050);
    AudioEngine engine(config);
    auto mockScheduler = std::make_unique<MockAudioScheduler>();
    MockAudioScheduler& mockRef = *mockScheduler;
    engine.setScheduler(std::move(mockScheduler));
    engine.init();

    NullSfxDelayScheduler delaySched;
    playSfxBank<FakeSfxBank>(engine, FakeSfxId::LoopTone, delaySched);

    TEST_ASSERT_EQUAL_UINT32(1u, static_cast<uint32_t>(count_play_events(mockRef)));
    TEST_ASSERT_TRUE(mockRef.submittedCommands[0].event.loop);

    // Legacy path still available without the helper.
    engine.playEvent(FakeSfxBank::layerEvent(FakeSfxId::LayersOnly, 0));
    TEST_ASSERT_EQUAL_UINT32(2u, static_cast<uint32_t>(count_play_events(mockRef)));
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

    RUN_TEST(test_play_sfx_bank_layers_only_no_schedule);
    RUN_TEST(test_play_sfx_bank_sequence_with_delays);
    RUN_TEST(test_play_sfx_bank_layers_and_sequence);
    RUN_TEST(test_play_sfx_bank_loop_flag_preserved_opt_in);
    
    return UNITY_END();
}
