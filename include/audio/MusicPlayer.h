#pragma once

#include "AudioEngine.h"
#include "AudioMusicTypes.h"
#include <cstddef> // Required for size_t

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

    /**
     * @brief Sets the global tempo scaling factor.
     * @param factor 1.0f is normal speed, 2.0f is double speed.
     */
    void setTempoFactor(float factor);

    /**
     * @brief Gets the current tempo scaling factor.
     * @return Current factor (default 1.0f).
     */
    float getTempoFactor() const;

private:
    AudioEngine& engine;
    const MusicTrack* currentTrack;
    size_t currentNoteIndex;
    float noteTimer; // Accumulated time in seconds
    float tempoFactor; // Global speed multiplier
    bool playing;
    bool paused;

    void playCurrentNote();
};

} // namespace pixelroot32::audio
