#pragma once
#include "platforms/EngineConfig.h"

#include <cstdint>

namespace camerademo {

/**
 * @file GameConstants.h
 * @brief Camera demo configuration.
 */
constexpr int TILE_SIZE = 8;
constexpr int TILEMAP_WIDTH = (DISPLAY_WIDTH * 3) / TILE_SIZE;
constexpr int TILEMAP_HEIGHT = DISPLAY_HEIGHT / TILE_SIZE;
constexpr int PLATFORM_COUNT = 3;
constexpr int PLATFORM_VISUAL_OFFSET = 2;

constexpr float PLAYER_GRAVITY = 400.0f;
constexpr float PLAYER_MOVE_SPEED = 90.0f;
constexpr float PLAYER_JUMP_VELOCITY = 220.0f;

constexpr float PLAYER_WIDTH = 16.0f;
constexpr float PLAYER_HEIGHT = 16.0f;

constexpr float PLAYER_START_X = 20.0f;
constexpr float PLAYER_START_Y =
    static_cast<float>((TILEMAP_HEIGHT - 8) * TILE_SIZE) - PLAYER_HEIGHT;

// --- Input mapping -------------------------------------------------------
// Six buttons are configured by the platform headers:
//   0 Up, 1 Down, 2 Left, 3 Right, 4 A (jump), 5 B.
constexpr uint8_t BTN_LEFT = 2;
constexpr uint8_t BTN_RIGHT = 3;
constexpr uint8_t BTN_JUMP = 4;
constexpr uint8_t BTN_EFFECT = 5;   ///< Cycles through the camera effects.
constexpr uint8_t BTN_TWEEN = 0;    ///< Pans the camera to the far platform.
constexpr uint8_t BTN_CANCEL = 1;   ///< Cancels every active camera effect.

// --- Camera effect presets ----------------------------------------------
// Amplitudes are in logical pixels, durations in milliseconds.
constexpr int SHAKE_AMPLITUDE = 8;
constexpr unsigned long SHAKE_DURATION_MS = 2000u;
constexpr int PUNCH_AMPLITUDE = 6;
constexpr unsigned long PUNCH_DURATION_MS = 1500u;
constexpr int OFFSET_AMPLITUDE = 4;
constexpr unsigned long OFFSET_DURATION_MS = 2000u;

/// Punch fired automatically when the player lands after a fall.
constexpr int LANDING_PUNCH_AMPLITUDE = 3;
constexpr unsigned long LANDING_PUNCH_DURATION_MS = 200u;

// --- Camera tween --------------------------------------------------------
/// Tween slots. Two are enough for the "pan out, pan back" round trip.
constexpr int CAMERA_TWEEN_SLOTS = 2;
constexpr unsigned short TWEEN_DURATION_MS = 700u;
/// Tile column the tween pans to — the third platform, off screen at spawn.
constexpr int TWEEN_TARGET_TILE_X = 50;
/// How long the camera holds on the target before panning back, in ms.
constexpr unsigned long TWEEN_HOLD_MS = 600u;

}

