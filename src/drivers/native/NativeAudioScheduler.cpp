/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#ifdef PLATFORM_NATIVE

#include "drivers/native/NativeAudioScheduler.h"
#include "audio/AudioMusicTypes.h"
#include "platforms/EngineConfig.h"
#include <cstring>
#include <cmath>
#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <cstdio>

namespace pixelroot32::audio {

    namespace logging = pixelroot32::core::logging;
    using logging::LogLevel;
    using logging::log;

    NativeAudioScheduler::NativeAudioScheduler(size_t ringBufferSize)
        : rbCapacity(ringBufferSize) {
        ringBuffer.resize(rbCapacity);
        
        channels[0].type = WaveType::PULSE;
        channels[1].type = WaveType::PULSE;
        channels[2].type = WaveType::TRIANGLE;
        channels[3].type = WaveType::NOISE;

        for (int i = 0; i < NUM_CHANNELS; i++) {
            channels[i].reset();
        }
    }

    NativeAudioScheduler::~NativeAudioScheduler() {
        stop();
    }

    void NativeAudioScheduler::init(AudioBackend* /*backend*/, int sampleRate, const pixelroot32::platforms::PlatformCapabilities& /*caps*/) {
        this->sampleRate = sampleRate;
    }

    void NativeAudioScheduler::submitCommand(const AudioCommand& cmd) {
        commandQueue.enqueue(cmd);
    }

    void NativeAudioScheduler::start() {
        if (running) return;
        running = true;
        audioThread = std::thread(&NativeAudioScheduler::threadLoop, this);
    }

    void NativeAudioScheduler::stop() {
        if (!running) return;
        running = false;
        if (audioThread.joinable()) {
            audioThread.join();
        }
    }

    void NativeAudioScheduler::generateSamples(int16_t* stream, int length) {
        if (!stream || length <= 0) return;

        size_t available = rbAvailableToRead();
        if (available < (size_t)length) {
            // Underflow - fill what we can and zero the rest
            rbRead(stream, available);
            memset(stream + available, 0, (length - available) * sizeof(int16_t));
        } else {
            rbRead(stream, length);
        }
    }

    void NativeAudioScheduler::threadLoop() {
        const int CHUNK_SIZE = 128;
        int16_t chunk[CHUNK_SIZE];

        auto lastLogTime = std::chrono::steady_clock::now();
        float currentPeak = 0.0f;
        const float MIXER_SCALE = 0.4f; // 40% per channel (Non-linear)
        const float MIXER_K = 0.5f;     // Compression factor
        const float FINAL_SCALE = 32767.0f;

        while (running) {
            if (rbAvailableToWrite() >= CHUNK_SIZE) {
                processCommands();
                updateMusicSequencer(CHUNK_SIZE);

                for (int i = 0; i < CHUNK_SIZE; i++) {
                    float acc = 0.0f;
                    for (int c = 0; c < NUM_CHANNELS; c++) {
                        if (channels[c].enabled) {
                            acc += generateSampleForChannel(channels[c]) * MIXER_SCALE;
                        }
                    }

                    // Apply Master Volume before non-linear compression
                    acc *= masterVolume;

                    // Non-linear mixing formula: f(x) = x / (1 + |x| * K)
                    float mixed = acc / (1.0f + std::abs(acc) * MIXER_K);
                    float finalSample = mixed * FINAL_SCALE;
                    
                    // Track peak (Phase 2)
                    float absSample = std::abs(finalSample);
                    if (absSample > currentPeak) currentPeak = absSample;

                    // Clamp once at the end
                    if (finalSample > 32767.0f) finalSample = 32767.0f;
                    if (finalSample < -32768.0f) finalSample = -32768.0f;
                    
                    chunk[i] = (int16_t)finalSample;
                }

                // Diagnostic logging (Phase 2)
                auto now = std::chrono::steady_clock::now();
                if (std::chrono::duration_cast<std::chrono::seconds>(now - lastLogTime).count() >= 1) {
                    if constexpr (pixelroot32::platforms::config::EnableProfiling) {
                        if (currentPeak > 32767.0f) {
                            log(LogLevel::Profiling, "[AUDIO] PEAK DETECTED: %.0f (CLIPPING!)", currentPeak);
                        } else {
                            log(LogLevel::Profiling, "[AUDIO] Peak: %.0f (%.1f%%)", currentPeak, (currentPeak / 32767.0f) * 100.0f);
                        }
                    }
                    currentPeak = 0.0f;
                    lastLogTime = now;
                }

                rbWrite(chunk, CHUNK_SIZE);
                audioTimeSamples += CHUNK_SIZE;
            } else {
                // Buffer full, wait a bit
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        }
    }

    void NativeAudioScheduler::processCommands() {
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

    void NativeAudioScheduler::updateMusicSequencer(int /*length*/) {
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

    void NativeAudioScheduler::playCurrentNote() {
        // Multi-track: notes are played in updateMusicSequencer directly
    }

    void NativeAudioScheduler::executePlayEvent(const AudioEvent& event) {
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
                // Initialize noise channel
                // If noisePeriod is set (>0), use it, otherwise use frequency-based calc
                if (event.noisePeriod > 0) {
                    ch->noisePeriodSamples = event.noisePeriod;
                } else {
                    float noiseHz = event.frequency;
                    if (noiseHz <= 0.0f) {
                        noiseHz = 1000.0f;
                    }
                    ch->noisePeriodSamples = (uint32_t)((float)sampleRate / noiseHz);
                }
                ch->noiseRegister = 0x4000;
                ch->noiseCountdown = 1u;  // Initialize countdown
                ch->phaseIncrement = 0.0f;
            }
        }
    }

    AudioChannel* NativeAudioScheduler::findFreeChannel(WaveType type) {
        AudioChannel* candidate = nullptr;
        uint64_t minRemaining = 0xFFFFFFFFFFFFFFFFULL;

        for (int i = 0; i < NUM_CHANNELS; i++) {
            if (channels[i].type == type) {
                if (!channels[i].enabled) return &channels[i];
                if (channels[i].remainingSamples < minRemaining) {
                    minRemaining = channels[i].remainingSamples;
                    candidate = &channels[i];
                }
            }
        }
        return candidate;
    }

    float NativeAudioScheduler::generateSampleForChannel(AudioChannel& ch) {
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
                // Use noisePeriodSamples to control LFSR update rate
                if (ch.noiseCountdown > 0u) {
                    ch.noiseCountdown--;
                }
                if (ch.noiseCountdown == 0u) {
                    ch.noiseRegister = (uint16_t)(rand() & 0xFFFF);
                    ch.noiseCountdown = ch.noisePeriodSamples;
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

        return sample * ch.volume;
    }

    // Ring buffer implementation
    size_t NativeAudioScheduler::rbAvailableToRead() const {
        size_t r = rbReadPos.load(std::memory_order_acquire);
        size_t w = rbWritePos.load(std::memory_order_acquire);
        if (w >= r) return w - r;
        return rbCapacity - (r - w);
    }

    size_t NativeAudioScheduler::rbAvailableToWrite() const {
        size_t r = rbReadPos.load(std::memory_order_acquire);
        size_t w = rbWritePos.load(std::memory_order_acquire);
        size_t avail;
        if (w >= r) avail = rbCapacity - (w - r);
        else avail = r - w;
        return avail > 0 ? avail - 1 : 0; // Keep one slot empty to distinguish full/empty
    }

    void NativeAudioScheduler::rbWrite(const int16_t* data, size_t count) {
        size_t w = rbWritePos.load(std::memory_order_relaxed);
        for (size_t i = 0; i < count; i++) {
            ringBuffer[w] = data[i];
            w = (w + 1) % rbCapacity;
        }
        rbWritePos.store(w, std::memory_order_release);
    }

    void NativeAudioScheduler::rbRead(int16_t* data, size_t count) {
        size_t r = rbReadPos.load(std::memory_order_relaxed);
        for (size_t i = 0; i < count; i++) {
            data[i] = ringBuffer[r];
            r = (r + 1) % rbCapacity;
        }
        rbReadPos.store(r, std::memory_order_release);
    }

} // namespace pixelroot32::audio

#endif // PLATFORM_NATIVE
