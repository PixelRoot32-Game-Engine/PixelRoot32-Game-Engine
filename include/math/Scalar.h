/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 *
 * @file Scalar.h
 * @brief Defines the Scalar type and conversion utilities.
 */
#pragma once
#include <type_traits>
#include <cmath>
#include "platforms/EngineConfig.h"
#include "Fixed16.h"

namespace pixelroot32::math {

/**
 * @typedef Scalar
 * @brief The underlying numeric type used for physics and math (float or Fixed16).
 */
using Scalar = std::conditional_t<
    platforms::config::HasFPU,
    float,
    Fixed16
>;

static constexpr bool USE_FIXED_POINT = !platforms::config::HasFPU;

/**
 * @brief Helper to convert numeric types to Scalar.
 * @param val The value to convert.
 * @return The converted value.
 */
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