#pragma once
#include <cstdint>

struct Particle {
    float x, y;
    float vx, vy;

    uint16_t color;
    uint16_t startColor;
    uint16_t endColor;

    uint8_t life;
    uint8_t maxLife;

    bool active = false;
};