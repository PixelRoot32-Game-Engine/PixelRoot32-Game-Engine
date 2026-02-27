/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#include "audio/DefaultAudioScheduler.h"
#include "audio/AudioMixerLUT.h"
#include "platforms/EngineConfig.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <cstdlib>
#include <cstdio>

#ifdef ESP32
#include <Arduino.h>
#include <soc/soc_caps.h>
#endif

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

void DefaultAudioScheduler::init(AudioBackend* /*backend*/, int sampleRate, const pixelroot32::platforms::PlatformCapabilities& /*caps*/) {
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

    // Advance music sequencer (Phase 3)
    updateMusicSequencer(length);

    static float currentPeak = 0.0f;
    const float FINAL_SCALE = 32767.0f;

#if defined(SOC_CPU_HAS_FPU) && SOC_CPU_HAS_FPU
    // FPU version
    const float MIXER_SCALE = 0.4f;
    const float MIXER_K = 0.5f;

    for (int i = 0; i < length; ++i) {
        float acc = 0.0f;
        for (int c = 0; c < NUM_CHANNELS; ++c) {
            if (channels[c].enabled) {
                acc += generateSampleForChannel(channels[c]) * MIXER_SCALE;
            }
        }

        acc *= masterVolume;

        float mixed = acc / (1.0f + std::abs(acc) * MIXER_K);
        float finalSample = mixed * FINAL_SCALE;
        
        float absSample = std::abs(finalSample);
        if (absSample > currentPeak) currentPeak = absSample;

        if (finalSample > 32767.0f) finalSample = 32767.0f;
        if (finalSample < -32768.0f) finalSample = -32768.0f;

        stream[i] = (int16_t)finalSample;
    }
#elif defined(ESP32)
    // ESP32 No-FPU version (LUT)
    for (int i = 0; i < length; ++i) {
        int32_t sum = 0;
        for (int c = 0; c < NUM_CHANNELS; ++c) {
            if (channels[c].enabled) {
                sum += (int32_t)(generateSampleForChannel(channels[c]) * FINAL_SCALE);
            }
        }

        if (masterVolume != 1.0f) {
            sum = (int32_t)(sum * masterVolume);
        }

        int32_t index = (sum + 131072) >> 8;
        if (index < 0) index = 0;
        if (index > 1024) index = 1024;
        
        int16_t finalSample = audio_mixer_lut[index];
        stream[i] = finalSample;

        float absSample = (float)std::abs((int32_t)finalSample);
        if (absSample > currentPeak) currentPeak = absSample;
    }
#else
    // Native / Fallback (Always use float for simplicity if not on ESP32)
    const float MIXER_SCALE = 0.4f;
    const float MIXER_K = 0.5f;

    for (int i = 0; i < length; ++i) {
        float acc = 0.0f;
        for (int c = 0; c < NUM_CHANNELS; ++c) {
            if (channels[c].enabled) {
                acc += generateSampleForChannel(channels[c]) * MIXER_SCALE;
            }
        }

        acc *= masterVolume;

        float mixed = acc / (1.0f + std::abs(acc) * MIXER_K);
        float finalSample = mixed * FINAL_SCALE;
        
        float absSample = std::abs(finalSample);
        if (absSample > currentPeak) currentPeak = absSample;

        if (finalSample > 32767.0f) finalSample = 32767.0f;
        if (finalSample < -32768.0f) finalSample = -32768.0f;

        stream[i] = (int16_t)finalSample;
    }
#endif

    // Using simple counter or clock if available, but for Default we might not have FreeRTOS
    // Let's use a simple sample counter based logging
    static uint64_t totalSamplesProcessed = 0;
    totalSamplesProcessed += length;
    if (totalSamplesProcessed >= (uint64_t)sampleRate) { // Approx every second
        if constexpr (pixelroot32::platforms::config::EnableProfiling) {
#ifdef ESP32
            if (currentPeak > 32767.0f) {
                Serial.printf("[AUDIO] PEAK DETECTED: %.0f (CLIPPING!)\n", currentPeak);
            } else {
                Serial.printf("[AUDIO] Peak: %.0f (%.1f%%)\n", currentPeak, (currentPeak / 32767.0f) * 100.0f);
            }
#else
            if (currentPeak > 32767.0f) {
                printf("[AUDIO] PEAK DETECTED: %.0f (CLIPPING!)\n", currentPeak);
            } else {
                printf("[AUDIO] Peak: %.0f (%.1f%%)\n", currentPeak, (currentPeak / 32767.0f) * 100.0f);
            }
#endif
        }
        currentPeak = 0.0f;
        totalSamplesProcessed = 0;
    }
    
    audioTimeSamples += length;
}

void DefaultAudioScheduler::processCommands() {
    AudioCommand cmd;
    while (commandQueue.dequeue(cmd)) {
        switch (cmd.type) {
            case AudioCommandType::PLAY_EVENT:
                executePlayEvent(cmd.event);
                break;
            case AudioCommandType::SET_MASTER_VOLUME:
                masterVolume = cmd.volume;
                if (masterVolume > 1.0f) masterVolume = 1.0f;
                if (masterVolume < 0.0f) masterVolume = 0.0f;
                break;
            case AudioCommandType::STOP_CHANNEL:
                if (cmd.channelIndex < NUM_CHANNELS) {
                    channels[cmd.channelIndex].reset();
                }
                break;
            case AudioCommandType::MUSIC_PLAY:
                currentTrack = cmd.track;
                currentNoteIndex = 0;
                nextNoteSample = audioTimeSamples;
                musicPlaying = true;
                musicPaused = false;
                break;
            case AudioCommandType::MUSIC_STOP:
                musicPlaying = false;
                currentTrack = nullptr;
                break;
            case AudioCommandType::MUSIC_PAUSE:
                musicPaused = true;
                break;
            case AudioCommandType::MUSIC_RESUME:
                musicPaused = false;
                break;
            case AudioCommandType::MUSIC_SET_TEMPO:
                tempoFactor = std::max(0.1f, cmd.tempoFactor);
                break;
            default:
                break;
        }
    }
}

void DefaultAudioScheduler::updateMusicSequencer(int /*length*/) {
    if (!musicPlaying || musicPaused || !currentTrack) return;

    while (musicPlaying && currentTrack && audioTimeSamples >= nextNoteSample) {
        playCurrentNote();

        // Calculate when the next note should play
        const MusicNote& note = currentTrack->notes[currentNoteIndex];
        uint64_t noteDurationSamples = (uint64_t)((note.duration / tempoFactor) * (float)sampleRate);
        nextNoteSample += noteDurationSamples;

        currentNoteIndex++;
        if (currentNoteIndex >= currentTrack->count) {
            if (currentTrack->loop) {
                currentNoteIndex = 0;
            } else {
                musicPlaying = false;
                currentTrack = nullptr;
            }
        }

        // Safety: if we are too far behind, catch up
        if (nextNoteSample < audioTimeSamples && musicPlaying) {
             // Optional: handle extreme lag by skipping notes or just letting it run fast
        }
    }
}

void DefaultAudioScheduler::playCurrentNote() {
    if (!currentTrack) return;
    const MusicNote& note = currentTrack->notes[currentNoteIndex];
    if (note.note == Note::Rest) return;

    AudioEvent event;
    event.type = currentTrack->channelType;
    event.frequency = noteToFrequency(note.note, note.octave);
    event.duration = note.duration / tempoFactor;
    event.volume = note.volume;
    event.duty = (event.type == WaveType::PULSE) ? currentTrack->duty : 0.5f;

    executePlayEvent(event);
}

void DefaultAudioScheduler::executePlayEvent(const AudioEvent& event) {
    AudioChannel* ch = findFreeChannel(event.type);
    if (ch) {
        ch->enabled = true;
        ch->frequency = event.frequency;
        ch->phase = 0.0f;
        ch->phaseIncrement = event.frequency / (float)sampleRate;
        
        // Phase 4: Volume interpolation setup
        // For a simple play event, we start at volume and stay there
        // or we could implement a quick fade-in to avoid clicks.
        ch->volume = event.volume;
        ch->targetVolume = event.volume;
        ch->volumeDelta = 0.0f;
        
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

float DefaultAudioScheduler::generateSampleForChannel(AudioChannel& ch) {
    if (!ch.enabled) return 0.0f;

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

    // Phase 4: Volume interpolation
    if (ch.volumeDelta != 0.0f) {
        ch.volume += ch.volumeDelta;
        // Check if we reached target
        if ((ch.volumeDelta > 0.0f && ch.volume >= ch.targetVolume) ||
            (ch.volumeDelta < 0.0f && ch.volume <= ch.targetVolume)) {
            ch.volume = ch.targetVolume;
            ch.volumeDelta = 0.0f;
        }
    }

    if (ch.remainingSamples > 0) {
        ch.remainingSamples--;
    } else {
        ch.enabled = false;
    }

    return sample * ch.volume;
}

} // namespace pixelroot32::audio
