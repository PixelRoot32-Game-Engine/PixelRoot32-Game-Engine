#pragma once
#include <stdint.h>

namespace Math {

    constexpr float kPi = 3.14159265f;
    constexpr float kDegToRad = 180.0f / kPi;
    constexpr float kRadToDeg = kPi / 180.0f;

    inline float lerp(float a, float b, float t) {
        return a + (b - a) * t;
    }

    inline float clamp(float v, float min, float max) {
        return (v < min) ? min : (v > max ? max : v);
    }
}
