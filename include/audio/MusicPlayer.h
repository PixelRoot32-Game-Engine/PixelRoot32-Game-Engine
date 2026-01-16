#pragma once

#include <cstddef>
#include "AudioEngine.h"
#include "AudioMusicTypes.h"

namespace pixelroot32::audio {

/**
 * @class MusicPlayer
 * @brief Simple sequencer to play MusicTracks.
 */
class MusicPlayer {
public:
    /**
     * @brief Constructs the MusicPlayer.
     * @param engine Reference to the AudioEngine used to play sounds.
     */
    MusicPlayer(AudioEngine& engine);

    /**
     * @brief Starts playing a track.
     * @param track The track to play.
     */
    void play(const MusicTrack& track);

    /**
     * @brief Stops playback and silences the channel.
     */
    void stop();

    /**
     * @brief Pauses playback.
     */
    void pause();

    /**
     * @brief Resumes playback.
     */
    void resume();

    /**
     * @brief Updates the player state.
     * Should be called every frame.
     * @param deltaTime Time elapsed since last frame in milliseconds.
     */
    void update(unsigned long deltaTime);

    /**
     * @brief Checks if a track is currently playing.
     * @return true if playing, false otherwise.
     */
    bool isPlaying() const;

private:
    AudioEngine& engine;
    const MusicTrack* currentTrack;
    size_t currentNoteIndex;
    float noteTimer; // Accumulated time in seconds
    bool playing;
    bool paused;

    void playCurrentNote();
};

} // namespace pixelroot32::audio
