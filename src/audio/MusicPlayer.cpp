/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#include "audio/MusicPlayer.h"
#include <algorithm>

namespace pixelroot32::audio {

MusicPlayer::MusicPlayer(AudioEngine& engine) 
    : engine(engine), currentTrack(nullptr), tempoFactor(1.0f), bpm(150.0f),
      playing(false), paused(false) {}

void MusicPlayer::play(const MusicTrack& track) {
    // Return early if main track is null
    if (!&track) return;

    currentTrack = &track;
    playing = true;
    paused = false;

    // Build command with all non-null sub-tracks
    AudioCommand cmd;
    cmd.type = AudioCommandType::MUSIC_PLAY;
    cmd.track = &track;
    cmd.subTrackCount = 0;

    // Collect all non-null sub-tracks
    if (track.secondVoice != nullptr) {
        cmd.subTracks[cmd.subTrackCount++] = track.secondVoice;
    }
    if (track.thirdVoice != nullptr) {
        cmd.subTracks[cmd.subTrackCount++] = track.thirdVoice;
    }
    if (track.percussion != nullptr) {
        cmd.subTracks[cmd.subTrackCount++] = track.percussion;
    }

    engine.submitCommand(cmd);
}

void MusicPlayer::stop() {
    playing = false;
    paused = false;
    currentTrack = nullptr;

    AudioCommand cmd;
    cmd.type = AudioCommandType::MUSIC_STOP;
    engine.submitCommand(cmd);
}

void MusicPlayer::pause() {
    if (playing) {
        paused = true;
        AudioCommand cmd;
        cmd.type = AudioCommandType::MUSIC_PAUSE;
        engine.submitCommand(cmd);
    }
}

void MusicPlayer::resume() {
    if (playing && paused) {
        paused = false;
        AudioCommand cmd;
        cmd.type = AudioCommandType::MUSIC_RESUME;
        engine.submitCommand(cmd);
    }
}

bool MusicPlayer::isPlaying() const {
    return playing && !paused;
}

void MusicPlayer::setTempoFactor(float factor) {
    tempoFactor = std::max(0.1f, factor);
    AudioCommand cmd;
    cmd.type = AudioCommandType::MUSIC_SET_TEMPO;
    cmd.tempoFactor = tempoFactor;
    engine.submitCommand(cmd);
}

float MusicPlayer::getTempoFactor() const {
    return tempoFactor;
}

void MusicPlayer::setBPM(float newBPM) {
    bpm = std::max(30.0f, std::min(300.0f, newBPM));  // Clamp to reasonable range
    AudioCommand cmd;
    cmd.type = AudioCommandType::MUSIC_SET_BPM;
    cmd.bpm = bpm;
    engine.submitCommand(cmd);
}

float MusicPlayer::getBPM() const {
    return bpm;
}

size_t MusicPlayer::getActiveTrackCount() const {
    if (!playing || !currentTrack) return 0;

    size_t count = 1; // Main track
    if (currentTrack->secondVoice) ++count;
    if (currentTrack->thirdVoice) ++count;
    if (currentTrack->percussion) ++count;
    return count;
}

} // namespace pixelroot32::audio
