/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */

#include "audio/DefaultAudioScheduler.h"
#include "audio/AudioMixerLUT.h"
#include "platforms/EngineConfig.h"
#include "core/Log.h"

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
    
    namespace platforms = pixelroot32::platforms;
    namespace logging = pixelroot32::core::logging;
    using logging::LogLevel;
    using logging::log;

    DefaultAudioScheduler::DefaultAudioScheduler() {
        channels[0].type = WaveType::PULSE;
        channels[1].type = WaveType::PULSE;
        channels[2].type = WaveType::TRIANGLE;
        channels[3].type = WaveType::NOISE;

        for (int i = 0; i < NUM_CHANNELS; ++i) {
            channels[i].reset();
        }
    }

    void DefaultAudioScheduler::init(AudioBackend* /*backend*/, int sampleRate, const platforms::PlatformCapabilities& /*caps*/) {
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

        // Performance timing (when profiling enabled)
        uint32_t startTime = 0;
        if constexpr (platforms::config::EnableProfiling) {
            startTime = micros();
        }

        // Only process commands if queue is not empty to avoid atomic overhead
        if (!commandQueue.isEmpty()) {
            processCommands();
        }

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

            // Apply master volume using pre-computed Q16 fixed-point scale
            if (masterVolumeScale != 65536) {
                sum = (sum * masterVolumeScale) >> 16;
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
            if constexpr (platforms::config::EnableProfiling) {
                if (currentPeak > 32767.0f) {
                    log(LogLevel::Profiling, "[AUDIO] PEAK DETECTED: %.0f (CLIPPING!)", currentPeak);
                } else {
                    log(LogLevel::Profiling, "[AUDIO] Peak: %.0f (%.1f%%)", currentPeak, (currentPeak / 32767.0f) * 100.0f);
                }
            }
            currentPeak = 0.0f;
            totalSamplesProcessed = 0;
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
                        // tickDuration = sampleRate / (BPM * TICKS_PER_BEAT / 60)
                        tickDurationSamples = (uint64_t)((float)sampleRate * 60.0f / (tempoBPM * (float)TICKS_PER_BEAT));
                        globalTickCounter = 0;
                        nextNoteSamples[0] = 0;  // Will be calculated as tick index * tickDuration
                        
                        // Add sub-tracks - all synchronized to global ticks
                        for (size_t i = 0; i < cmd.subTrackCount && activeTrackCount < MAX_MUSIC_TRACKS; ++i) {
                            if (cmd.subTracks[i]) {
                                tracks[activeTrackCount] = cmd.subTracks[i];
                                currentNoteIndices[activeTrackCount] = 0;
                                nextNoteSamples[activeTrackCount] = 0;  // All start at tick 0
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
                    // Recalculate tick duration when BPM changes
                    tickDurationSamples = (uint64_t)((float)sampleRate * 60.0f / (tempoBPM * (float)TICKS_PER_BEAT));
                    break;
                default:
                    break;
            }
        }
    }

    void DefaultAudioScheduler::updateMusicSequencer(int /*length*/) {
        if (!musicPlaying || musicPaused || activeTrackCount == 0) return;

        // NES-style: Advance global tick counter based on elapsed samples
        // Check if we've crossed a tick boundary
        uint64_t currentTick = audioTimeSamples / tickDurationSamples;
        
        // Only process if we've moved to a new tick
        if (currentTick <= globalTickCounter && globalTickCounter > 0) {
            return;  // No new tick yet
        }
        
        // Update global tick counter
        uint64_t ticksAdvanced = currentTick - globalTickCounter;
        globalTickCounter = currentTick;

        // Process notes for all active tracks synchronously on each tick
        for (size_t trackIdx = 0; trackIdx < activeTrackCount; ++trackIdx) {
            const MusicTrack* track = tracks[trackIdx];
            if (!track) continue;

            size_t& noteIdx = currentNoteIndices[trackIdx];
            uint64_t& nextTick = nextNoteSamples[trackIdx];  // Now stores tick number, not sample

            // Check if it's time to play the next note for this track
            while (musicPlaying && globalTickCounter >= nextTick) {
                const MusicNote& note = track->notes[noteIdx];
                
                // Play the note (skip rests for melodic tracks, but NOT for noise/percussion)
                // Percussion tracks use Note::Rest to indicate when to play the drum sound
                // where the instrument preset defines the sound characteristics
                bool shouldPlay = (note.note != Note::Rest);
                
                // For noise channel (percussion), always play - even for Rest notes
                // The frequency will default to 1000Hz if note is Rest
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
                        // Use noisePeriod directly if defined, otherwise calc from frequency
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

                // Calculate note duration in ticks (NES-style: duration is in ticks)
                // 1.0 = one beat (TICKS_PER_BEAT ticks), 0.25 = one quarter beat (1 tick)
                uint64_t noteTicks = (uint64_t)(note.duration * (float)TICKS_PER_BEAT / tempoFactor);
                if (noteTicks == 0) noteTicks = 1;  // Minimum 1 tick
                
                nextTick += noteTicks;

                // Advance to next note
                noteIdx++;
                if (noteIdx >= track->count) {
                    if (track->loop) {
                        noteIdx = 0;
                    } else {
                        // Track finished
                        track = nullptr;
                        break;
                    }
                }
            }
        }

        // Check if all tracks have finished (non-loop tracks)
        bool allFinished = true;
        for (size_t trackIdx = 0; trackIdx < activeTrackCount; ++trackIdx) {
            const MusicTrack* track = tracks[trackIdx];
            if (track && (currentNoteIndices[trackIdx] < track->count || track->loop)) {
                allFinished = false;
                break;
            }
        }
        
        if (allFinished && activeTrackCount > 0) {
            // Check if any track is looping
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

    void DefaultAudioScheduler::playCurrentNote() {
        // Multi-track: notes are played in updateMusicSequencer directly
        // This function kept for backward compatibility but no longer needed
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

            if (event.type == WaveType::PULSE) {
                ch->dutyCycle = event.duty;
            } else if (event.type == WaveType::NOISE) {
                // Initialize noise channel (LFSR, period, countdown)
                // If noisePeriod is set (>0), use it directly, otherwise calc from frequency
                uint32_t period;
                if (event.noisePeriod > 0) {
                    // Direct period for percussion (Kick=25, Snare=50, Hi-HAT=12)
                    period = event.noisePeriod;
                } else {
                    // Legacy: calc from frequency
                    float noiseHz = event.frequency;
                    if (noiseHz <= 0.0f) {
                        noiseHz = 1000.0f;
                    }
                    period = (uint32_t)((float)sampleRate / noiseHz);
                    if (period < 1u) {
                        period = 1u;
                    }
                }
                ch->noisePeriodSamples = period;
                ch->noiseCountdown = 1u;
                ch->lfsrState = 0x4000;  // NES-style 15-bit LFSR initial state
                ch->phaseIncrement = 0.0f;  // Not used for noise
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
                // NES-style 15-bit LFSR with period control via noiseCountdown
                if (ch.noiseCountdown > 0u) {
                    ch.noiseCountdown--;
                }
                if (ch.noiseCountdown == 0u) {
                    uint16_t feedback = ((ch.lfsrState & 1) ^ ((ch.lfsrState >> 1) & 1));
                    ch.lfsrState = (ch.lfsrState >> 1) | (feedback << 14);
                    ch.noiseCountdown = ch.noisePeriodSamples;
                }
                sample = (ch.lfsrState & 1) ? 1.0f : -1.0f;
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
