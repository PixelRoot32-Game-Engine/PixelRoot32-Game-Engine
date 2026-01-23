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
#include <cstdarg>
#include <cstdint>

namespace pixelroot32::input {

/**
 * @struct InputConfig
 * @brief Configuration structure for the InputManager.
 *
 * Defines the mapping between logical inputs and physical pins (ESP32) 
 * or keyboard keys (Native/SDL2).
 *
 * Uses variadic arguments to allow flexible configuration of input count.
 */
struct InputConfig{
#ifdef PLATFORM_NATIVE
    uint8_t* buttonNames = nullptr; ///< Array of button mappings (scancodes) for Native.
#else
    int* inputPins = nullptr ; ///< Array of GPIO pin numbers for ESP32.
#endif
    int count = 0;         ///< Total number of configured inputs.

    /**
     * @brief Constructs a new InputConfig.
     * @param count Number of inputs to configure.
     * @param ... Variable arguments list of pins (int) or keys.
     */
    InputConfig(int count, ...): count(count) {
        if (count <= 0) {
            this->count = 0;
            #ifdef PLATFORM_NATIVE
                buttonNames = nullptr;
            #else
                inputPins = nullptr;
            #endif
            return;
        }

        va_list args;
        va_start(args, count);

        #ifdef PLATFORM_NATIVE
            buttonNames = new uint8_t[count];
            for (int i = 0; i < count; i++) {
                buttonNames[i] = (uint8_t)va_arg(args, int);
            }
        #else
            inputPins = new int[count];
            for (int i = 0; i < count; i++) {
                inputPins[i] = va_arg(args, int);
            }
        #endif
        va_end(args);
    }
};

}