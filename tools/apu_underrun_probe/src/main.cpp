/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */

/**
 * @file main.cpp
 * @brief Underrun gate harness (E0 acceptance, S15.6).
 *
 * Runs a real `AudioEngine` plus `ESP32_I2S_AudioBackend` for `RUN_SECONDS` with
 * eight voices and prints the backend's own counters. This is deliberately not a
 * benchmark: the CPU baseline comes from the APU's `tools/apu_bench_firmware`,
 * which renders into RAM. Here the samples go out through I2S, so the numbers
 * reported are the ones that decide whether the producer kept up:
 *
 * - `underruns`: the DMA ring had already drained when we came to write (the
 *   documented rule: the write barely waited, see `ESP32_I2S_AudioBackend`).
 * - `worst_block_us`: the slowest whole iteration.
 * - `worst_write_us`: the longest the write waited for space.
 *
 * Pins are the project's `esp32_dev_ST7789` mapping (BCLK 26, LRCK 25, DOUT 22,
 * MAX98357A-class DAC). No speaker is required for the gate to be valid: the I2S
 * peripheral consumes the ring at the sample rate either way.
 */

#include <Arduino.h>

#include <audio/AudioConfig.h>
#include <audio/AudioEngine.h>
#include <drivers/esp32/ESP32_I2S_AudioBackend.h>
#include <pixelroot32/apu/AudioMusicTypes.h>
#include <pixelroot32/apu/AudioTypes.h>

#ifndef RUN_SAMPLE_RATE
#define RUN_SAMPLE_RATE 22050
#endif

#ifndef RUN_SECONDS
#define RUN_SECONDS 600
#endif

namespace {

using pixelroot32::audio::AudioCommand;
using pixelroot32::audio::AudioCommandType;
using pixelroot32::audio::AudioConfig;
using pixelroot32::audio::AudioEngine;
using pixelroot32::audio::AudioEvent;
using pixelroot32::audio::InstrumentPreset;
using pixelroot32::audio::MusicNote;
using pixelroot32::audio::MusicTrack;
using pixelroot32::audio::Note;
using pixelroot32::audio::WaveType;
using pixelroot32::drivers::esp32::ESP32_I2S_AudioBackend;

using pixelroot32::audio::INSTR_PULSE_LEAD;
using pixelroot32::audio::INSTR_TRIANGLE_BASS;

/// @brief I2S pins (BCLK / LRCK / DOUT).
///
/// The ESP32 mapping is the project's `esp32_dev_ST7789` one (MAX98357A). The C3
/// has no GPIO 26/25/22 at all — it stops at 21 — so asking for them makes
/// `i2s_set_pin` fail, the backend return before creating its task, and the
/// counters read zero forever: a gate that would pass without rendering a single
/// sample. Hence the per-target block, and the liveness check in `loop`.
#if defined(CONFIG_IDF_TARGET_ESP32C3)
constexpr int kI2sBclk = 4;
constexpr int kI2sLrck = 5;
constexpr int kI2sDout = 6;
#else
constexpr int kI2sBclk = 26;
constexpr int kI2sLrck = 25;
constexpr int kI2sDout = 22;
#endif

// The render block is left at the platform default (512 with an FPU, 1024
// without), so the gate measures the configuration the product ships on each
// board instead of one picked here.

/// @brief One 128-beat note, long enough to outlive the whole run.
constexpr float kMusicNoteBeats = 128.0f;

constexpr MusicNote kMusicNotes0[] = {
    {Note::A, 4, kMusicNoteBeats, 0.30f, &INSTR_PULSE_LEAD}};
constexpr MusicNote kMusicNotes1[] = {
    {Note::E, 5, kMusicNoteBeats, 0.28f, nullptr}};
constexpr MusicNote kMusicNotes2[] = {
    {Note::C, 5, kMusicNoteBeats, 0.22f, nullptr}};
constexpr MusicNote kMusicNotes3[] = {
    {Note::G, 4, kMusicNoteBeats, 0.22f, nullptr}};

constexpr MusicTrack kMusicTracks[] = {
    {kMusicNotes0, 1, true, WaveType::PULSE, 0.5f, nullptr, nullptr, nullptr},
    {kMusicNotes1, 1, true, WaveType::TRIANGLE, 0.5f, nullptr, nullptr, nullptr},
    {kMusicNotes2, 1, true, WaveType::SINE, 0.5f, nullptr, nullptr, nullptr},
    {kMusicNotes3, 1, true, WaveType::SAW, 0.5f, nullptr, nullptr, nullptr},
};

/// File scope on purpose: the backend carries a 1 KB render buffer and the
/// Arduino loop task stack is 8 KB. See the engine's audio guide.
ESP32_I2S_AudioBackend gBackend(kI2sBclk, kI2sLrck, kI2sDout, RUN_SAMPLE_RATE);
/// Peak of the mixed buffer, written by the audio task through the post-mix
/// hook. It is the only way this harness can prove that voices are actually
/// sounding: the underrun counters say nothing about the mix, and a gate that
/// reports ``underruns=0`` over silence is worthless.
volatile int32_t gPeak = 0;

void postMixMono(int16_t* mono, int length, void*)
{
    int32_t peak = 0;
    for (int i = 0; i < length; ++i) {
        const int32_t value = mono[i] < 0 ? -static_cast<int32_t>(mono[i]) : mono[i];
        if (value > peak) {
            peak = value;
        }
    }
    if (peak > gPeak) {
        gPeak = peak;
    }
}
AudioConfig gConfig(&gBackend, RUN_SAMPLE_RATE);
AudioEngine gEngine(gConfig);

/// Wires the post-mix peak counter into the config once the engine exists.
struct PostMixWireup {
    PostMixWireup() { gConfig.postMixMono = &postMixMono; }
};
PostMixWireup gPostMixWireup;

AudioEvent makeLoopEvent(WaveType type, float frequency, float volume,
                         const InstrumentPreset* preset)
{
    AudioEvent event{};
    event.type = type;
    event.frequency = frequency;
    event.duration = 0.0f;
    event.volume = volume;
    event.duty = 0.5f;
    event.preset = preset;
    event.loop = true;
    return event;
}

/// @brief Hands one command to the engine.
///
/// `AudioEngine::submitCommand` returns void (only `ApuCore` exposes the
/// queue result), so a rejection would be silent. Five commands against a
/// 64-slot queue cannot fill it, and the run prints the block cost, which would
/// drop if a voice failed to start.
void submit(const AudioCommand& cmd)
{
    gEngine.submitCommand(cmd);
}

/**
 * @brief Fills the four sequencer slots and the four SFX slots.
 *
 * Same recipe as the host benchmark: `PLAY_EVENT` alone can only own slots 4-7,
 * so the other four voices come from a music command with three sub-tracks.
 */
void startEightVoices()
{
    AudioCommand event;
    event.type = AudioCommandType::PLAY_EVENT;
    event.event = makeLoopEvent(WaveType::PULSE, 330.0f, 0.30f, &INSTR_PULSE_LEAD);
    submit(event);

    AudioCommand music;
    music.type = AudioCommandType::MUSIC_PLAY;
    music.track = &kMusicTracks[0];
    music.subTracks[0] = &kMusicTracks[1];
    music.subTracks[1] = &kMusicTracks[2];
    music.subTracks[2] = &kMusicTracks[3];
    music.subTrackCount = 3;
    submit(music);

    event.event = makeLoopEvent(WaveType::TRIANGLE, 196.0f, 0.28f,
                                &INSTR_TRIANGLE_BASS);
    submit(event);
    event.event = makeLoopEvent(WaveType::SINE, 523.25f, 0.22f, nullptr);
    submit(event);
    event.event = makeLoopEvent(WaveType::SAW, 392.0f, 0.22f, nullptr);
    submit(event);
}

void printReport(const char* tag, uint32_t elapsedSeconds)
{
    Serial.printf(
        "REPORT %s rate=%d block=%d seconds=%u underruns=%u worst_block_us=%u "
        "worst_write_us=%u peak=%ld\n",
        tag, RUN_SAMPLE_RATE, gEngine.getBlockSize(), elapsedSeconds,
        static_cast<unsigned>(gBackend.getUnderrunCount()),
        static_cast<unsigned>(gBackend.getWorstBlockUs()),
        static_cast<unsigned>(gBackend.getWorstWriteUs()),(long)gPeak);
    Serial.flush();
}

uint32_t gStartMs = 0;
uint32_t gLastPrintSeconds = 0;

}  // namespace

void setup()
{
    Serial.begin(115200);
    delay(1500);

    Serial.println();
    Serial.printf("# apu_underrun_probe rate=%d seconds=%d block=%d pins=%d/%d/%d\n",
                  RUN_SAMPLE_RATE, RUN_SECONDS, gConfig.blockSize, kI2sBclk,
                  kI2sLrck, kI2sDout);
    Serial.printf("# chip=%s rev=%u mhz=%u\n", ESP.getChipModel(),
                  static_cast<unsigned>(ESP.getChipRevision()),
                  static_cast<unsigned>(ESP.getCpuFreqMHz()));
    Serial.flush();

    gEngine.init();
    startEightVoices();
    Serial.printf("# state music=%d volume_pct=%d\n", gEngine.isMusicPlaying() ? 1 : 0,
                  (int)(gEngine.getMasterVolume() * 100.0f));
    Serial.flush();
    gStartMs = millis();
    Serial.println("# running");
    Serial.flush();
}

void loop()
{
    const uint32_t elapsedMs = millis() - gStartMs;
    const uint32_t elapsedSeconds = elapsedMs / 1000u;

    // Liveness: a failed `i2s_driver_install` or `i2s_set_pin` makes the
    // backend return before creating its task, and every counter then reads zero
    // forever. Reporting `underruns=0` in that state would be a false pass, so
    // the run dies loudly instead.
    static bool aliveChecked = false;
    if (!aliveChecked && elapsedSeconds >= 5u) {
        aliveChecked = true;
        if (gBackend.getWorstBlockUs() == 0u) {
            Serial.printf("ERROR backend never iterated: i2s init failed? pins=%d/%d/%d\n",
                          kI2sBclk, kI2sLrck, kI2sDout);
            Serial.flush();
            while (true) {
                delay(1000);
            }
        }
    }

    if (elapsedSeconds >= static_cast<uint32_t>(RUN_SECONDS)) {
        printReport("final", elapsedSeconds);
        while (true) {
            delay(1000);
        }
    }

    if (elapsedSeconds >= gLastPrintSeconds + 30u) {
        gLastPrintSeconds = elapsedSeconds;
        printReport("progress", elapsedSeconds);
    }

    delay(100);
}
