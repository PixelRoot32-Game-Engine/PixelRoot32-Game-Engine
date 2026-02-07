/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#ifdef ESP32

#include "drivers/esp32/ESP32AudioScheduler.h"
#include <algorithm>
#include <cstring>

namespace pixelroot32::audio {

    ESP32AudioScheduler::ESP32AudioScheduler(int coreId, int priority)
        : coreId(coreId), priority(priority) {
    }

    ESP32AudioScheduler::~ESP32AudioScheduler() {
        stop();
    }

    void ESP32AudioScheduler::init(AudioBackend* backend, int sampleRate) {
        this->backend = backend;
        this->sampleRate = sampleRate;
        for (int i = 0; i < NUM_CHANNELS; i++) {
            channels[i].reset();
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

        memset(stream, 0, length * sizeof(int16_t));

        for (int i = 0; i < NUM_CHANNELS; i++) {
            if (!channels[i].enabled) continue;

            for (int j = 0; j < length; j++) {
                int16_t sample = generateSampleForChannel(channels[i]);
                // Mix sample (simple additive mixing with master volume)
                float mixed = (float)stream[j] + (float)sample * masterVolume;
                // Clamp to 16-bit range
                stream[j] = (int16_t)std::clamp(mixed, -32768.0f, 32767.0f);
            }
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
                    masterVolume = std::clamp(cmd.volume, 0.0f, 1.0f);
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

    AudioChannel* ESP32AudioScheduler::findFreeChannel(WaveType /*type*/) {
        for (int i = 0; i < NUM_CHANNELS; i++) {
            if (!channels[i].enabled) return &channels[i];
        }
        return &channels[0]; // Simple steal
    }

    int16_t ESP32AudioScheduler::generateSampleForChannel(AudioChannel& ch) {
        float sample = 0.0f;
        
        switch (ch.type) {
            case WaveType::SQUARE:
                sample = (ch.phase < 0.5f) ? 1.0f : -1.0f;
                break;
            case WaveType::PULSE:
                sample = (ch.phase < ch.dutyCycle) ? 1.0f : -1.0f;
                break;
            case WaveType::TRIANGLE:
                sample = 4.0f * fabsf(ch.phase - 0.5f) - 1.0f;
                break;
            case WaveType::SAWTOOTH:
                sample = 2.0f * ch.phase - 1.0f;
                break;
            case WaveType::SINE:
                sample = sinf(ch.phase * 2.0f * M_PI);
                break;
            case WaveType::NOISE:
                sample = ((float)rand() / (float)RAND_MAX) * 2.0f - 1.0f;
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

        return (int16_t)(sample * ch.volume * 32767.0f);
    }

} // namespace pixelroot32::audio

#endif // ESP32
