/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#ifdef ESP32

#include "drivers/esp32/ESP32AudioScheduler.h"
#include "audio/AudioMixerLUT.h"
#include "platforms/EngineConfig.h"
#include "core/Log.h"

#include <Arduino.h>
#include <algorithm>
#include <cstring>
#include <soc/soc_caps.h>

namespace pixelroot32::audio {

    namespace platforms = pixelroot32::platforms;
    namespace logging = pixelroot32::core::logging;
    
    using logging::LogLevel;
    using logging::log;

    static inline void stepNoiseLfsr15(uint16_t& state) {
        const uint16_t feedback = (uint16_t)(((state & 1u) ^ ((state >> 1) & 1u)) & 1u);
        state = (uint16_t)((state >> 1) | (feedback << 14));
    }

    ESP32AudioScheduler::ESP32AudioScheduler(int /*reservedCoreId*/, int /*reservedTaskPriority*/) {
    }

    ESP32AudioScheduler::~ESP32AudioScheduler() {
        stop();
    }

    void ESP32AudioScheduler::init(AudioBackend* backend, int sampleRate, const platforms::PlatformCapabilities& /*caps*/) {
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
        if (!commandQueue.enqueue(cmd)) {
            droppedCommands.fetch_add(1u, std::memory_order_relaxed);
#if defined(PIXELROOT32_DEBUG_MODE)
            static uint32_t lastDropLogCount = 0;
            const uint32_t total = droppedCommands.load(std::memory_order_relaxed);
            if (total - lastDropLogCount >= 16u) {
                lastDropLogCount = total;
                log(LogLevel::Warning, "[AUDIO] command queue full; dropped %u total", (unsigned)total);
            }
#endif
        }
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

        // Performance timing (when profiling enabled)
        uint32_t startTime = 0;
        if constexpr (platforms::config::EnableProfiling) {
            startTime = micros();
        }

        // Only process commands if queue is not empty to avoid atomic overhead
        if (!commandQueue.isEmpty()) {
            processCommands();
        }
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

            // Apply Master Volume using pre-computed Q16 fixed-point scale
            if (masterVolumeScale != 65536) {
                sum = (sum * masterVolumeScale) >> 16;
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
            if constexpr (platforms::config::EnableProfiling) {
                if (currentPeak > 32767.0f) {
                    log(LogLevel::Profiling, "[AUDIO] PEAK DETECTED: %.0f (CLIPPING!)", currentPeak);
                } else {
                    log(LogLevel::Profiling, "[AUDIO] Peak: %.0f (%.1f%%)", currentPeak, (currentPeak / 32767.0f) * 100.0f);
                }
            }
            currentPeak = 0.0f; // Reset peak after logging
            lastLog = now;
        }
        
        audioTimeSamples += length;

        // Performance timing (when profiling enabled)
        if constexpr (platforms::config::EnableProfiling) {
            uint32_t elapsed = micros() - startTime;
            totalGenerateTimeUs += elapsed;
            generateSampleCount++;
            if (elapsed > maxGenerateTimeUs) {
                maxGenerateTimeUs = elapsed;
            }
        }
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
                    // Pre-compute for LUT path (Q16 fixed-point)
                    masterVolumeScale = (int32_t)(masterVolume * 65536.0f);
                    break;
                case AudioCommandType::STOP_CHANNEL:
                    if (cmd.channelIndex < NUM_CHANNELS) {
                        channels[cmd.channelIndex].reset();
                    }
                    break;
                case AudioCommandType::MUSIC_PLAY:
                    {
                        // Initialize multi-track playback with NES-style tick sync
                        activeTrackCount = 1;
                        tracks[0] = cmd.track;
                        currentNoteIndices[0] = 0;
                        
                        // Initialize tick-based timing (NES-style)
                        tickDurationSamples = (uint64_t)((float)sampleRate * 60.0f / (tempoBPM * (float)TICKS_PER_BEAT));
                        globalTickCounter = 0;
                        nextNoteSamples[0] = 0;
                        
                        // Add sub-tracks - all synchronized to global ticks
                        for (size_t i = 0; i < cmd.subTrackCount && activeTrackCount < MAX_MUSIC_TRACKS; ++i) {
                            if (cmd.subTracks[i]) {
                                tracks[activeTrackCount] = cmd.subTracks[i];
                                currentNoteIndices[activeTrackCount] = 0;
                                nextNoteSamples[activeTrackCount] = 0;
                                activeTrackCount++;
                            }
                        }
                        
                        musicPlaying = true;
                        musicPaused = false;
                    }
                    break;
                case AudioCommandType::MUSIC_STOP:
                    // Stop all tracks
                    for (size_t i = 0; i < MAX_MUSIC_TRACKS; ++i) {
                        tracks[i] = nullptr;
                        currentNoteIndices[i] = 0;
                        nextNoteSamples[i] = 0;
                    }
                    activeTrackCount = 0;
                    musicPlaying = false;
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
                case AudioCommandType::MUSIC_SET_BPM:
                    tempoBPM = std::max(30.0f, std::min(300.0f, cmd.bpm));
                    tickDurationSamples = (uint64_t)((float)sampleRate * 60.0f / (tempoBPM * (float)TICKS_PER_BEAT));
                    break;
                default:
                    break;
            }
        }
    }

    void ESP32AudioScheduler::updateMusicSequencer(int /*length*/) {
        if (!musicPlaying || musicPaused || activeTrackCount == 0) return;

        // NES-style: Advance global tick counter based on elapsed samples
        uint64_t currentTick = audioTimeSamples / tickDurationSamples;
        
        // Only process if we've moved to a new tick
        if (currentTick <= globalTickCounter && globalTickCounter > 0) {
            return;
        }
        
        // Update global tick counter
        globalTickCounter = currentTick;

        // Process notes for all active tracks synchronously on each tick
        for (size_t trackIdx = 0; trackIdx < activeTrackCount; ++trackIdx) {
            const MusicTrack* track = tracks[trackIdx];
            if (!track) continue;

            size_t& noteIdx = currentNoteIndices[trackIdx];
            uint64_t& nextTick = nextNoteSamples[trackIdx];

            // Check if it's time to play the next note for this track
            while (musicPlaying && globalTickCounter >= nextTick) {
                const MusicNote& note = track->notes[noteIdx];
                
                // Play the note (skip rests for melodic tracks, but NOT for noise/percussion)
                // Percussion tracks use Note::Rest to indicate when to play the drum sound
                bool shouldPlay = (note.note != Note::Rest);
                
                // For noise channel (percussion), always play - even for Rest notes
                if (!shouldPlay && track->channelType == WaveType::NOISE) {
                    shouldPlay = true;
                }
                
                if (shouldPlay) {
                    AudioEvent event;
                    event.type = track->channelType;
                    
                    // Use note.preset for percussion (duty==0 means NOISE/percussion)
                    const InstrumentPreset* percPreset = nullptr;
                    if (note.preset && note.preset->duty == 0.0f) {
                        percPreset = note.preset;
                    }
                    
                    if (percPreset) {
                        // Percussion: use instrumentToFrequency from preset
                        event.frequency = instrumentToFrequency(*percPreset, note.note, note.octave);
                        // Use defaultDuration if defined (>0), otherwise use note.duration
                        event.duration = (percPreset->defaultDuration > 0.0f) 
                            ? percPreset->defaultDuration / tempoFactor
                            : note.duration / tempoFactor;
                        event.noisePeriod = percPreset->noisePeriod;
                    } else {
                        // Melodic: legacy behavior
                        // For percussion (Note::Rest on NOISE channel), use default frequency
                        if (note.note == Note::Rest && event.type == WaveType::NOISE) {
                            event.frequency = 1000.0f;
                        } else {
                            event.frequency = noteToFrequency(note.note, note.octave);
                        }
                        event.duration = note.duration / tempoFactor;
                        event.noisePeriod = 0;
                    }
                    
                    event.volume = note.volume;
                    event.duty = (event.type == WaveType::PULSE) ? track->duty : 0.5f;
                    executePlayEvent(event);
                }

                // Calculate note duration in ticks
                uint64_t noteTicks = (uint64_t)(note.duration * (float)TICKS_PER_BEAT / tempoFactor);
                if (noteTicks == 0) noteTicks = 1;
                
                nextTick += noteTicks;

                // Advance to next note
                noteIdx++;
                if (noteIdx >= track->count) {
                    if (track->loop) {
                        noteIdx = 0;
                    } else {
                        track = nullptr;
                        break;
                    }
                }
            }
        }

        // Check if all non-looping tracks have finished
        bool allFinished = true;
        for (size_t trackIdx = 0; trackIdx < activeTrackCount; ++trackIdx) {
            const MusicTrack* track = tracks[trackIdx];
            if (track && (currentNoteIndices[trackIdx] < track->count || track->loop)) {
                allFinished = false;
                break;
            }
        }
        
        if (allFinished && activeTrackCount > 0) {
            bool anyLooping = false;
            for (size_t trackIdx = 0; trackIdx < activeTrackCount; ++trackIdx) {
                const MusicTrack* track = tracks[trackIdx];
                if (track && track->loop) {
                    anyLooping = true;
                    break;
                }
            }
            if (!anyLooping) {
                musicPlaying = false;
            }
        }
    }

    void ESP32AudioScheduler::playCurrentNote() {
        // Multi-track: notes are played in updateMusicSequencer directly
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
            } else if (event.type == WaveType::NOISE) {
                // If noisePeriod is set (>0), use it directly, otherwise calc from frequency
                uint32_t period;
                if (event.noisePeriod > 0) {
                    period = event.noisePeriod;
                } else {
                    float noiseHz = event.frequency;
                    if (noiseHz < 1.0f) {
                        noiseHz = 1.0f;
                    }
                    period = (uint32_t)((float)sampleRate / noiseHz);
                    if (period < 1u) {
                        period = 1u;
                    }
                }
                ch->noisePeriodSamples = period;
                ch->noiseCountdown = 1u;
                ch->lfsrState = 0x4000;
                ch->phase = 0.0f;
                ch->phaseIncrement = 0.0f;
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
            case WaveType::NOISE: {
                if (ch.noiseCountdown > 0u) {
                    ch.noiseCountdown--;
                }
                if (ch.noiseCountdown == 0u) {
                    stepNoiseLfsr15(ch.lfsrState);
                    ch.noiseCountdown = ch.noisePeriodSamples;
                }
                sample = (ch.lfsrState & 1u) ? 1.0f : -1.0f;
                break;
            }
        }

        if (ch.type != WaveType::NOISE) {
            ch.phase += ch.phaseIncrement;
            if (ch.phase >= 1.0f) {
                ch.phase -= 1.0f;
            }
        }

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
