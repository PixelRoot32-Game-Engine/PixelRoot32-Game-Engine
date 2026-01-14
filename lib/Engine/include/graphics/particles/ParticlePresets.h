#pragma once
#include "ParticleConfig.h"

/**
 * @namespace ParticlePresets
 * @brief Predefined configurations for common particle effects.
 *
 * Provides ready-to-use constants for effects like Fire, Explosion, Smoke, etc.
 */
namespace ParticlePresets {

    /**
     * @brief Fire effect configuration.
     * Red/Orange colors, rising upward (negative gravity), moderate life.
     */
    constexpr ParticleConfig Fire {
        0xF800, 0x7800,
        0.5f, 1.5f,
        -0.02f,
        0.98f,
        20, 40,
        true
    };

    /**
     * @brief Explosion effect configuration.
     * Yellow to Red, high initial speed, expanding outward.
     */
    constexpr ParticleConfig Explosion {
        0xFFE0, 0x0000,
        2.0f, 4.0f,
        0.1f,
        0.90f,
        10, 20,
        true
    };

    /**
     * @brief Sparks effect configuration.
     * White to Yellow, quick bursts, gravity affects them.
     */
    constexpr ParticleConfig Sparks {
        0xFFFF, 0xFFE0,
        1.5f, 3.0f,
        0.15f,
        0.85f,    
        8, 15,
        true
    };

    /**
     * @brief Smoke effect configuration.
     * Grey, slow rising, long life.
     */
    constexpr ParticleConfig Smoke {
        0x7BEF, 0x0000,
        0.2f, 0.6f,
        -0.01f,
        0.97f,
        40, 80,
        true
    };

    /**
     * @brief Dust effect configuration.
     * Light grey, subtle movement.
     */
    constexpr ParticleConfig Dust {
        0xC618, 0x7BEF,
        0.3f, 1.0f,
        0.08f,
        0.90f,
        12, 25,
        true
    };
}
