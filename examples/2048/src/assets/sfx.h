/**
 * @file sfx.h
 * @brief Audio SFX for 2048 game
 */

#pragma once

#if PIXELROOT32_ENABLE_AUDIO
#include <audio/AudioEngine.h>
namespace audio = pixelroot32::audio;
#endif

namespace game2048 {

#if PIXELROOT32_ENABLE_AUDIO

/**
 * @brief Triangle wave - played when a new tile spawns in empty space
 */
inline static void playSpawnSound(audio::AudioEngine& audioEngine) {
    audio::AudioEvent ev{};
    ev.type = audio::WaveType::TRIANGLE;
    ev.frequency = 440.0f;  // A4
    ev.duration = 0.08f;
    ev.volume = 0.35f;
    audioEngine.playEvent(ev);
}

/**
 * @brief Double triangle wave - played when tiles merge
 */
inline static void playMergeSound(audio::AudioEngine& audioEngine) {
    audio::AudioEvent ev{};
    ev.type = audio::WaveType::TRIANGLE;
    ev.frequency = 523.25f;  // C5
    ev.duration = 0.12f;
    ev.volume = 0.4f;
    audioEngine.playEvent(ev);
    // Second triangle slightly higher
    ev.frequency = 659.25f;  // E5
    ev.duration = 0.1f;
    audioEngine.playEvent(ev);
}

/**
 * @brief Simple triangle wave - played when moving without merge
 */
inline static void playMoveSound(audio::AudioEngine& audioEngine) {
    audio::AudioEvent ev{};
    ev.type = audio::WaveType::TRIANGLE;
    ev.frequency = 220.0f;  // A3
    ev.duration = 0.05f;
    ev.volume = 0.25f;
    audioEngine.playEvent(ev);
}

/**
 * @brief Noise-based game over sound
 */
inline static void playGameOverSound(audio::AudioEngine& audioEngine) {
    audio::AudioEvent ev{};
    ev.type = audio::WaveType::NOISE;
    ev.preset = &audio::INSTR_SNARE;
    ev.frequency = 120.0f;
    ev.duration = 0.4f;
    ev.volume = 0.5f;
    ev.duty = 0.5f;
    audioEngine.playEvent(ev);
}

/**
 * @brief Arpeggio win sound
 */
inline static void playWinSound(audio::AudioEngine& audioEngine) {
    audio::AudioEvent ev{};
    ev.type = audio::WaveType::PULSE;
    ev.frequency = 523.25f;  // C5
    ev.duration = 0.12f;
    ev.volume = 0.5f;
    ev.duty = 0.5f;
    audioEngine.playEvent(ev);
    // Quick arpeggio up
    ev.frequency = 659.25f;  // E5
    ev.duration = 0.12f;
    audioEngine.playEvent(ev);
    ev.frequency = 783.99f;  // G5
    ev.duration = 0.12f;
    audioEngine.playEvent(ev);
}

#endif // PIXELROOT32_ENABLE_AUDIO

} // namespace game2048