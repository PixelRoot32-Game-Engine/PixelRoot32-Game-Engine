/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#include "audio/ApuCore.h"
#include "audio/AudioMusicTypes.h"
#include "audio/AudioMixerLUT.h"
#include "audio/AudioOscLUT.h"
#include "platforms/EngineConfig.h"
#include "core/Log.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>

#ifdef ESP32
#include <Arduino.h>
#include <soc/soc_caps.h>
#endif

namespace pixelroot32::audio {

    // ========================================================================================================
    // Wave generator lambdas (moved to file scope for branch-free dispatch)
    // ========================================================================================================
    static auto generatePulseSampleQ15 = [](AudioChannel& ch) -> int32_t {
        return (ch.phaseQ32 < ch.dutyCycleQ32) ? 32767 : -32767;
    };
    static auto generateTriangleSampleQ15 = [](AudioChannel& ch) -> int32_t {
        const uint32_t p16 = ch.phaseQ32 >> 16;
        return (p16 < 32768u) ? ((int32_t)(p16 * 2) - 32768) : (32768 - (int32_t)((p16 - 32768u) * 2));
    };
    static auto generateSawSampleQ15 = [](AudioChannel& ch) -> int32_t {
        const int64_t v = ((int64_t)ch.phaseQ32 << 1) - (1LL << 32);
        return (int32_t)(v >> 17);
    };
    static auto generateNoiseSampleQ15 = [](AudioChannel& ch) -> int32_t {
        return (ch.lfsrState & 1u) ? 32767 : -32767;
    };

    // Function pointer array for branch-free dispatch
    typedef int32_t (*WaveGeneratorQ15)(AudioChannel&);
    static constexpr WaveGeneratorQ15 WAVE_GENERATORS_Q15[] = {
        generatePulseSampleQ15,    // WaveType::PULSE = 0
        generateTriangleSampleQ15, // WaveType::TRIANGLE = 1
        nullptr,                    // WaveType::NOISE = 2 (special case - requires state update)
        nullptr,                    // WaveType::SINE = 3 (uses LUT)
        generateSawSampleQ15       // WaveType::SAW = 4
    };

    static_assert(
        sizeof(WAVE_GENERATORS_Q15) / sizeof(WAVE_GENERATORS_Q15[0]) == 5,
        "WAVE_GENERATORS_Q15 must have 5 entries for WaveType enum"
    );

    // ============================================================================
    // Internal helper functions
    // ============================================================================

    static inline uint32_t frequency_hz_to_phase_inc_q32(float hz, int sr) {
        if (sr <= 0) return 0u;
        if (hz < 0.0f) hz = 0.0f;
        const double inc = (double)hz * 4294967296.0 / (double)sr;
        if (inc < 0.0) return 0u;
        if (inc >= 4294967295.0) return 0xFFFFFFFFu;
        return (uint32_t)inc;
    }

    /** Linear sweep: updates frequency + phaseIncrement; decrements sweep counter. */
    static inline void apply_linear_frequency_sweep_float(AudioChannel& ch, int sr) {
        if (sr <= 0) return;
        if (ch.sweepSamplesTotal == 0 || ch.type == WaveType::NOISE) return;
        if (ch.sweepSamplesRemaining == 0) return;
        const float denom = (float)ch.sweepSamplesTotal;
        const float alpha = (float)(ch.sweepSamplesTotal - ch.sweepSamplesRemaining) / denom;
        ch.frequency = ch.sweepStartHz + (ch.sweepEndHz - ch.sweepStartHz) * alpha;
        ch.phaseIncrement = ch.frequency / (float)sr;
        ch.sweepSamplesRemaining--;
        if (ch.sweepSamplesRemaining == 0) {
            ch.frequency = ch.sweepEndHz;
            ch.phaseIncrement = ch.sweepEndHz / (float)sr;
            ch.sweepSamplesTotal = 0;
        }
    }

    static inline int16_t apply_master_bitcrush(int16_t sample, uint8_t bits) {
        if (bits == 0 || bits >= 16) return sample;
        const int shift = 16 - bits;
        int32_t v = (int32_t)sample;
        v = (v >> shift) << shift;
        if (v > 32767) return 32767;
        if (v < -32768) return -32768;
        return (int16_t)v;
    }

    namespace platforms = pixelroot32::platforms;
    namespace logging = pixelroot32::core::logging;
    using logging::LogLevel;
    using logging::log;

    // ------------------------------------------------------------------
    // Construction / lifecycle
    // ------------------------------------------------------------------
    ApuCore::ApuCore() {
        for (int i = 0; i < MAX_VOICES; ++i) {
            voices[i].reset();
            voices[i].type = WaveType::PULSE;
        }
    }

    void ApuCore::init(int sr) {
        // Graceful degradation: use safe default if invalid sample rate
        sampleRate = (sr > 0) ? sr : 44100;
        tickDurationSamples =
            (uint64_t)((float)sampleRate * 60.0f / (tempoBPM * (float)TICKS_PER_BEAT));
    }

    void ApuCore::reset() {
        for (int i = 0; i < MAX_VOICES; ++i) {
            voices[i].reset();
            voices[i].type = WaveType::PULSE;
        }
        masterVolume = 1.0f;
        masterVolumeScale = 65536;
        masterBitcrushBits_ = 0;
        hpfPrevIn = hpfPrevOut = 0.0f;
        hpfPrevInQ15 = hpfPrevOutQ15 = 0;  // Q15 state for no-FPU path
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
        firstSequencerCallAfterPlay_ = false;
    }

#if defined(UNIT_TEST)
    size_t ApuCore::countEnabledVoicesForTesting() const {
        size_t n = 0;
        for (int i = 0; i < MAX_VOICES; ++i) {
            if (voices[i].enabled) {
                ++n;
            }
        }
        return n;
    }

    size_t ApuCore::getSequencerMainNoteIndexForTesting() const {
        return currentNoteIndices[0];
    }
#endif

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

                case AudioCommandType::SET_MASTER_BITCRUSH: {
                    uint8_t b = cmd.masterBitcrushBits;
                    if (b > 15u) b = 15u;
                    masterBitcrushBits_ = b;
                    break;
                }

                case AudioCommandType::STOP_CHANNEL:
                    if (cmd.channelIndex < MAX_VOICES) {
                        voices[cmd.channelIndex].reset();
                    }
                    break;

                case AudioCommandType::MUSIC_PLAY: {
                    activeTrackCount = 1;
                    tracks[0] = cmd.track;
                    currentNoteIndices[0] = 0;

                    tickDurationSamples =
                        (uint64_t)((float)sampleRate * 60.0f / (tempoBPM * (float)TICKS_PER_BEAT));

                    // Anchor sequencer to audio-thread "now" so we do not treat
                    // elapsed time since boot as a backlog of ticks (would fire
                    // MAX_NOTES_PER_FRAME notes in one block and voice-steal).
                    const uint64_t startTick =
                        (tickDurationSamples > 0) ? (audioTimeSamples / tickDurationSamples) : 0;
                    globalTickCounter = startTick;
                    nextNoteTicks[0] = startTick;

                    for (size_t i = 0;
                         i < cmd.subTrackCount && activeTrackCount < MAX_MUSIC_TRACKS;
                         ++i) {
                        if (cmd.subTracks[i]) {
                            tracks[activeTrackCount] = cmd.subTracks[i];
                            currentNoteIndices[activeTrackCount] = 0;
                            nextNoteTicks[activeTrackCount] = startTick;
                            activeTrackCount++;
                        }
                    }

                    firstSequencerCallAfterPlay_ = true;
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
        if (currentTick <= globalTickCounter && globalTickCounter > 0
            && !firstSequencerCallAfterPlay_) {
            return;
        }
        firstSequencerCallAfterPlay_ = false;
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
                    event.preset = note.preset;  // Forward ADSR/LFO params
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
        Voice* ch = findVoiceForEvent(event.type);
        if (!ch) return;

        const VoiceType voiceType = toVoiceType(event.type);
        // Compatibility fallback required by migration plan.
        ch->type = toWaveType(voiceType);
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
        ch->basePhaseIncQ32 = ch->phaseIncQ32;

        // ADSR envelope initialization from preset (or legacy defaults).
        // Default values (attack=2ms, decay=0, sustain=1.0, release=5ms)
        // produce identical output to the old anti-click ramp.
        float attackTime   = 0.002f;
        float decayTime    = 0.0f;
        float sustainLevel = 1.0f;
        float releaseTime  = 0.005f;
        if (event.preset) {
            attackTime   = event.preset->attackTime;
            decayTime    = event.preset->decayTime;
            sustainLevel = event.preset->sustainLevel;
            releaseTime  = event.preset->releaseTime;
        }

        auto& env = ch->envelope;
        env.attackSamples  = (uint32_t)std::max(1.0f, attackTime  * (float)sampleRate);
        env.decaySamples   = (uint32_t)(decayTime   * (float)sampleRate);
        env.sustainLevel   = sustainLevel;
        // Clamp release to <= sampleRate/10 (100 ms max) per Decision D2
        env.releaseSamples = std::min((uint32_t)(releaseTime * (float)sampleRate),
                                     (uint32_t)(sampleRate / 10));
        env.sampleCounter  = 0;
        env.currentLevel   = 0.0f;
        env.stage          = EnvelopeState::Stage::ATTACK;

        ch->volume = event.volume;  // base volume (preset level)
        ch->targetVolume = event.volume;
        ch->volumeDelta = 0.0f;     // no longer used for anti-click

        env.attackDelta  = 1.0f / (float)env.attackSamples;
        env.decayDelta   = (env.decaySamples > 0) ? (1.0f - sustainLevel) / (float)env.decaySamples : 0.0f;
        env.releaseDelta = (env.releaseSamples > 0) ? sustainLevel / (float)env.releaseSamples : 0.0f;

#if defined(ESP32) && (!defined(SOC_CPU_HAS_FPU) || !SOC_CPU_HAS_FPU)
        // Initialize Q15 envelope fields for RISC-V fast path
        env.currentLevelQ15 = 0;
        env.sustainLevelQ15 = (int32_t)(sustainLevel * 32768.0f);
        env.attackDeltaQ15 = (env.attackSamples > 0) ? (32768 / (int32_t)env.attackSamples) : 32768;
        env.decayDeltaQ15 = (env.decaySamples > 0) ? ((32768 - env.sustainLevelQ15) / (int32_t)env.decaySamples) : 0;
        env.releaseDeltaQ15 = (env.releaseSamples > 0) ? (env.sustainLevelQ15 / (int32_t)env.releaseSamples) : 0;
#endif

        // LFO initialization
        ch->lfo.enabled = false;
        if (event.preset && event.preset->lfoTarget != LfoTarget::NONE && event.preset->lfoFrequency > 0.0f) {
            ch->lfo.enabled = true;
            ch->lfo.target = event.preset->lfoTarget;
            ch->lfo.depth = event.preset->lfoDepth;
            ch->lfo.periodSamples = (uint32_t)((float)sampleRate / event.preset->lfoFrequency);
            if (ch->lfo.periodSamples < 1u) ch->lfo.periodSamples = 1u;
            ch->lfo.sampleCounter = 0;
            ch->lfo.currentValue = 0.0f;
            ch->lfo.delaySamples = (uint16_t)(event.preset->lfoDelay * (float)sampleRate);
            ch->lfo.delayCounter = 0;
            // Convert float depth to Q15 for no-FPU path
            ch->lfo.depthQ15 = (int32_t)(ch->lfo.depth * 32768.0f);
            ch->lfo.currentValueQ15 = 0;
        }

        ch->remainingSamples = (uint64_t)(event.duration * (float)sampleRate);

        ch->sweepSamplesTotal = 0;
        ch->sweepSamplesRemaining = 0;
        if ((event.type == WaveType::PULSE || event.type == WaveType::TRIANGLE
                || event.type == WaveType::SINE || event.type == WaveType::SAW)
            && event.sweepDurationSec > 0.0f && event.sweepEndHz > 0.0f && sampleRate > 0) {
            const uint64_t noteLen = ch->remainingSamples;
            if (noteLen > 0) {
                uint64_t sweepLen = (uint64_t)(event.sweepDurationSec * (float)sampleRate);
                if (sweepLen == 0) sweepLen = 1;
                if (sweepLen > noteLen) sweepLen = noteLen;
                if (sweepLen >= 1) {
                    ch->sweepSamplesTotal = (uint32_t)sweepLen;
                    ch->sweepSamplesRemaining = ch->sweepSamplesTotal;
                    ch->sweepStartHz = event.frequency;
                    ch->sweepEndHz = event.sweepEndHz;
                    ch->sweepStartIncQ32 = frequency_hz_to_phase_inc_q32(event.frequency, sampleRate);
                    ch->sweepEndIncQ32 = frequency_hz_to_phase_inc_q32(event.sweepEndHz, sampleRate);
                }
            }
        }

        if (event.type == WaveType::PULSE) {
            ch->dutyCycle = event.duty;
            double d = (double)event.duty;
            if (d < 0.0) d = 0.0;
            if (d > 1.0) d = 1.0;
            ch->dutyCycleQ32 = (uint32_t)(d * 4294967296.0);
            
            if (event.preset) {
                ch->dutySweep = event.preset->dutySweep / (float)sampleRate;
                ch->dutySweepQ32 = (int32_t)((double)event.preset->dutySweep * 4294967296.0 / (double)sampleRate);
            } else {
                ch->dutySweep = 0.0f;
                ch->dutySweepQ32 = 0;
            }
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
            ch->noiseShortMode = (event.preset) ? event.preset->noiseShortMode : false;
            ch->phase = 0.0f;
            ch->phaseIncrement = 0.0f;
        } else if (event.type == WaveType::SINE || event.type == WaveType::SAW) {
            ch->dutyCycle = 0.5f;
            ch->dutySweep = 0.0f;
            ch->dutySweepQ32 = 0;
        }
    }

    Voice* ApuCore::findVoiceForEvent(WaveType type) {
        Voice* bestInactive = nullptr;
        Voice* bestSteal = nullptr;
        uint64_t minRemaining = UINT64_MAX;

        for (int i = 0; i < MAX_VOICES; ++i) {
            Voice& voice = voices[i];
            if (!voice.enabled) {
                if (voice.type == type) return &voice;
                if (!bestInactive) bestInactive = &voice;
                continue;
            }
            if (voice.remainingSamples < minRemaining) {
                minRemaining = voice.remainingSamples;
                bestSteal = &voice;
            }
        }

        if (bestInactive) return bestInactive;
        return bestSteal;
    }

    // ------------------------------------------------------------------
    // ADSR Envelope state machine (per-sample tick)
    // ------------------------------------------------------------------
    static inline void tickEnvelope(AudioChannel& ch) {
        auto& env = ch.envelope;
        switch (env.stage) {
            case EnvelopeState::Stage::ATTACK:
                env.sampleCounter++;
                // env.currentLevel = (float)env.sampleCounter / (float)env.attackSamples;
                env.currentLevel += env.attackDelta;
                if (env.sampleCounter >= env.attackSamples) {
                    env.currentLevel = 1.0f;
                    env.sampleCounter = 0;
                    // Skip decay if decaySamples == 0
                    if (env.decaySamples > 0) {
                        env.stage = EnvelopeState::Stage::DECAY;
                    } else {
                        env.currentLevel = env.sustainLevel;
                        env.stage = EnvelopeState::Stage::SUSTAIN;
                    }
                }
                break;

            case EnvelopeState::Stage::DECAY:
                env.sampleCounter++;
                // Linear ramp from 1.0 down to sustainLevel
                // env.currentLevel = 1.0f - (1.0f - env.sustainLevel)
                //     * ((float)env.sampleCounter / (float)env.decaySamples);
                env.currentLevel -= env.decayDelta;
                if (env.sampleCounter >= env.decaySamples) {
                    env.currentLevel = env.sustainLevel;
                    env.sampleCounter = 0;
                    env.stage = EnvelopeState::Stage::SUSTAIN;
                }
                break;

            case EnvelopeState::Stage::SUSTAIN:
                // Hold at sustainLevel — nothing to tick
                break;

            case EnvelopeState::Stage::RELEASE:
                env.sampleCounter++;
                // env.currentLevel = env.sustainLevel
                //     * (1.0f - (float)env.sampleCounter / (float)env.releaseSamples);
                env.currentLevel -= env.releaseDelta;
                if (env.sampleCounter >= env.releaseSamples || env.currentLevel <= 0.0f) {
                    env.currentLevel = 0.0f;
                    env.stage = EnvelopeState::Stage::OFF;
                    ch.enabled = false;
                }
                break;

            case EnvelopeState::Stage::OFF:
                break;
        }
    }

#if defined(ESP32) && (!defined(SOC_CPU_HAS_FPU) || !SOC_CPU_HAS_FPU)
    // ------------------------------------------------------------------
    // ADSR Envelope state machine (fixed-point Q15 for RISC-V)
    // ------------------------------------------------------------------
    static inline void tickEnvelopeQ15(AudioChannel& ch) {
        auto& env = ch.envelope;
        switch (env.stage) {
            case EnvelopeState::Stage::ATTACK:
                env.sampleCounter++;
                env.currentLevelQ15 += env.attackDeltaQ15;
                if (env.sampleCounter >= env.attackSamples) {
                    env.currentLevelQ15 = 32768; // 1.0 in Q15
                    env.sampleCounter = 0;
                    if (env.decaySamples > 0) {
                        env.stage = EnvelopeState::Stage::DECAY;
                    } else {
                        env.currentLevelQ15 = env.sustainLevelQ15;
                        env.stage = EnvelopeState::Stage::SUSTAIN;
                    }
                }
                break;
            case EnvelopeState::Stage::DECAY:
                env.sampleCounter++;
                env.currentLevelQ15 -= env.decayDeltaQ15;
                if (env.sampleCounter >= env.decaySamples) {
                    env.currentLevelQ15 = env.sustainLevelQ15;
                    env.sampleCounter = 0;
                    env.stage = EnvelopeState::Stage::SUSTAIN;
                }
                break;
            case EnvelopeState::Stage::SUSTAIN:
                break;
            case EnvelopeState::Stage::RELEASE:
                env.sampleCounter++;
                env.currentLevelQ15 -= env.releaseDeltaQ15;
                if (env.sampleCounter >= env.releaseSamples || env.currentLevelQ15 <= 0) {
                    env.currentLevelQ15 = 0;
                    env.stage = EnvelopeState::Stage::OFF;
                    ch.enabled = false;
                }
                break;
            case EnvelopeState::Stage::OFF:
                env.currentLevelQ15 = 0;
                break;
        }
    }
#endif

    // ------------------------------------------------------------------
    // LFO oscillator tick (triangle wave, per-sample)
    // ------------------------------------------------------------------
    static inline void tickLfo(AudioChannel& ch) {
        auto& lfo = ch.lfo;
        if (!lfo.enabled || lfo.periodSamples == 0) return;

        if (lfo.delayCounter < lfo.delaySamples) {
            lfo.delayCounter++;
            return;
        }

        lfo.sampleCounter++;
        if (lfo.sampleCounter >= lfo.periodSamples) lfo.sampleCounter = 0;

        // Triangle wave in [-1.0, +1.0]
        const float t = (float)lfo.sampleCounter / (float)lfo.periodSamples;
        lfo.currentValue = (t < 0.5f) ? (4.0f * t - 1.0f) : (3.0f - 4.0f * t);
    }

    // ------------------------------------------------------------------
    // LFO oscillator tick (Q15 integer-only, triangle wave)
    // ------------------------------------------------------------------
    static inline void tickLfoQ15(LfoState& lfo) {
        if (!lfo.enabled || lfo.periodSamples == 0) return;

        if (lfo.delayCounter < lfo.delaySamples) {
            lfo.delayCounter++;
            return;
        }

        lfo.sampleCounter++;
        if (lfo.sampleCounter >= lfo.periodSamples) lfo.sampleCounter = 0;

        // Integer triangle wave in Q15
        // tQ15 = (sampleCounter * 32768) / periodSamples (range: 0 to 32768)
        int32_t tQ15 = (int32_t)((uint64_t)lfo.sampleCounter * 32768u / lfo.periodSamples);

        // Triangle: if t < 0.5: 4*t - 1, else: 3 - 4*t
        // In Q15: if tQ15 < 16384: 4*tQ15 - 32768, else: 98304 - 4*tQ15
        if (tQ15 < 16384) {
            lfo.currentValueQ15 = (tQ15 << 2) - 32768;  // 4*tQ15 - 32768
        } else {
            lfo.currentValueQ15 = 98304 - (tQ15 << 2);  // 98304 - 4*tQ15
        }
    }

    // ------------------------------------------------------------------
    // Per-channel sample generation
    // ------------------------------------------------------------------
    float ApuCore::generateSampleForVoice(Voice& ch) {
        if (!ch.enabled) return 0.0f;

        apply_linear_frequency_sweep_float(ch, sampleRate);

        float sample = 0.0f;
        switch (ch.type) {
            case WaveType::PULSE:
                sample = (ch.phase < ch.dutyCycle) ? 1.0f : -1.0f;
                break;
            case WaveType::TRIANGLE:
                // TODO: future NES-accurate 4-bit quantization (deferred)
                sample = (ch.phase < 0.5f)
                       ? (4.0f * ch.phase - 1.0f)
                       : (3.0f - 4.0f * ch.phase);
                break;
            case WaveType::SINE: {
                const unsigned i = (unsigned)(ch.phase * 256.0f) & 255u;
                sample = (float)SINE_LUT_Q15[i] / 32768.0f;
                break;
            }
            case WaveType::SAW:
                sample = 2.0f * ch.phase - 1.0f;
                break;
            case WaveType::NOISE: {
                // Unified 15-bit NES LFSR shared by all platforms so the
                // timbre is identical in simulator and hardware.
                if (ch.noiseCountdown > 0u) ch.noiseCountdown--;
                if (ch.noiseCountdown == 0u) {
                    const uint16_t bitToXor = ch.noiseShortMode ? ((ch.lfsrState >> 6) & 1u) : ((ch.lfsrState >> 1) & 1u);
                    const uint16_t fb = (uint16_t)(((ch.lfsrState & 1u) ^ bitToXor) & 1u);
                    ch.lfsrState = (uint16_t)((ch.lfsrState >> 1) | (fb << 14));
                    ch.noiseCountdown = ch.noisePeriodSamples;
                }
                sample = (ch.lfsrState & 1u) ? 1.0f : -1.0f;
                break;
            }
            default:
                sample = 0.0f;
                break;
        }

        if (ch.type != WaveType::NOISE) {
            ch.phase += ch.phaseIncrement;
            if (ch.phase >= 1.0f) ch.phase -= 1.0f;
            
            if (ch.type == WaveType::PULSE && ch.dutySweep != 0.0f) {
                ch.dutyCycle += ch.dutySweep;
                if (ch.dutyCycle > 1.0f) ch.dutyCycle -= 1.0f;
                else if (ch.dutyCycle < 0.0f) ch.dutyCycle += 1.0f;
            }
        }

        // Tick the ADSR envelope
        tickEnvelope(ch);

        // Tick the LFO and apply modulation
        tickLfo(ch);
        float lfoVolMod = 1.0f;
        if (ch.lfo.enabled) {
            if (ch.lfo.target == LfoTarget::PITCH) {
                // Vibrato: modulate phase increment (frequency)
                ch.phaseIncrement = ch.frequency / (float)sampleRate
                    * (1.0f + ch.lfo.currentValue * ch.lfo.depth);
            } else if (ch.lfo.target == LfoTarget::VOLUME) {
                // Tremolo: modulate output amplitude
                lfoVolMod = 1.0f - ch.lfo.depth * 0.5f * (1.0f - ch.lfo.currentValue);
            }
        }

        // Trigger RELEASE when remaining duration expires (Decision D2:
        // release samples are added to remainingSamples at transition).
        if (ch.remainingSamples > 0) {
            ch.remainingSamples--;
            if (ch.remainingSamples == 0) {
                if (ch.envelope.stage != EnvelopeState::Stage::RELEASE
                    && ch.envelope.stage != EnvelopeState::Stage::OFF
                    && ch.envelope.releaseSamples > 0) {
                    // Enter release and extend channel lifetime
                    ch.envelope.stage = EnvelopeState::Stage::RELEASE;
                    ch.envelope.sampleCounter = 0;
                    ch.remainingSamples = ch.envelope.releaseSamples;
                } else {
                    ch.enabled = false;
                }
            }
        }

        return sample * ch.volume * ch.envelope.currentLevel * lfoVolMod;
    }

    // ------------------------------------------------------------------
    // Main mixing path
    // ------------------------------------------------------------------
    void ApuCore::generateSamples(int16_t* stream, int length) {
        if (!stream || length <= 0) return;

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
            for (int c = 0; c < MAX_VOICES; ++c) {
                if (voices[c].enabled) {
                    acc += generateSampleForVoice(voices[c]) * MIXER_SCALE;
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
            stream[i] = apply_master_bitcrush((int16_t)finalSample, masterBitcrushBits_);
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
        int32_t volQ15[MAX_VOICES];
        // envQ15 is now tracked continuously per-channel via tickEnvelopeQ15
        for (int c = 0; c < MAX_VOICES; ++c) {
            volQ15[c] = (int32_t)(voices[c].volume * 32768.0f);
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

            for (int c = 0; c < MAX_VOICES; ++c) {
                Voice& ch = voices[c];
                if (!ch.enabled) continue;

                int32_t s = 0; // Q15 sample in [-32767, +32767]

                // Branch-free type dispatch using function pointer lookup
                if (ch.type == WaveType::NOISE) {
                    // NOISE: inline state update (required for LFSR mutation)
                    if (ch.noiseCountdown > 0u) ch.noiseCountdown--;
                    if (ch.noiseCountdown == 0u) {
                        const uint16_t bitToXor = ch.noiseShortMode ? ((ch.lfsrState >> 6) & 1u) : ((ch.lfsrState >> 1) & 1u);
                        const uint16_t fb = (uint16_t)(((ch.lfsrState & 1u) ^ bitToXor) & 1u);
                        ch.lfsrState = (uint16_t)((ch.lfsrState >> 1) | (fb << 14));
                        ch.noiseCountdown = ch.noisePeriodSamples;
                    }
                    s = generateNoiseSampleQ15(ch);
                } else if (ch.type == WaveType::SINE) {
                    // SINE: direct LUT lookup
                    s = (int32_t)SINE_LUT_Q15[(ch.phaseQ32 >> 24) & 255u];
                } else if (auto gen = WAVE_GENERATORS_Q15[static_cast<int>(ch.type)]) {
                    // PULSE, TRIANGLE, SAW: function pointer lookup
                    s = gen(ch);
                } else {
                    // Fallback (should never reach)
                    s = 0;
                }

                // Apply per-channel Q15 volume × envelope: (s * volQ15 * envQ15) >> 30
                int32_t sv = (s * volQ15[c]) >> 15;
                sv = (sv * ch.envelope.currentLevelQ15) >> 15;

                // Volume LFO (tremolo) — Q15 only, no float conversion.
                if (ch.lfo.enabled && ch.lfo.target == LfoTarget::VOLUME) {
                    // Tremolo: vol = 1.0 - depth * 0.5 * (1.0 - lfo)
                    // In Q15: volQ15 = 32768 - ((depthQ15 * (32768 - currentValueQ15)) >> 16)
                    int32_t volModQ15 = 32768 - ((ch.lfo.depthQ15 * (32768 - ch.lfo.currentValueQ15)) >> 16);
                    sv = (sv * volModQ15) >> 15;
                }

                // MIXER_SCALE = 0.4 per channel. 0.4 ≈ 13107/32768 so we
                // scale by 13107 and shift 15 to land back in Q15. This
                // matches the FPU branch exactly (MIXER_SCALE * channel).
                sv = (sv * 13107) >> 15;
                sum += sv;

                // Phase accumulator (integer, wraps automatically).
                if (ch.type != WaveType::NOISE) {
                    ch.phaseQ32 += ch.phaseIncQ32;
                    if (ch.type == WaveType::PULSE && ch.dutySweepQ32 != 0) {
                        ch.dutyCycleQ32 += ch.dutySweepQ32; // naturally wraps
                    }
                }

                // ADSR envelope tick (fixed-point Q15, once per sample).
                tickEnvelopeQ15(ch);

                // Linear frequency sweep (integer phase inc, no soft-float in lerp).
                if (ch.sweepSamplesTotal > 0 && ch.type != WaveType::NOISE && ch.sweepSamplesRemaining > 0) {
                    const uint32_t numer = ch.sweepSamplesTotal - ch.sweepSamplesRemaining;
                    const int64_t delta = (int64_t)ch.sweepEndIncQ32 - (int64_t)ch.sweepStartIncQ32;
                    int64_t inc = (int64_t)ch.sweepStartIncQ32
                        + (delta * (int64_t)numer) / (int64_t)ch.sweepSamplesTotal;
                    if (inc < 0) inc = 0;
                    if (inc > (int64_t)0xFFFFFFFFLL) inc = 0xFFFFFFFFLL;
                    ch.basePhaseIncQ32 = (uint32_t)inc;
                    ch.sweepSamplesRemaining--;
                    if (ch.sweepSamplesRemaining == 0) {
                        ch.basePhaseIncQ32 = ch.sweepEndIncQ32;
                        ch.sweepSamplesTotal = 0;
                    }
                }

                // LFO tick + pitch modulation (Q15-only, no float).
                tickLfoQ15(ch.lfo);
                if (ch.lfo.enabled && ch.lfo.target == LfoTarget::PITCH) {
                    // Vibrato: inc = base * (1 + lfo * depth)
                    // modQ15 = currentValueQ15 * depthQ15 >> 15
                    int32_t modQ15 = (int32_t)((int64_t)ch.lfo.currentValueQ15 * ch.lfo.depthQ15 >> 15);
                    int64_t inc = (int64_t)ch.basePhaseIncQ32 + ((int64_t)ch.basePhaseIncQ32 * modQ15 >> 15);
                    ch.phaseIncQ32 = (inc < 0) ? 0u : (uint32_t)inc;
                }

                // Duration countdown + release trigger (same as float path).
                if (ch.remainingSamples > 0) {
                    ch.remainingSamples--;
                    if (ch.remainingSamples == 0) {
                        if (ch.envelope.stage != EnvelopeState::Stage::RELEASE
                            && ch.envelope.stage != EnvelopeState::Stage::OFF
                            && ch.envelope.releaseSamples > 0) {
                            ch.envelope.stage = EnvelopeState::Stage::RELEASE;
                            ch.envelope.sampleCounter = 0;
                            ch.remainingSamples = ch.envelope.releaseSamples;
                        } else {
                            ch.enabled = false;
                        }
                    }
                }
            }

            int32_t index = (sum + 131072) >> 8;
            if (index < 0) index = 0;
            if (index > 1024) index = 1024;

            // Q15 HPF coefficient: R = 0.995, Q15 = 0.995 * 32768 = 32604
            // Yields ~35 Hz -3dB cutoff at 22050 Hz sample rate
            static constexpr int32_t HPF_R_Q15 = 32604;

            // Q15 HPF: y[n] = x[n] - x[n-1] + (R * y[n-1])
            // Uses Q15 fixed-point to avoid soft-float on RISC-V cores
            int32_t inputQ15 = audio_mixer_lut[index];

            // Compute R * y[n-1] in Q30, then shift to Q15
            int64_t feedback = (int64_t)HPF_R_Q15 * (int64_t)hpfPrevOutQ15;
            int32_t feedbackQ15 = (int32_t)(feedback >> 15);

            // HPF difference equation
            int32_t hpfOutQ15 = inputQ15 - hpfPrevInQ15 + feedbackQ15;

            // Saturating clamp to prevent overflow artifacts
            if (hpfOutQ15 > 32767) hpfOutQ15 = 32767;
            if (hpfOutQ15 < -32768) hpfOutQ15 = -32768;

            // Update state for next sample
            hpfPrevInQ15 = inputQ15;
            hpfPrevOutQ15 = hpfOutQ15;

            // Convert to Q14 for master volume (matches FPU path scaling)
            int32_t finalSample = hpfOutQ15 >> 1;

            // Apply master volume after HPF (matches FPU path order)
            if (masterVolumeScale != 65536) {
                finalSample = (finalSample * masterVolumeScale) >> 16;
                // Clamp to 16-bit range
                if (finalSample > 32767) finalSample = 32767;
                if (finalSample < -32768) finalSample = -32768;
            }
            
            stream[i] = apply_master_bitcrush((int16_t)finalSample, masterBitcrushBits_);

            const int32_t absSample = (stream[i] < 0) ? -(int32_t)stream[i] : (int32_t)stream[i];
            if ((float)absSample > currentPeak) currentPeak = (float)absSample;
        }

        // Sync per-channel float volume back from Q15 so the next block
        // (and the FPU fallback if ever invoked) sees up-to-date state.
        for (int c = 0; c < MAX_VOICES; ++c) {
            voices[c].volume = (float)volQ15[c] / 32768.0f;
        }
#else
        // ---- Native / fallback path -------------------------------------
        for (int i = 0; i < length; ++i) {
            float acc = 0.0f;
            for (int c = 0; c < MAX_VOICES; ++c) {
                if (voices[c].enabled) {
                    acc += generateSampleForVoice(voices[c]) * MIXER_SCALE;
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
            stream[i] = apply_master_bitcrush((int16_t)finalSample, masterBitcrushBits_);
        }
#endif

        if (postMixMono_) {
            postMixMono_(stream, length, postMixUser_);
        }

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
    }

    void ApuCore::setPostMixMono(void (*fn)(int16_t* mono, int length, void* user), void* user) {
        postMixMono_ = fn;
        postMixUser_ = user;
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
