/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#include "audio/DefaultAudioScheduler.h"
#include <cstring>
#include <cmath>
#include <algorithm>

namespace pixelroot32::audio {

DefaultAudioScheduler::DefaultAudioScheduler() {
    channels[0].type = WaveType::PULSE;
    channels[1].type = WaveType::PULSE;
    channels[2].type = WaveType::TRIANGLE;
    channels[3].type = WaveType::NOISE;

    for (int i = 0; i < NUM_CHANNELS; ++i) {
        channels[i].reset();
    }
}

void DefaultAudioScheduler::init(AudioBackend* /*backend*/, int sampleRate) {
    this->sampleRate = sampleRate;
}

void DefaultAudioScheduler::submitCommand(const AudioCommand& cmd) {
    commandQueue.enqueue(cmd);
}

void DefaultAudioScheduler::start() {
    running = true;
}

void DefaultAudioScheduler::stop() {
    running = false;
}

bool DefaultAudioScheduler::isIndependent() const {
    return false; // For now, DefaultAudioScheduler is driven by the backend callback
}

void DefaultAudioScheduler::generateSamples(int16_t* stream, int length) {
    if (!stream || length <= 0) return;

    processCommands();

    memset(stream, 0, length * sizeof(int16_t));

    for (int i = 0; i < length; ++i) {
        int32_t mixedSample = 0;

        for (int c = 0; c < NUM_CHANNELS; ++c) {
            mixedSample += generateSampleForChannel(channels[c]);
        }

        // Hard clipper / Limiter
        if (mixedSample > 32767) mixedSample = 32767;
        if (mixedSample < -32768) mixedSample = -32768;

        stream[i] = (int16_t)mixedSample;
        audioTimeSamples++;
    }
}

void DefaultAudioScheduler::processCommands() {
    AudioCommand cmd;
    while (commandQueue.dequeue(cmd)) {
        switch (cmd.type) {
            case AudioCommandType::PLAY_EVENT:
                executePlayEvent(cmd.event);
                break;
            case AudioCommandType::SET_MASTER_VOLUME:
                masterVolume = std::clamp(cmd.volume, 0.0f, 1.0f);
                break;
            case AudioCommandType::STOP_CHANNEL:
                if (cmd.channelIndex < NUM_CHANNELS) {
                    channels[cmd.channelIndex].reset();
                }
                break;
            default:
                break;
        }
    }
}

void DefaultAudioScheduler::executePlayEvent(const AudioEvent& event) {
    AudioChannel* ch = findFreeChannel(event.type);
    if (ch) {
        ch->enabled = true;
        ch->frequency = event.frequency;
        ch->phase = 0.0f;
        ch->phaseIncrement = event.frequency / (float)sampleRate;
        ch->volume = event.volume;
        
        // Convert seconds to samples
        ch->remainingSamples = (uint64_t)(event.duration * (float)sampleRate);
        ch->durationMs = (unsigned long)(event.duration * 1000.0f);
        ch->remainingMs = ch->durationMs;
        
        if (event.type == WaveType::PULSE) {
            ch->dutyCycle = event.duty;
        }
    }
}

AudioChannel* DefaultAudioScheduler::findFreeChannel(WaveType type) {
    AudioChannel* candidate = nullptr;
    uint64_t minRemaining = 0xFFFFFFFFFFFFFFFF;

    for (int i = 0; i < NUM_CHANNELS; ++i) {
        if (channels[i].type == type) {
            if (!channels[i].enabled) {
                return &channels[i];
            }
            if (channels[i].remainingSamples < minRemaining) {
                minRemaining = channels[i].remainingSamples;
                candidate = &channels[i];
            }
        }
    }
    return candidate;
}

int16_t DefaultAudioScheduler::generateSampleForChannel(AudioChannel& ch) {
    if (!ch.enabled) return 0;

    float sample = 0.0f;
    switch (ch.type) {
        case WaveType::PULSE:
            sample = (ch.phase < ch.dutyCycle) ? 1.0f : -1.0f;
            break;
        case WaveType::TRIANGLE:
            if (ch.phase < 0.5f) {
                sample = 4.0f * ch.phase - 1.0f;
            } else {
                sample = 3.0f - 4.0f * ch.phase;
            }
            break;
        case WaveType::NOISE:
            if (ch.phase < ch.phaseIncrement) {
                ch.noiseRegister = (uint16_t)(rand() & 0xFFFF);
            }
            sample = (ch.noiseRegister & 1) ? 1.0f : -1.0f;
            break;
    }

    ch.phase += ch.phaseIncrement;
    if (ch.phase >= 1.0f) ch.phase -= 1.0f;

    if (ch.remainingSamples > 0) {
        ch.remainingSamples--;
    } else {
        ch.enabled = false;
    }

    return (int16_t)(sample * ch.volume * masterVolume * 12000.0f);
}

} // namespace pixelroot32::audio
