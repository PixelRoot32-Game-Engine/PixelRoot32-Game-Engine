#pragma once
#include <cstdint>

struct ParticleConfig {
    uint16_t startColor;
    uint16_t endColor;

    float minSpeed;
    float maxSpeed;

    float gravity;
    float friction;

    uint8_t minLife;
    uint8_t maxLife;

    bool fadeColor;
};
