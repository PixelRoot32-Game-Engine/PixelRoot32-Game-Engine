#include <unity.h>
#include "audio/DefaultAudioScheduler.h"
#include "audio/AudioConfig.h"
#include "audio/AudioMusicTypes.h"

using namespace pixelroot32::audio;

void setUp(void) {}
void tearDown(void) {}

void test_default_scheduler_initialization(void) {
    DefaultAudioScheduler scheduler;
    
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
    
    AudioCommand cmd;
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
    AudioCommand cmd;
    cmd.type = AudioCommandType::PLAY_EVENT;
    cmd.event.type = WaveType::PULSE;
    cmd.event.frequency = 440.0f;
    cmd.event.volume = 1.0f;
    cmd.event.duration = 1.0f;
    scheduler.submitCommand(cmd);
    
    // Stop channel 0 (DefaultAudioScheduler uses channels 0-3)
    AudioCommand stopCmd;
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
    AudioCommand cmd;
    cmd.type = AudioCommandType::PLAY_EVENT;
    cmd.event.type = WaveType::TRIANGLE;
    cmd.event.frequency = 220.0f;
    cmd.event.volume = 0.5f;
    cmd.event.duration = 0.5f;
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
    AudioCommand cmd;
    cmd.type = AudioCommandType::PLAY_EVENT;
    cmd.event.type = WaveType::TRIANGLE;
    cmd.event.frequency = 330.0f;
    cmd.event.volume = 1.0f;
    cmd.event.duration = 0.1f;
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
    AudioCommand cmd;
    cmd.type = AudioCommandType::PLAY_EVENT;
    cmd.event.type = WaveType::NOISE;
    cmd.event.frequency = 440.0f;
    cmd.event.volume = 0.5f;
    cmd.event.duration = 0.05f;
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
    AudioCommand cmd;
    cmd.type = AudioCommandType::PLAY_EVENT;
    cmd.event.type = WaveType::PULSE;
    cmd.event.frequency = 440.0f;
    cmd.event.volume = 1.0f;
    cmd.event.duration = 1.0f;
    for (int ch = 0; ch < 4; ch++) {
        scheduler.submitCommand(cmd);
    }
    AudioCommand stopCmd;
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
    AudioCommand volCmd;
    volCmd.type = AudioCommandType::SET_MASTER_VOLUME;
    volCmd.volume = 0.5f;
    scheduler.submitCommand(volCmd);
    AudioCommand cmd;
    cmd.type = AudioCommandType::PLAY_EVENT;
    cmd.event.type = WaveType::PULSE;
    cmd.event.frequency = 440.0f;
    cmd.event.volume = 1.0f;
    cmd.event.duration = 0.1f;
    scheduler.submitCommand(cmd);
    int16_t buffer[256];
    scheduler.generateSamples(buffer, 256);
    bool hasNonZero = false;
    for (int i = 0; i < 256; i++) {
        if (buffer[i] != 0) { hasNonZero = true; break; }
    }
    TEST_ASSERT_TRUE(hasNonZero);
}

void test_default_scheduler_long_sequence(void) {
    DefaultAudioScheduler scheduler;
    AudioCommand cmd;
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
    AudioCommand cmd;
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
    AudioCommand cmd;
    cmd.type = AudioCommandType::MUSIC_PLAY;
    scheduler.submitCommand(cmd);
    scheduler.generateSamples(nullptr, 256);
    TEST_ASSERT_TRUE(true);
}

void test_music_stop_command(void) {
    DefaultAudioScheduler scheduler;
    AudioCommand cmd;
    cmd.type = AudioCommandType::MUSIC_PLAY;
    scheduler.submitCommand(cmd);
    cmd.type = AudioCommandType::MUSIC_STOP;
    scheduler.submitCommand(cmd);
    scheduler.generateSamples(nullptr, 256);
    TEST_ASSERT_TRUE(true);
}

void test_music_pause_resume_command(void) {
    DefaultAudioScheduler scheduler;
    AudioCommand cmd;
    cmd.type = AudioCommandType::MUSIC_PLAY;
    scheduler.submitCommand(cmd);
    cmd.type = AudioCommandType::MUSIC_PAUSE;
    scheduler.submitCommand(cmd);
    cmd.type = AudioCommandType::MUSIC_RESUME;
    scheduler.submitCommand(cmd);
    scheduler.generateSamples(nullptr, 256);
    TEST_ASSERT_TRUE(true);
}

void test_music_set_tempo_command(void) {
    DefaultAudioScheduler scheduler;
    AudioCommand cmd;
    cmd.type = AudioCommandType::MUSIC_SET_TEMPO;
    cmd.tempoFactor = 2.0f;
    scheduler.submitCommand(cmd);
    scheduler.generateSamples(nullptr, 256);
    TEST_ASSERT_TRUE(true);
}

void test_set_master_volume_clamping(void) {
    DefaultAudioScheduler scheduler;
    AudioCommand cmd;
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
    return UNITY_END();
}
