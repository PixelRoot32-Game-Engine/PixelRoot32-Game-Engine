/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#include "audio/MusicPlayer.h"
#include <algorithm>

namespace pixelroot32::audio {

MusicPlayer::MusicPlayer(AudioEngine& engine) 
    : engine(engine), currentTrack(nullptr), tempoFactor(1.0f), 
      playing(false), paused(false) {}

void MusicPlayer::play(const MusicTrack& track) {
    currentTrack = &track;
    playing = true;
    paused = false;
    
    AudioCommand cmd;
    cmd.type = AudioCommandType::MUSIC_PLAY;
    cmd.track = &track;
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
    return playing;
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

} // namespace pixelroot32::audio
