#include <unity.h>
#include "audio/MusicPlayer.h"
#include "audio/AudioEngine.h"

using namespace pixelroot32::audio;

// Simple mock for AudioEngine
class MockAudioEngine : public AudioEngine {
public:
    MockAudioEngine(const AudioConfig& config) : AudioEngine(config) {}
    
    // We don't really need to override much for MusicPlayer tests
    // because MusicPlayer uses submitCommand or playEvent
};

void setUp(void) {}
void tearDown(void) {}

void test_music_player_initial_state(void) {
    AudioConfig config;
    MockAudioEngine engine(config);
    MusicPlayer player(engine);
    
    TEST_ASSERT_FALSE(player.isPlaying());
    TEST_ASSERT_EQUAL_FLOAT(1.0f, player.getTempoFactor());
}

void test_music_player_play_stop(void) {
    AudioConfig config;
    MockAudioEngine engine(config);
    MusicPlayer player(engine);
    
    MusicNote notes[] = { {Note::C, 4, 0.5f, 1.0f} };
    MusicTrack track = { notes, 1, false, WaveType::PULSE, 0.5f };
    
    player.play(track);
    TEST_ASSERT_TRUE(player.isPlaying());
    
    player.stop();
    TEST_ASSERT_FALSE(player.isPlaying());
}

void test_music_player_pause_resume(void) {
    AudioConfig config;
    MockAudioEngine engine(config);
    MusicPlayer player(engine);
    
    MusicNote notes[] = { {Note::C, 4, 0.5f, 1.0f} };
    MusicTrack track = { notes, 1, false, WaveType::PULSE, 0.5f };
    
    player.play(track);
    player.pause();
    TEST_ASSERT_FALSE(player.isPlaying());
    
    player.resume();
    TEST_ASSERT_TRUE(player.isPlaying());
}

void test_music_player_tempo_factor(void) {
    AudioConfig config;
    MockAudioEngine engine(config);
    MusicPlayer player(engine);
    
    player.setTempoFactor(2.0f);
    TEST_ASSERT_EQUAL_FLOAT(2.0f, player.getTempoFactor());
    
    player.setTempoFactor(0.5f);
    TEST_ASSERT_EQUAL_FLOAT(0.5f, player.getTempoFactor());
}

void test_music_player_getActiveTrackCount_single_track(void) {
    AudioConfig config;
    MockAudioEngine engine(config);
    MusicPlayer player(engine);
    
    // Not playing - should return 0
    TEST_ASSERT_EQUAL(0, player.getActiveTrackCount());
    
    MusicNote notes[] = { {Note::C, 4, 0.5f, 1.0f} };
    MusicTrack track = { notes, 1, false, WaveType::PULSE, 0.5f };
    
    player.play(track);
    TEST_ASSERT_EQUAL(1, player.getActiveTrackCount());
    
    player.stop();
    TEST_ASSERT_EQUAL(0, player.getActiveTrackCount());
}

void test_music_player_getActiveTrackCount_multi_track(void) {
    AudioConfig config;
    MockAudioEngine engine(config);
    MusicPlayer player(engine);

    // Create main track
    MusicNote mainNotes[] = { {Note::C, 4, 0.5f, 1.0f} };
    MusicTrack mainTrack = { mainNotes, 1, false, WaveType::PULSE, 0.5f };

    // Create sub-tracks
    MusicNote bassNotes[] = { {Note::C, 2, 1.0f, 0.8f} };
    MusicTrack bassTrack = { bassNotes, 1, true, WaveType::PULSE, 0.25f };

    MusicNote drumNotes[] = { {Note::Rest, 0, 0.25f, 0.5f} };
    MusicTrack drumTrack = { drumNotes, 1, true, WaveType::NOISE, 0.0f };

    // Add sub-tracks to main track
    mainTrack.secondVoice = &bassTrack;
    mainTrack.thirdVoice = nullptr;
    mainTrack.percussion = &drumTrack;

    player.play(mainTrack);
    // Should be 3 tracks: main + bass + drums
    TEST_ASSERT_EQUAL(3, player.getActiveTrackCount());
}

// =============================================================================
// Value Tests for line coverage - BPM
// =============================================================================

void test_music_player_set_bpm_default(void) {
    AudioConfig config;
    MockAudioEngine engine(config);
    MusicPlayer player(engine);

    // Default BPM should be 150
    TEST_ASSERT_EQUAL_FLOAT(150.0f, player.getBPM());
}

void test_music_player_set_bpm_valid(void) {
    AudioConfig config;
    MockAudioEngine engine(config);
    MusicPlayer player(engine);

    player.setBPM(120.0f);
    TEST_ASSERT_EQUAL_FLOAT(120.0f, player.getBPM());
}

void test_music_player_set_bpm_clamp_low(void) {
    AudioConfig config;
    MockAudioEngine engine(config);
    MusicPlayer player(engine);

    // BPM below 30 should be clamped to 30
    player.setBPM(10.0f);
    TEST_ASSERT_EQUAL_FLOAT(30.0f, player.getBPM());
}

void test_music_player_set_bpm_clamp_high(void) {
    AudioConfig config;
    MockAudioEngine engine(config);
    MusicPlayer player(engine);

    // BPM above 300 should be clamped to 300
    player.setBPM(500.0f);
    TEST_ASSERT_EQUAL_FLOAT(300.0f, player.getBPM());
}

void test_music_player_set_bpm_boundary_min(void) {
    AudioConfig config;
    MockAudioEngine engine(config);
    MusicPlayer player(engine);

    // Boundary test: minimum valid BPM
    player.setBPM(30.0f);
    TEST_ASSERT_EQUAL_FLOAT(30.0f, player.getBPM());
}

void test_music_player_set_bpm_boundary_max(void) {
    AudioConfig config;
    MockAudioEngine engine(config);
    MusicPlayer player(engine);

    // Boundary test: maximum valid BPM
    player.setBPM(300.0f);
    TEST_ASSERT_EQUAL_FLOAT(300.0f, player.getBPM());
}

// =============================================================================
// Value Tests for line coverage - Master Volume
// =============================================================================

void test_music_player_set_master_volume_default(void) {
    AudioConfig config;
    MockAudioEngine engine(config);
    MusicPlayer player(engine);

    // Default master volume from AudioEngine
    float vol = player.getMasterVolume();
    TEST_ASSERT_TRUE(vol >= 0.0f && vol <= 1.0f);
}

void test_music_player_set_master_volume_valid(void) {
    AudioConfig config;
    MockAudioEngine engine(config);
    MusicPlayer player(engine);

    player.setMasterVolume(0.75f);
    TEST_ASSERT_EQUAL_FLOAT(0.75f, player.getMasterVolume());
}

void test_music_player_set_master_volume_clamp_high(void) {
    AudioConfig config;
    MockAudioEngine engine(config);
    MusicPlayer player(engine);

    // Volume above 1.0 should be clamped (AudioEngine handles clamping)
    player.setMasterVolume(1.5f);
    TEST_ASSERT_TRUE(player.getMasterVolume() <= 1.0f);
}

// =============================================================================
// Value Tests for line coverage - Third Voice (line 29)
// =============================================================================

void test_music_player_play_with_third_voice(void) {
    AudioConfig config;
    MockAudioEngine engine(config);
    MusicPlayer player(engine);

    // Create main track
    MusicNote mainNotes[] = { {Note::C, 4, 0.5f, 1.0f} };
    MusicTrack mainTrack = { mainNotes, 1, false, WaveType::PULSE, 0.5f };

    // Create third voice track
    MusicNote harmonyNotes[] = { {Note::G, 4, 0.5f, 0.5f} };
    MusicTrack harmonyTrack = { harmonyNotes, 1, true, WaveType::TRIANGLE, 0.5f };

    // Set third voice (line 29 branch)
    mainTrack.secondVoice = nullptr;
    mainTrack.thirdVoice = &harmonyTrack;
    mainTrack.percussion = nullptr;

    player.play(mainTrack);
    TEST_ASSERT_TRUE(player.isPlaying());
    TEST_ASSERT_EQUAL(2, player.getActiveTrackCount());  // main + third voice
}

// =============================================================================
// Edge cases for play/stop
// =============================================================================

void test_music_player_stop_when_not_playing(void) {
    AudioConfig config;
    MockAudioEngine engine(config);
    MusicPlayer player(engine);

    // Stop when nothing is playing - should not crash
    player.stop();
    TEST_ASSERT_FALSE(player.isPlaying());
    TEST_ASSERT_EQUAL(0, player.getActiveTrackCount());
}

void test_music_player_pause_when_not_playing(void) {
    AudioConfig config;
    MockAudioEngine engine(config);
    MusicPlayer player(engine);

    // Pause when not playing - should not crash
    player.pause();
    TEST_ASSERT_FALSE(player.isPlaying());
}

void test_music_player_resume_when_not_playing(void) {
    AudioConfig config;
    MockAudioEngine engine(config);
    MusicPlayer player(engine);

    // Resume when not playing - should not crash
    player.resume();
    TEST_ASSERT_FALSE(player.isPlaying());
}

void test_music_player_resume_when_not_paused(void) {
    AudioConfig config;
    MockAudioEngine engine(config);
    MusicPlayer player(engine);

    MusicNote notes[] = { {Note::C, 4, 0.5f, 1.0f} };
    MusicTrack track = { notes, 1, false, WaveType::PULSE, 0.5f };

    // Play without pausing, then try to resume
    player.play(track);
    TEST_ASSERT_TRUE(player.isPlaying());

    // Resume when not paused - should not crash
    player.resume();
    TEST_ASSERT_TRUE(player.isPlaying());
}

int main(int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(test_music_player_initial_state);
    RUN_TEST(test_music_player_play_stop);
    RUN_TEST(test_music_player_pause_resume);
    RUN_TEST(test_music_player_tempo_factor);
    RUN_TEST(test_music_player_getActiveTrackCount_single_track);
    RUN_TEST(test_music_player_getActiveTrackCount_multi_track);

    // BPM value tests
    RUN_TEST(test_music_player_set_bpm_default);
    RUN_TEST(test_music_player_set_bpm_valid);
    RUN_TEST(test_music_player_set_bpm_clamp_low);
    RUN_TEST(test_music_player_set_bpm_clamp_high);
    RUN_TEST(test_music_player_set_bpm_boundary_min);
    RUN_TEST(test_music_player_set_bpm_boundary_max);

    // Master volume value tests
    RUN_TEST(test_music_player_set_master_volume_default);
    RUN_TEST(test_music_player_set_master_volume_valid);
    RUN_TEST(test_music_player_set_master_volume_clamp_high);

    // Third voice test (line 29)
    RUN_TEST(test_music_player_play_with_third_voice);

    // Edge case tests
    RUN_TEST(test_music_player_stop_when_not_playing);
    RUN_TEST(test_music_player_pause_when_not_playing);
    RUN_TEST(test_music_player_resume_when_not_playing);
    RUN_TEST(test_music_player_resume_when_not_paused);

    return UNITY_END();
}
