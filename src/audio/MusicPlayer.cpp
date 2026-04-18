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
    currentTrack = &track;
    playing = true;
    paused = false;

    AudioCommand cmd;
    cmd.type = AudioCommandType::MUSIC_PLAY;
    cmd.track = &track;
    cmd.subTrackCount = 0;

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
    // Authoritative source is the audio thread's ApuCore atomic flag, which
    // is cleared automatically when a non-looping track finishes. We AND
    // with our local "paused" so the contract stays: true only when audio
    // is actively producing the track.
    const bool engineSaysPlaying = engine.isMusicPlaying() && !engine.isMusicPaused();
    // If the engine hasn't processed the MUSIC_PLAY command yet (race),
    // the locally-set `playing` flag keeps isPlaying() consistent from
    // the caller's point of view for the short window until the audio
    // thread picks up the command.
    if (playing && !paused && !engineSaysPlaying) {
        return true;  // Command in-flight; mirror the intent.
    }
    return engineSaysPlaying;
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
    bpm = std::max(30.0f, std::min(300.0f, newBPM));
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
