#pragma once
#include "platforms/EngineConfig.h"

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

}

