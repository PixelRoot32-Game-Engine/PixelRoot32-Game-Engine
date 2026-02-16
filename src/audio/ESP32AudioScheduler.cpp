/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#ifdef ESP32

#include "drivers/esp32/ESP32AudioScheduler.h"
#include "audio/AudioMixerLUT.h"
#include <Arduino.h>
#include <algorithm>
#include <cstring>
#include <cstdlib>
#include <soc/soc_caps.h>

namespace pixelroot32::audio {

    ESP32AudioScheduler::ESP32AudioScheduler(int coreId, int priority)
        : coreId(coreId), priority(priority) {
    }

    ESP32AudioScheduler::~ESP32AudioScheduler() {
        stop();
    }

    void ESP32AudioScheduler::init(AudioBackend* backend, int sampleRate, const pixelroot32::core::PlatformCapabilities& /*caps*/) {
        this->backend = backend;
        this->sampleRate = sampleRate;
        for (int i = 0; i < NUM_CHANNELS; i++) {
            channels[i].reset();
        }

        // Initialize NES-like channel types
        if (NUM_CHANNELS >= 4) {
            channels[0].type = WaveType::PULSE;
            channels[1].type = WaveType::PULSE;
            channels[2].type = WaveType::TRIANGLE;
            channels[3].type = WaveType::NOISE;
        }
    }

    void ESP32AudioScheduler::submitCommand(const AudioCommand& cmd) {
        commandQueue.enqueue(cmd);
    }

    void ESP32AudioScheduler::start() {
        if (running) return;
        running = true;
        // Note: Currently backends create their own task.
        // If we wanted the scheduler to drive the backend, we would create a task here.
        // For now, we'll let the backend drive generateSamples.
    }

    void ESP32AudioScheduler::stop() {
        running = false;
        if (taskHandle) {
            vTaskDelete(taskHandle);
            taskHandle = nullptr;
        }
    }

    void ESP32AudioScheduler::generateSamples(int16_t* stream, int length) {
        if (!stream || length <= 0) return;

        processCommands();
        updateMusicSequencer(length);

        static float currentPeak = 0.0f;
        const float FINAL_SCALE = 32767.0f;

#if defined(SOC_CPU_HAS_FPU) && SOC_CPU_HAS_FPU
        // ---------------------------------------------------------------------
        // FPU-Optimized Mixing (ESP32, S3)
        // Uses floating-point non-linear compression
        // ---------------------------------------------------------------------
        const float MIXER_SCALE = 0.4f; // 40% per channel (Non-linear)
        const float MIXER_K = 0.5f;     // Compression factor

        for (int j = 0; j < length; j++) {
            float acc = 0.0f;
            for (int i = 0; i < NUM_CHANNELS; i++) {
                if (channels[i].enabled) {
                    acc += generateSampleForChannel(channels[i]) * MIXER_SCALE;
                }
            }

            acc *= masterVolume;

            // Non-linear mixing formula: f(x) = x / (1 + |x| * K)
            float mixed = acc / (1.0f + std::abs(acc) * MIXER_K);
            float finalSample = mixed * FINAL_SCALE;
            
            float absSample = std::abs(finalSample);
            if (absSample > currentPeak) currentPeak = absSample;

            if (finalSample > 32767.0f) finalSample = 32767.0f;
            if (finalSample < -32768.0f) finalSample = -32768.0f;
            stream[j] = (int16_t)finalSample;
        }
#else
        // ---------------------------------------------------------------------
        // Integer-Optimized Mixing (ESP32-C3, RISC-V No FPU)
        // Uses Look-Up Table (LUT) for non-linear compression
        // ---------------------------------------------------------------------
        for (int j = 0; j < length; j++) {
            int32_t sum = 0;
            for (int i = 0; i < NUM_CHANNELS; i++) {
                if (channels[i].enabled) {
                    // Convert float channel output to int32 sum
                    // Each channel at 1.0 volume contributes +/- 32767
                    sum += (int32_t)(generateSampleForChannel(channels[i]) * FINAL_SCALE);
                }
            }

            // Apply Master Volume
            if (masterVolume != 1.0f) {
                sum = (int32_t)(sum * masterVolume);
            }

            // Map sum [-131072, 131071] to LUT index [0, 1024]
            // index = (sum + 131072) / 256
            int32_t index = (sum + 131072) >> 8;
            if (index < 0) index = 0;
            if (index > 1024) index = 1024;
            
            int16_t finalSample = audio_mixer_lut[index];
            stream[j] = finalSample;

            // Track peak for diagnostics
            float absSample = (float)std::abs((int32_t)finalSample);
            if (absSample > currentPeak) currentPeak = absSample;
        }
#endif

        // Diagnostic logging (Phase 2)
        static uint32_t lastLog = 0;
        uint32_t now = xTaskGetTickCount() * portTICK_PERIOD_MS;
        if (now - lastLog > 1000) {
            #ifdef PIXELROOT32_ENABLE_PROFILING
            if (currentPeak > 32767.0f) {
                Serial.printf("[AUDIO] PEAK DETECTED: %.0f (CLIPPING!)\n", currentPeak);
            } else {
                Serial.printf("[AUDIO] Peak: %.0f (%.1f%%)\n", currentPeak, (currentPeak / 32767.0f) * 100.0f);
            }
            #endif
            currentPeak = 0.0f; // Reset peak after logging
            lastLog = now;
        }
        
        audioTimeSamples += length;
    }

    void ESP32AudioScheduler::processCommands() {
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

    void ESP32AudioScheduler::updateMusicSequencer(int /*length*/) {
        if (!musicPlaying || musicPaused || !currentTrack) return;

        while (musicPlaying && currentTrack && audioTimeSamples >= nextNoteSample) {
            playCurrentNote();

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
        }
    }

    void ESP32AudioScheduler::playCurrentNote() {
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

    void ESP32AudioScheduler::executePlayEvent(const AudioEvent& event) {
        AudioChannel* ch = findFreeChannel(event.type);
        if (ch) {
            ch->enabled = true;
            ch->frequency = event.frequency;
            ch->phase = 0.0f;
            ch->phaseIncrement = event.frequency / (float)sampleRate;
            ch->volume = event.volume;
            ch->targetVolume = event.volume;
            ch->volumeDelta = 0.0f;
            ch->remainingSamples = (uint64_t)(event.duration * (float)sampleRate);
            if (event.type == WaveType::PULSE) {
                ch->dutyCycle = event.duty;
            }
        }
    }

    AudioChannel* ESP32AudioScheduler::findFreeChannel(WaveType type) {
        AudioChannel* candidate = nullptr;
        uint64_t minRemaining = 0xFFFFFFFFFFFFFFFFULL;

        for (int i = 0; i < NUM_CHANNELS; i++) {
            if (channels[i].type == type) {
                if (!channels[i].enabled) return &channels[i];
                
                // Voice stealing: find the one that will finish soonest
                if (channels[i].remainingSamples < minRemaining) {
                    minRemaining = channels[i].remainingSamples;
                    candidate = &channels[i];
                }
            }
        }
        return candidate;
    }

    float ESP32AudioScheduler::generateSampleForChannel(AudioChannel& ch) {
        if (!ch.enabled) return 0.0f;

        float sample = 0.0f;
        
        switch (ch.type) {
            case WaveType::PULSE:
                sample = (ch.phase < ch.dutyCycle) ? 1.0f : -1.0f;
                break;
            case WaveType::TRIANGLE:
                // NES-style triangle: rises -1 to 1, falls 1 to -1
                if (ch.phase < 0.5f) {
                    sample = 4.0f * ch.phase - 1.0f;
                } else {
                    sample = 3.0f - 4.0f * ch.phase;
                }
                break;
            case WaveType::NOISE:
                // Update noise register only on phase wrap
                if (ch.phase < ch.phaseIncrement) {
                    ch.noiseRegister = (uint16_t)(rand() & 0xFFFF);
                }
                sample = (ch.noiseRegister & 1) ? 1.0f : -1.0f;
                break;
        }

        ch.phase += ch.phaseIncrement;
        if (ch.phase >= 1.0f) ch.phase -= 1.0f;

        if (ch.volumeDelta != 0.0f) {
            ch.volume += ch.volumeDelta;
            if ((ch.volumeDelta > 0.0f && ch.volume >= ch.targetVolume) ||
                (ch.volumeDelta < 0.0f && ch.volume <= ch.targetVolume)) {
                ch.volume = ch.targetVolume;
                ch.volumeDelta = 0.0f;
            }
        }

        if (ch.remainingSamples > 0) {
            ch.remainingSamples--;
            if (ch.remainingSamples == 0) {
                ch.enabled = false;
            }
        }

        // Return normalized sample multiplied by channel volume
        return sample * ch.volume;
    }

} // namespace pixelroot32::audio

#endif // ESP32
