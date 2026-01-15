#pragma once
#include <cstdint>
#include "Color.h"

namespace pixelroot32::graphics::particles {
/**
 * @struct Particle
 * @brief Represents a single particle in the particle system.
 *
 * Designed to be lightweight to fit many instances in memory (RAM optimization).
 */
struct Particle {
    float x, y;     ///< Current position.
    float vx, vy;   ///< Velocity vector.

    uint16_t color;      ///< Current color (RGB565).
    Color startColor; ///< Initial color for interpolation.
    Color endColor;   ///< Final color for interpolation.

    uint8_t life;    ///< Current remaining life (frames or ticks).
    uint8_t maxLife; ///< Total life duration.

    bool active = false; ///< Whether the particle is currently in use.
};

}