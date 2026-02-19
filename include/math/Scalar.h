#pragma once
#include <type_traits>
#include <cmath>
#include "platforms/EngineConfig.h"
#include "Fixed16.h"

namespace pixelroot32::math {

// Select numeric type based on hardware capabilities defined in EngineConfig
using Scalar = std::conditional_t<
    platforms::config::HasFPU,
    float,
    Fixed16
>;

static constexpr bool USE_FIXED_POINT = !platforms::config::HasFPU;

// Helper to convert numeric types to Scalar
template<typename T>
constexpr Scalar toScalar(T val) {
    if constexpr (std::is_same_v<Scalar, float>) {
        return static_cast<float>(val);
    } else {
        if constexpr (std::is_integral_v<T>) {
            return Scalar(static_cast<int>(val));
        } else {
            return Scalar(val);
        }
    }
}

} // namespace pixelroot32::math