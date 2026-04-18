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

int main(int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(test_music_player_initial_state);
    RUN_TEST(test_music_player_play_stop);
    RUN_TEST(test_music_player_pause_resume);
    RUN_TEST(test_music_player_tempo_factor);
    RUN_TEST(test_music_player_getActiveTrackCount_single_track);
    RUN_TEST(test_music_player_getActiveTrackCount_multi_track);
    return UNITY_END();
}
