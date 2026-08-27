#pragma once
#include <cstdint>
#include <audio/AudioTypes.h>
#include <audio/AudioMusicTypes.h>

/**
 * @file GameConstants.h
 * @brief Central configuration for gameplay tuning
 *
 * Contains:
 * - Input mappings
 * - Entity dimensions
 * - NES-style-ish SFX (pulse duty / sweeps + noise hits)
 */

namespace brickbreaker {

    /** Input button IDs */
    constexpr uint8_t BTN_START = 4;
    constexpr uint8_t BTN_RIGHT = 3;
    constexpr uint8_t BTN_LEFT = 2;

    /** Entity dimensions and layout */
    constexpr int BRICK_WIDTH = 30;
    constexpr int BRICK_HEIGHT = 12;
    constexpr int PADDLE_W = 40;
    constexpr int PADDLE_H = 8;
    constexpr int BALL_SIZE = 6;
    constexpr float BORDER_TOP = 20.0f;

    /** Sound effects — design: narrow pulse for paddle, thicker for wall, layered brick, sweeps for life/start */
    namespace sfx {
        inline const pixelroot32::audio::AudioEvent PADDLE_HIT = {
            pixelroot32::audio::WaveType::PULSE, 520.0f, 0.095f, 0.5f, 0.125f};

        inline const pixelroot32::audio::AudioEvent WALL_HIT = {
            pixelroot32::audio::WaveType::PULSE, 205.0f, 0.12f, 0.48f, 0.5f};

        /** Noise “crumble” overlay. */
        inline const pixelroot32::audio::AudioEvent BRICK_CRACK = {
            pixelroot32::audio::WaveType::NOISE, 900.0f, 0.05f, 0.42f, 0.5f,
            58u, &pixelroot32::audio::INSTR_SNARE};

        inline const pixelroot32::audio::AudioEvent LIFE_LOST = {
            pixelroot32::audio::WaveType::PULSE, 290.0f, 0.4f, 0.54f, 0.5f,
            0u, nullptr, 95.0f, 0.14f};

        inline const pixelroot32::audio::AudioEvent START_GAME = {
            pixelroot32::audio::WaveType::PULSE, 1500.0f, 0.18f, 0.45f, 0.5f,
            0u, &pixelroot32::audio::INSTR_PULSE_LEAD, 420.0f, 0.08f};
    }

}
