/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#ifndef MOCK_ARDUINO_H
#define MOCK_ARDUINO_H

#include "platforms/PlatformSelect.h"

// The mock is genuinely SDL-backed -- millis() is SDL_GetTicks(), delay() is
// SDL_Delay() -- so it is gated on the SDL capability and not on a host. Gating
// it on PLATFORM_NATIVE left the web build including <Arduino.h> instead, which
// no host toolchain provides. EngineConfig reaches it only when the Arduino core
// is absent, which is the caller's question, not this file's.
#if PIXELROOT32_HAS_SDL

// SDL_MAIN_HANDLED is defined in platformio.ini build_flags
#include <SDL2/SDL.h>
#include <cstdint>
#include <cstdio>
#include <cstring>

/**
 * @file MockArduino.h
 * @brief Mocks Arduino core functions for native platform (PC/SDL2).
 *
 * This file allows code written for Arduino to compile and run on desktop
 * by providing compatible signatures for common functions like millis(),
 * delay(), and pin operations.
 */

#define HIGH 1
#define LOW 0
#define INPUT 0
#define INPUT_PULLUP 1
#define OUTPUT 1

// Pin modes (not used in mock, but needed for compilation)
inline void pinMode(uint8_t pin, uint8_t mode) {
    (void)pin;
    (void)mode;
    // No-op in mock
}

// Digital read (not used in mock, but needed for compilation)
inline int digitalRead(uint8_t* pin) {
    (void)pin;
    return pin == SDL_GetKeyboardState(nullptr); // Default to HIGH (pullup)
}

// Digital write (not used in mock, but needed for compilation)
inline void digitalWrite(uint8_t pin, uint8_t value) {
    (void)pin;
    (void)value;
    // No-op in mock
}

// Analog read (not used in mock, but needed for compilation)
inline int analogRead(uint8_t pin) {
    (void)pin;
    return 0;
}

// Time functions
inline uint32_t millis() {
    return SDL_GetTicks();
}

inline uint32_t micros() {
    return SDL_GetTicks() * 1000;
}

inline void delay(uint32_t ms) {
    SDL_Delay(ms);
}

inline void delayMicroseconds(uint32_t us) {
    SDL_Delay((us + 999) / 1000); // Approximate
}

#endif // PIXELROOT32_HAS_SDL

#endif // MOCK_ARDUINO_H
