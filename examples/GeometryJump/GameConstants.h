#pragma once
#include <cstdint>

namespace geometryjump {

    constexpr std::uint8_t BTN_JUMP = 0;

    constexpr int PLAYER_WIDTH = 16;
    constexpr int PLAYER_HEIGHT = 16;

    constexpr float PLAYER_SPEED_X = 80.0f;

    constexpr float GRAVITY = 400.0f;
    constexpr float JUMP_FORCE = 220.0f;
    constexpr float MAX_FALL_SPEED = 500.0f;

    constexpr float SCROLL_SPEED = 80.0f;
    constexpr float MAX_SCROLL_SPEED = 180.0f;
    constexpr float SPEED_INCREMENT = 2.0f;

    constexpr int OBSTACLE_WIDTH = 16;
    constexpr int OBSTACLE_HEIGHT = 24;
    constexpr int OBSTACLE_GROUND_OFFSET = 0;

    constexpr int GROUND_HEIGHT = 40;

    constexpr int MAX_OBSTACLES = 8;

    struct ObstacleDefinition {
        float distance;
        int heightOffset;
        int width;
        int height;
    };

    inline constexpr ObstacleDefinition LEVEL1_OBSTACLES[] = {
        { 120.0f, 0, 16, 16 },
        { 220.0f, 0, 20, 28 }, // Taller
        { 320.0f, 0, 24, 20 },
        { 420.0f, 0, 18, 18 },
        { 520.0f, 0, 28, 16 },
        { 620.0f, 0, 16, 32 }, // Taller
        { 720.0f, 0, 22, 20 },
        { 820.0f, 0, 14, 18 },
        { 920.0f, 0, 26, 36 }, // Much taller
        { 1020.0f, 0, 18, 24 },
        { 1120.0f, 0, 20, 18 },
        { 1220.0f, 0, 24, 16 },
        { 1320.0f, 0, 16, 20 },
        { 1420.0f, 0, 22, 40 }, // Maximum safe height
        { 1520.0f, 0, 28, 18 },
        { 1620.0f, 0, 18, 30 } // Taller
    };

    constexpr int LEVEL1_OBSTACLE_COUNT = 16;
    constexpr float LEVEL1_TOTAL_DISTANCE = 1800.0f;
    
}
