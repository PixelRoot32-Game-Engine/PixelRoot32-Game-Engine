/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#include "audio/ApuCore.h"
#include "audio/AudioMixerLUT.h"
#include "platforms/EngineConfig.h"
#include "core/Log.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <cstdlib>

#ifdef ESP32
#include <Arduino.h>
#include <soc/soc_caps.h>
#endif

namespace pixelroot32::audio {

    namespace platforms = pixelroot32::platforms;
    namespace logging = pixelroot32::core::logging;
    using logging::LogLevel;
    using logging::log;

    // ------------------------------------------------------------------
    // Construction / lifecycle
    // ------------------------------------------------------------------
    ApuCore::ApuCore() {
        channels[0].type = WaveType::PULSE;
        channels[1].type = WaveType::PULSE;
        channels[2].type = WaveType::TRIANGLE;
        channels[3].type = WaveType::NOISE;

        for (int i = 0; i < NUM_CHANNELS; ++i) {
            channels[i].reset();
        }
    }

    void ApuCore::init(int sr) {
        sampleRate = sr;
        tickDurationSamples =
            (uint64_t)((float)sampleRate * 60.0f / (tempoBPM * (float)TICKS_PER_BEAT));
    }

    void ApuCore::reset() {
        for (int i = 0; i < NUM_CHANNELS; ++i) {
            channels[i].reset();
        }
        masterVolume = 1.0f;
        masterVolumeScale = 65536;
        hpfPrevIn = hpfPrevOut = 0.0f;
        currentPeak = 0.0f;
        samplesSinceLog = 0;
        audioTimeSamples = 0;
        for (size_t i = 0; i < MAX_MUSIC_TRACKS; ++i) {
            tracks[i] = nullptr;
            currentNoteIndices[i] = 0;
            nextNoteTicks[i] = 0;
        }
        globalTickCounter = 0;
        activeTrackCount = 0;
        tempoFactor = 1.0f;
        sequencerNoteLimit.store(MAX_NOTES_PER_FRAME, std::memory_order_release);
        deferredNotes.store(0, std::memory_order_release);
        musicPlayingFlag.store(false, std::memory_order_release);
        musicPausedFlag.store(false, std::memory_order_release);
        droppedCommands.store(0, std::memory_order_release);
    }

    void ApuCore::setSequencerNoteLimit(size_t limit) {
        if (limit == 0) {
            // Zero means unbounded (fallback to effectively unlimited)
            limit = MAX_NOTES_PER_FRAME * 32; // ~1024, effectively unbounded
        } else if (limit > 1000) {
            limit = 32; // Clamp per spec: > 1000 → 32
        }
        sequencerNoteLimit.store(limit, std::memory_order_release);
    }

    bool ApuCore::submitCommand(const AudioCommand& cmd) {
        const bool ok = commandQueue.enqueue(cmd);
        if (!ok) {
            droppedCommands.fetch_add(1u, std::memory_order_relaxed);
#if defined(PIXELROOT32_DEBUG_MODE)
            static uint32_t lastLogged = 0;
            const uint32_t total = droppedCommands.load(std::memory_order_relaxed);
            if (total - lastLogged >= 16u) {
                lastLogged = total;
                log(LogLevel::Warning, "[AUDIO] command queue full; dropped %u total", (unsigned)total);
            }
#endif
        }
        return ok;
    }

    // ------------------------------------------------------------------
    // Command processing
    // ------------------------------------------------------------------
    void ApuCore::processCommands() {
        AudioCommand cmd;
        while (commandQueue.dequeue(cmd)) {
            switch (cmd.type) {
                case AudioCommandType::PLAY_EVENT:
                    executePlayEvent(cmd.event);
                    break;

                case AudioCommandType::SET_MASTER_VOLUME: {
                    float v = cmd.volume;
                    if (v > 1.0f) v = 1.0f;
                    if (v < 0.0f) v = 0.0f;
                    masterVolume = v;
                    masterVolumeScale = (int32_t)(masterVolume * 65536.0f);
                    break;
                }

                case AudioCommandType::STOP_CHANNEL:
                    if (cmd.channelIndex < NUM_CHANNELS) {
                        channels[cmd.channelIndex].reset();
                    }
                    break;

                case AudioCommandType::MUSIC_PLAY: {
                    activeTrackCount = 1;
                    tracks[0] = cmd.track;
                    currentNoteIndices[0] = 0;
                    nextNoteTicks[0] = 0;

                    tickDurationSamples =
                        (uint64_t)((float)sampleRate * 60.0f / (tempoBPM * (float)TICKS_PER_BEAT));
                    globalTickCounter = 0;

                    for (size_t i = 0;
                         i < cmd.subTrackCount && activeTrackCount < MAX_MUSIC_TRACKS;
                         ++i) {
                        if (cmd.subTracks[i]) {
                            tracks[activeTrackCount] = cmd.subTracks[i];
                            currentNoteIndices[activeTrackCount] = 0;
                            nextNoteTicks[activeTrackCount] = 0;
                            activeTrackCount++;
                        }
                    }

                    musicPlayingFlag.store(true, std::memory_order_release);
                    musicPausedFlag.store(false, std::memory_order_release);
                    break;
                }

                case AudioCommandType::MUSIC_STOP:
                    for (size_t i = 0; i < MAX_MUSIC_TRACKS; ++i) {
                        tracks[i] = nullptr;
                        currentNoteIndices[i] = 0;
                        nextNoteTicks[i] = 0;
                    }
                    activeTrackCount = 0;
                    musicPlayingFlag.store(false, std::memory_order_release);
                    break;

                case AudioCommandType::MUSIC_PAUSE:
                    musicPausedFlag.store(true, std::memory_order_release);
                    break;

                case AudioCommandType::MUSIC_RESUME:
                    musicPausedFlag.store(false, std::memory_order_release);
                    break;

                case AudioCommandType::MUSIC_SET_TEMPO:
                    tempoFactor = std::max(0.1f, cmd.tempoFactor);
                    break;

                case AudioCommandType::MUSIC_SET_BPM:
                    tempoBPM = std::max(30.0f, std::min(300.0f, cmd.bpm));
                    tickDurationSamples =
                        (uint64_t)((float)sampleRate * 60.0f / (tempoBPM * (float)TICKS_PER_BEAT));
                    break;

                default:
                    break;
            }
        }
    }

    // ------------------------------------------------------------------
    // Music sequencer (NES-style tick sync)
    // ------------------------------------------------------------------
    void ApuCore::updateMusicSequencer() {
        if (!musicPlayingFlag.load(std::memory_order_acquire)
            || musicPausedFlag.load(std::memory_order_acquire)
            || activeTrackCount == 0
            || tickDurationSamples == 0) {
            return;
        }

        uint64_t currentTick = audioTimeSamples / tickDurationSamples;
        if (currentTick <= globalTickCounter && globalTickCounter > 0) return;
        globalTickCounter = currentTick;

        const size_t limit = sequencerNoteLimit.load(std::memory_order_acquire);
        size_t notesProcessedThisFrame = 0;
        size_t frameDeferredNotes = 0;

        for (size_t trackIdx = 0; trackIdx < activeTrackCount; ++trackIdx) {
            const MusicTrack* track = tracks[trackIdx];
            if (!track) continue;

            size_t& noteIdx = currentNoteIndices[trackIdx];
            uint64_t& nextTick = nextNoteTicks[trackIdx];

            while (musicPlayingFlag.load(std::memory_order_acquire)
                   && globalTickCounter >= nextTick) {
                // Check note limit per frame - bounded processing
                if (notesProcessedThisFrame >= limit) {
                    frameDeferredNotes++;
                    // Still need to advance timing even if we skip the note
                    const MusicNote& note = track->notes[noteIdx];
                    uint64_t noteTicks = (uint64_t)(note.duration * (float)TICKS_PER_BEAT / tempoFactor);
                    if (noteTicks == 0) noteTicks = 1;
                    nextTick += noteTicks;
                    noteIdx++;
                    if (noteIdx >= track->count) {
                        if (track->loop) {
                            noteIdx = 0;
                        } else {
                            tracks[trackIdx] = nullptr;
                            break;
                        }
                    }
                    continue;
                }
                const MusicNote& note = track->notes[noteIdx];

                // On NOISE channels, Rest notes are treated as hits so
                // percussion tracks can be authored as a pattern of hits.
                bool shouldPlay = (note.note != Note::Rest)
                               || (track->channelType == WaveType::NOISE);

                if (shouldPlay) {
                    AudioEvent event{};
                    event.type = track->channelType;

                    const InstrumentPreset* percPreset = nullptr;
                    if (note.preset && note.preset->duty == 0.0f) {
                        percPreset = note.preset;
                    }

                    if (percPreset) {
                        event.frequency = instrumentToFrequency(*percPreset, note.note, note.octave);
                        event.duration = (percPreset->defaultDuration > 0.0f)
                            ? percPreset->defaultDuration / tempoFactor
                            : note.duration / tempoFactor;
                        event.noisePeriod = percPreset->noisePeriod;
                    } else {
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

                uint64_t noteTicks = (uint64_t)(note.duration * (float)TICKS_PER_BEAT / tempoFactor);
                if (noteTicks == 0) noteTicks = 1;
                nextTick += noteTicks;

                noteIdx++;
                if (noteIdx >= track->count) {
                    if (track->loop) {
                        noteIdx = 0;
                    } else {
                        tracks[trackIdx] = nullptr;
                        break;
                    }
                }
                notesProcessedThisFrame++;
            }
        }

// Update deferred notes counter for diagnostics
        if (frameDeferredNotes > 0) {
            deferredNotes.store(frameDeferredNotes, std::memory_order_relaxed);
#if defined(PIXELROOT32_DEBUG_MODE)
            log(LogLevel::Info, "[APU] deferred %u notes (limit=%u)", (unsigned)frameDeferredNotes, (unsigned)limit);
#endif
        } else {
            deferredNotes.store(0, std::memory_order_relaxed);
        }

        // If every track has finished and none loops, stop music so that
        // MusicPlayer::isPlaying() reports the real state.
        bool anyActive = false;
        for (size_t trackIdx = 0; trackIdx < activeTrackCount; ++trackIdx) {
            const MusicTrack* track = tracks[trackIdx];
            if (track && (track->loop || currentNoteIndices[trackIdx] < track->count)) {
                anyActive = true;
                break;
            }
        }
        if (!anyActive) {
            musicPlayingFlag.store(false, std::memory_order_release);
        }
    }

    // ------------------------------------------------------------------
    // Play event (retrigger a voice)
    // ------------------------------------------------------------------
    void ApuCore::executePlayEvent(const AudioEvent& event) {
        AudioChannel* ch = findFreeChannel(event.type);
        if (!ch) return;

        ch->enabled = true;
        ch->frequency = event.frequency;
        ch->phase = 0.0f;
        ch->phaseIncrement = event.frequency / (float)sampleRate;

        // Fixed-point mirror so the no-FPU mixing path (ESP32-C3) never
        // touches float inside the per-sample inner loop. Computed once
        // per retrigger in float, which is fine since executePlayEvent is
        // rare compared to the 22 050 Hz sample loop.
        ch->phaseQ32 = 0u;
        if (sampleRate > 0) {
            const double inc = (double)event.frequency * 4294967296.0 / (double)sampleRate;
            ch->phaseIncQ32 = (inc < 0.0) ? 0u
                            : (inc >= 4294967295.0) ? 0xFFFFFFFFu
                            : (uint32_t)inc;
        } else {
            ch->phaseIncQ32 = 0u;
        }

        // Anti-click: instead of snapping volume to event.volume (which
        // produces a discontinuity when voice-stealing or retriggering),
        // ramp from 0 to the target over ~2 ms. Attack is short enough to
        // feel instantaneous while killing pops.
        const float attackSamples = std::max(1.0f, (float)sampleRate * 0.002f);
        ch->volume = 0.0f;
        ch->targetVolume = event.volume;
        ch->volumeDelta = event.volume / attackSamples;

        ch->remainingSamples = (uint64_t)(event.duration * (float)sampleRate);

        if (event.type == WaveType::PULSE) {
            ch->dutyCycle = event.duty;
            double d = (double)event.duty;
            if (d < 0.0) d = 0.0;
            if (d > 1.0) d = 1.0;
            ch->dutyCycleQ32 = (uint32_t)(d * 4294967296.0);
        } else if (event.type == WaveType::NOISE) {
            uint32_t period;
            if (event.noisePeriod > 0) {
                period = event.noisePeriod;
            } else {
                float noiseHz = event.frequency;
                if (noiseHz < 1.0f) noiseHz = 1000.0f;
                period = (uint32_t)((float)sampleRate / noiseHz);
                if (period < 1u) period = 1u;
            }
            ch->noisePeriodSamples = period;
            ch->noiseCountdown = 1u;
            ch->lfsrState = 0x4000;
            ch->phase = 0.0f;
            ch->phaseIncrement = 0.0f;
        }
    }

    AudioChannel* ApuCore::findFreeChannel(WaveType type) {
        AudioChannel* candidate = nullptr;
        uint64_t minRemaining = UINT64_MAX;
        for (int i = 0; i < NUM_CHANNELS; ++i) {
            if (channels[i].type != type) continue;
            if (!channels[i].enabled) return &channels[i];
            if (channels[i].remainingSamples < minRemaining) {
                minRemaining = channels[i].remainingSamples;
                candidate = &channels[i];
            }
        }
        return candidate;
    }

    // ------------------------------------------------------------------
    // Per-channel sample generation
    // ------------------------------------------------------------------
    float ApuCore::generateSampleForChannel(AudioChannel& ch) {
        if (!ch.enabled) return 0.0f;

        float sample = 0.0f;
        switch (ch.type) {
            case WaveType::PULSE:
                sample = (ch.phase < ch.dutyCycle) ? 1.0f : -1.0f;
                break;
            case WaveType::TRIANGLE:
                sample = (ch.phase < 0.5f)
                       ? (4.0f * ch.phase - 1.0f)
                       : (3.0f - 4.0f * ch.phase);
                break;
            case WaveType::NOISE: {
                // Unified 15-bit NES LFSR shared by all platforms so the
                // timbre is identical in simulator and hardware.
                if (ch.noiseCountdown > 0u) ch.noiseCountdown--;
                if (ch.noiseCountdown == 0u) {
                    const uint16_t fb = (uint16_t)(((ch.lfsrState & 1u) ^ ((ch.lfsrState >> 1) & 1u)) & 1u);
                    ch.lfsrState = (uint16_t)((ch.lfsrState >> 1) | (fb << 14));
                    ch.noiseCountdown = ch.noisePeriodSamples;
                }
                sample = (ch.lfsrState & 1u) ? 1.0f : -1.0f;
                break;
            }
        }

        if (ch.type != WaveType::NOISE) {
            ch.phase += ch.phaseIncrement;
            if (ch.phase >= 1.0f) ch.phase -= 1.0f;
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
            if (ch.remainingSamples == 0) ch.enabled = false;
        }

        return sample * ch.volume;
    }

    // ------------------------------------------------------------------
    // Main mixing path
    // ------------------------------------------------------------------
    void ApuCore::generateSamples(int16_t* stream, int length) {
        if (!stream || length <= 0) return;

// #if defined(ESP32) && defined(PIXELROOT32_ENABLE_PROFILING)
//         const uint32_t startTime = micros();
// #endif

        if (!commandQueue.isEmpty()) processCommands();
        updateMusicSequencer();

        constexpr float FINAL_SCALE = 32767.0f;
        // HPF coefficient: y[n] = x[n] - x[n-1] + R*y[n-1]
        // R = 0.995 at 22050 Hz -> ~35 Hz -3dB
        constexpr float HPF_R = 0.995f;

#if defined(SOC_CPU_HAS_FPU) && SOC_CPU_HAS_FPU
        // ---- FPU path (ESP32 classic, ESP32-S3, native) -----------------
        for (int i = 0; i < length; ++i) {
            float acc = 0.0f;
            for (int c = 0; c < NUM_CHANNELS; ++c) {
                if (channels[c].enabled) {
                    acc += generateSampleForChannel(channels[c]) * MIXER_SCALE;
                }
            }
            acc *= masterVolume;
            float mixed = acc / (1.0f + std::fabs(acc) * MIXER_K);

            // DC-blocker: removes duty-asymmetry DC and softens retrigger pops.
            const float hpfOut = mixed - hpfPrevIn + HPF_R * hpfPrevOut;
            hpfPrevIn = mixed;
            hpfPrevOut = hpfOut;

            float finalSample = hpfOut * FINAL_SCALE;
            const float absSample = std::fabs(finalSample);
            if (absSample > currentPeak) currentPeak = absSample;
            if (finalSample > 32767.0f) finalSample = 32767.0f;
            if (finalSample < -32768.0f) finalSample = -32768.0f;
            stream[i] = (int16_t)finalSample;
        }
#elif defined(ESP32)
        // ---- Integer / LUT path (ESP32-C3, RISC-V no-FPU) ---------------
        // Uses the Q32 phase mirror + Q15 output per channel so the inner
        // loop contains zero soft-float operations. The LUT is pre-fitted
        // to the same compression curve as the FPU path, so both paths
        // produce numerically equivalent output.
        //
        // OPTIMIZATION: Hoisted switch from inner loop. Instead of switch(ch.type) per
        // sample (causes branch misprediction at 22kHz), we process by wave type
        // using separate accumulators. This eliminates ~4 branches per sample.
        //
        // Volume is still stored in float; we convert to Q15 once per
        // block (not per sample) so volume ramps still work but the 22kHz
        // inner loop stays integer-only.
        int32_t volQ15[NUM_CHANNELS];
        int32_t volDeltaQ15[NUM_CHANNELS];
        int32_t volTargetQ15[NUM_CHANNELS];
        for (int c = 0; c < NUM_CHANNELS; ++c) {
            volQ15[c]       = (int32_t)(channels[c].volume       * 32768.0f);
            volDeltaQ15[c]  = (int32_t)(channels[c].volumeDelta  * 32768.0f);
            volTargetQ15[c] = (int32_t)(channels[c].targetVolume * 32768.0f);
        }

        // Hoisted wave type dispatch - compute sample once before inner loop
        // using inline branch-free operations instead of switch
        auto generatePulseSampleQ15 = [](AudioChannel& ch) -> int32_t {
            return (ch.phaseQ32 < ch.dutyCycleQ32) ? 32767 : -32767;
        };
        auto generateTriangleSampleQ15 = [](AudioChannel& ch) -> int32_t {
            const uint32_t p16 = ch.phaseQ32 >> 16;
            return (p16 < 32768u) ? ((int32_t)(p16 * 2) - 32768) : (32768 - (int32_t)((p16 - 32768u) * 2));
        };
        auto generateNoiseSampleQ15 = [](AudioChannel& ch) -> int32_t {
            return (ch.lfsrState & 1u) ? 32767 : -32767;
        };

        // Process channels by type - hoisted from inner loop
        // This eliminates the switch(ch.type) branch misprediction
        for (int i = 0; i < length; ++i) {
            int32_t sum = 0;

            for (int c = 0; c < NUM_CHANNELS; ++c) {
                AudioChannel& ch = channels[c];
                if (!ch.enabled) continue;

                int32_t s = 0; // Q15 sample in [-32767, +32767]

                // Hoisted wave generation - branch-free type dispatch
                // Using if-else chain once per frame (compile-time constant channel types)
                // instead of switch per sample (branch misprediction)
                if (ch.type == WaveType::PULSE) {
                    s = generatePulseSampleQ15(ch);
                } else if (ch.type == WaveType::TRIANGLE) {
                    s = generateTriangleSampleQ15(ch);
                } else if (ch.type == WaveType::NOISE) {
                    // Noise requires state update - do it inline
                    if (ch.noiseCountdown > 0u) ch.noiseCountdown--;
                    if (ch.noiseCountdown == 0u) {
                        const uint16_t fb = (uint16_t)(((ch.lfsrState & 1u) ^ ((ch.lfsrState >> 1) & 1u)) & 1u);
                        ch.lfsrState = (uint16_t)((ch.lfsrState >> 1) | (fb << 14));
                        ch.noiseCountdown = ch.noisePeriodSamples;
                    }
                    s = generateNoiseSampleQ15(ch);
                }

                // Apply per-channel Q15 volume: (s * volQ15) >> 15 keeps
                // the result in Q15 centred around 0.
                int32_t sv = (s * volQ15[c]) >> 15;

                // MIXER_SCALE = 0.4 per channel. 0.4 ≈ 13107/32768 so we
                // scale by 13107 and shift 15 to land back in Q15. This
                // matches the FPU branch exactly (MIXER_SCALE * channel).
                sv = (sv * 13107) >> 15;
                sum += sv;

                // Phase accumulator (integer, wraps automatically).
                if (ch.type != WaveType::NOISE) {
                    ch.phaseQ32 += ch.phaseIncQ32;
                }

                // Volume ramp — integer.
                if (volDeltaQ15[c] != 0) {
                    volQ15[c] += volDeltaQ15[c];
                    if ((volDeltaQ15[c] > 0 && volQ15[c] >= volTargetQ15[c]) ||
                        (volDeltaQ15[c] < 0 && volQ15[c] <= volTargetQ15[c])) {
                        volQ15[c] = volTargetQ15[c];
                        volDeltaQ15[c] = 0;
                    }
                }

                // Duration countdown (same bookkeeping as float path).
                if (ch.remainingSamples > 0) {
                    ch.remainingSamples--;
                    if (ch.remainingSamples == 0) ch.enabled = false;
                }
            }

            if (masterVolumeScale != 65536) {
                sum = (sum * masterVolumeScale) >> 16;
            }

            int32_t index = (sum + 131072) >> 8;
            if (index < 0) index = 0;
            if (index > 1024) index = 1024;

            // NOTE: HPF intentionally omitted on the no-FPU path — running
            // a float recursive filter per sample on a soft-float core
            // would wipe out the gains of the integer oscillator.
            int16_t finalSample = audio_mixer_lut[index];
            stream[i] = finalSample;

            const int32_t absSample = (finalSample < 0) ? -(int32_t)finalSample : (int32_t)finalSample;
            if ((float)absSample > currentPeak) currentPeak = (float)absSample;
        }

        // Sync per-channel float volume back from Q15 so the next block
        // (and the FPU fallback if ever invoked) sees up-to-date state.
        for (int c = 0; c < NUM_CHANNELS; ++c) {
            channels[c].volume      = (float)volQ15[c]       / 32768.0f;
            channels[c].volumeDelta = (float)volDeltaQ15[c]  / 32768.0f;
        }
#else
        // ---- Native / fallback path -------------------------------------
        for (int i = 0; i < length; ++i) {
            float acc = 0.0f;
            for (int c = 0; c < NUM_CHANNELS; ++c) {
                if (channels[c].enabled) {
                    acc += generateSampleForChannel(channels[c]) * MIXER_SCALE;
                }
            }
            acc *= masterVolume;
            float mixed = acc / (1.0f + std::fabs(acc) * MIXER_K);

            const float hpfOut = mixed - hpfPrevIn + HPF_R * hpfPrevOut;
            hpfPrevIn = mixed;
            hpfPrevOut = hpfOut;

            float finalSample = hpfOut * FINAL_SCALE;
            const float absSample = std::fabs(finalSample);
            if (absSample > currentPeak) currentPeak = absSample;
            if (finalSample > 32767.0f) finalSample = 32767.0f;
            if (finalSample < -32768.0f) finalSample = -32768.0f;
            stream[i] = (int16_t)finalSample;
        }
#endif

        audioTimeSamples += (uint64_t)length;
        samplesSinceLog += (uint64_t)length;

        if constexpr (platforms::config::EnableProfiling) {
            if (samplesSinceLog >= (uint64_t)sampleRate) {
                uint8_t idx = profileWriteIdx.fetch_add(1, std::memory_order_relaxed);
                idx %= PROFILE_RING_SIZE;
                profileRing[idx].audioTimeSamples = audioTimeSamples;
                profileRing[idx].peak = currentPeak;
                profileRing[idx].clipped = (currentPeak >= 32767.0f);
                if (profileCount < PROFILE_RING_SIZE) {
                    profileCount++;
                }
                currentPeak = 0.0f;
                samplesSinceLog = 0;
            }
        } else if (samplesSinceLog >= (uint64_t)sampleRate) {
            currentPeak = 0.0f;
            samplesSinceLog = 0;
        }

// #if defined(ESP32) && defined(PIXELROOT32_ENABLE_PROFILING)
//         (void)startTime;
// #endif
    }

void ApuCore::getAndResetProfileStats(ProfileEntry* out, uint8_t& count) {
        count = profileCount;
        uint8_t startIdx = profileCount > 0 ? (profileWriteIdx.load(std::memory_order_relaxed) - profileCount + PROFILE_RING_SIZE) % PROFILE_RING_SIZE : 0;
        for (uint8_t i = 0; i < count; ++i) {
            out[i] = profileRing[(startIdx + i) % PROFILE_RING_SIZE];
        }
        profileCount = 0;
    }

} // namespace pixelroot32::audio
