/*
 * Original work:
 * Copyright (c) nbourre
 * Licensed under the MIT License
 *
 * Modifications:
 * Copyright (c) 2026 PixelRoot32
 *
 * This file remains licensed under the MIT License.
 */
#pragma once
#include <array>
#include <cstdint>
#include <type_traits>

namespace pixelroot32::input {

// Forward declaration for static_assert validation
struct InputManager;

/**
 * @struct InputConfig
 * @brief Configuration structure for the InputManager.
 *
 * Maps logical inputs to physical GPIO pins (ESP32) or keyboard scancodes (Native/SDL2).
 * Uses fixed-size std::array instead of std::vector for zero-allocation and
 * deterministic memory usage on ESP32.
 *
 * Usage:
 * @code
 * // ESP32
 * pr32::input::InputConfig config(BTN_UP, BTN_DOWN, BTN_LEFT, BTN_RIGHT);
 *
 * // Native (SDL2)
 * pr32::input::InputConfig config(SDL_SCANCODE_UP, SDL_SCANCODE_DOWN);
 *
 * // Empty
 * pr32::input::InputConfig config{};
 * @endcode
 *
 * @see InputManager
 */
struct InputConfig {
    /// Maximum number of inputs supported (compile-time fixed size).
    static constexpr size_t MAX_INPUT_COUNT = 16;

    static_assert(MAX_INPUT_COUNT <= 16,
        "MAX_INPUT_COUNT must not exceed compile-time limits");

#ifdef PLATFORM_NATIVE
    /// Array of SDL scancodes for Native platform.
    std::array<uint8_t, MAX_INPUT_COUNT> buttonNames{};
#else
    /// Array of GPIO pin numbers for ESP32.
    std::array<int, MAX_INPUT_COUNT> inputPins{};
#endif

    /// Total number of configured inputs (auto-deduced from constructor arguments).
    size_t count = 0;

    /**
     * @brief Template variadic constructor.
     * @tparam Args Types of input arguments (int for pins/scancodes).
     * @param args Variable number of input values.
     *
     * Count is auto-deduced from argument count. Exceeding MAX_INPUT_COUNT
     * triggers a static_assert compile error.
     */
    template<typename... Args>
    InputConfig(Args... args) : count(sizeof...(Args)) {
        static_assert(sizeof...(Args) <= MAX_INPUT_COUNT,
            "Too many arguments for InputConfig");

        if (count > MAX_INPUT_COUNT) {
            count = MAX_INPUT_COUNT;
        }

        if constexpr (sizeof...(args) > 0) {
            size_t index = 0;
#ifdef PLATFORM_NATIVE
            ((buttonNames[index++] = static_cast<uint8_t>(args)), ...);
#else
            ((inputPins[index++] = static_cast<int>(args)), ...);
#endif
        }
    }

    /// @brief Default constructor (empty configuration).
    InputConfig() : count(0) {}
};

} // namespace pixelroot32::input