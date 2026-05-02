/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#pragma once

#include "AudioEngine.h"
#include "AudioMusicTypes.h"
#include "AudioTypes.h"
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

    /**
     * @brief Sets the tempo in BPM (beats per minute).
     * @param bpm Beats per minute (default 150).
     */
    void setBPM(float bpm);

    /**
     * @brief Gets the current BPM setting.
     * @return Current BPM (default 150).
     */
    float getBPM() const;

    /**
     * @brief Gets the number of currently active tracks.
     * @return Number of active tracks (1-4), 0 if not playing.
     */
    size_t getActiveTrackCount() const;

    /**
     * @brief Sets the master volume level.
     * @param volume Volume level (0.0f = silent, 1.0f = full volume).
     */
    void setMasterVolume(float volume);

    /**
     * @brief Gets the current master volume level.
     * @return Current volume (0.0f - 1.0f).
     */
    float getMasterVolume() const;

private:
    AudioEngine& engine;
    const MusicTrack* currentTrack;
    float tempoFactor; // Global speed multiplier (1.0 = normal)
    float bpm;         // Beats per minute (default 150)
    bool playing;
    bool paused;
};

} // namespace pixelroot32::audio
