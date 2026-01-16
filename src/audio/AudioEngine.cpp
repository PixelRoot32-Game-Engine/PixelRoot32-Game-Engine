#include "audio/AudioEngine.h"
#include <cstring> // for memset
#include <cmath>   // for fmod, etc.
#include <algorithm> // for std::clamp

namespace pixelroot32::audio {

    AudioEngine::AudioEngine(const AudioConfig& config) : config(config) {
        // Initialize channels
        channels[0].type = WaveType::PULSE;
        channels[1].type = WaveType::PULSE;
        channels[2].type = WaveType::TRIANGLE;
        channels[3].type = WaveType::NOISE;

        for (int i = 0; i < NUM_CHANNELS; ++i) {
            channels[i].reset();
        }
    }

    void AudioEngine::init() {
        if (config.backend) {
            config.backend->init(this);
        }
    }

    void AudioEngine::update(unsigned long deltaTime) {
        // Update durations
        for (int i = 0; i < NUM_CHANNELS; ++i) {
            AudioChannel& ch = channels[i];
            if (ch.enabled && ch.remainingMs > 0) {
                if (ch.remainingMs <= deltaTime) {
                    ch.remainingMs = 0;
                    ch.enabled = false;
                } else {
                    ch.remainingMs -= deltaTime;
                }
            }
        }
    }

    int16_t AudioEngine::generateSampleForChannel(AudioChannel& ch) {
        if (!ch.enabled) return 0;

        float sample = 0.0f;

        switch (ch.type) {
            case WaveType::PULSE:
                // Simple Pulse Wave
                sample = (ch.phase < ch.dutyCycle) ? 1.0f : -1.0f;
                break;
            
            case WaveType::TRIANGLE:
                // Triangle Wave: 0->1->0->-1->0
                // phase is 0.0 to 1.0
                if (ch.phase < 0.5f) {
                    sample = 4.0f * ch.phase - 1.0f;
                } else {
                    sample = 3.0f - 4.0f * ch.phase;
                }
                break;

            case WaveType::NOISE:
                // Simple LFSR Noise or just random for now
                // Using a simple pseudo-random check based on phase for determinism or keep state
                // Ideally, update LFSR at frequency rate.
                // For simplicity here: check if phase wrapped around
                if (ch.phase < ch.phaseIncrement) {
                     // Update noise value roughly when phase wraps
                     // This is a simplification; true NES noise is a shift register clocked by frequency
                     ch.noiseRegister = (uint16_t)(rand() & 0xFFFF);
                }
                sample = (ch.noiseRegister & 1) ? 1.0f : -1.0f;
                break;
        }

        // Advance phase
        ch.phase += ch.phaseIncrement;
        if (ch.phase >= 1.0f) {
            ch.phase -= 1.0f;
        }

        // Apply volume (approx 5000 amplitude to stay well within int16)
        return (int16_t)(sample * ch.volume * 4000.0f); 
    }

    void AudioEngine::generateSamples(int16_t* stream, int length) {
        if (!stream || length <= 0) return;

        // Clear buffer first
        memset(stream, 0, length * sizeof(int16_t));

        // Mix all channels
        for (int i = 0; i < length; ++i) {
            int32_t mixedSample = 0;

            for (int c = 0; c < NUM_CHANNELS; ++c) {
                mixedSample += generateSampleForChannel(channels[c]);
            }

            // Hard clipper / Limiter
            if (mixedSample > 32767) mixedSample = 32767;
            if (mixedSample < -32768) mixedSample = -32768;

            stream[i] = (int16_t)mixedSample;
        }
    }

    void AudioEngine::playEvent(const AudioEvent& event) {
        AudioChannel* ch = findFreeChannel(event.type);
        if (ch) {
            ch->enabled = true;
            ch->frequency = event.frequency;
            ch->phase = 0.0f;
            ch->phaseIncrement = event.frequency / (float)config.sampleRate;
            ch->volume = event.volume;
            ch->durationMs = (unsigned long)(event.duration * 1000.0f);
            ch->remainingMs = ch->durationMs;
            
            if (event.type == WaveType::PULSE) {
                ch->dutyCycle = event.duty;
            }
        }
    }

    AudioChannel* AudioEngine::findFreeChannel(WaveType type) {
        // Priority: Find a channel of correct type that is NOT enabled.
        // If all busy, steal the one with least remaining time (simple voice stealing).
        
        AudioChannel* candidate = nullptr;
        unsigned long minRemaining = 0xFFFFFFFF;

        for (int i = 0; i < NUM_CHANNELS; ++i) {
            if (channels[i].type == type) {
                if (!channels[i].enabled) {
                    return &channels[i]; // Found free channel
                }
                
                // Track for stealing
                if (channels[i].remainingMs < minRemaining) {
                    minRemaining = channels[i].remainingMs;
                    candidate = &channels[i];
                }
            }
        }
        
        // If we are here, all matching channels are busy. Steal the oldest one.
        return candidate;
    }

}
