/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#ifdef PLATFORM_NATIVE

#include "drivers/native/NativeAudioScheduler.h"
#include "audio/AudioMusicTypes.h"
#include <cstring>
#include <cmath>
#include <algorithm>
#include <chrono>
#include <cstdlib>

namespace pixelroot32::audio {

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

    void NativeAudioScheduler::init(AudioBackend* /*backend*/, int sampleRate, const pixelroot32::core::PlatformCapabilities& /*caps*/) {
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

        while (running) {
            if (rbAvailableToWrite() >= CHUNK_SIZE) {
                processCommands();
                updateMusicSequencer(CHUNK_SIZE);

                memset(chunk, 0, sizeof(chunk));

                for (int c = 0; c < NUM_CHANNELS; c++) {
                    if (!channels[c].enabled) continue;

                    for (int i = 0; i < CHUNK_SIZE; i++) {
                        int16_t sample = generateSampleForChannel(channels[c]);
                        float mixed = (float)chunk[i] + (float)sample * masterVolume;
                        
                        if (mixed > 32767.0f) mixed = 32767.0f;
                        if (mixed < -32768.0f) mixed = -32768.0f;
                        
                        chunk[i] = (int16_t)mixed;
                    }
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

    void NativeAudioScheduler::updateMusicSequencer(int /*length*/) {
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

    void NativeAudioScheduler::playCurrentNote() {
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

    int16_t NativeAudioScheduler::generateSampleForChannel(AudioChannel& ch) {
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

        return (int16_t)(sample * ch.volume * 12000.0f);
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
