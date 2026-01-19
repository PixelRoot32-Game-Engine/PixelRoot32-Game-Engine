/*
 * Copyright (c) 2026 Gabriel Perez
 * Licensed under the GNU GPL v3
 */
#ifndef MOCK_ARDUINO_H
#define MOCK_ARDUINO_H

#ifdef PLATFORM_NATIVE

// Tell SDL2 not to replace main() with SDL_main/WinMain
#define SDL_MAIN_HANDLED
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

/**
 * @class SerialClass
 * @brief Mocks the Arduino Serial object, redirecting output to stdout.
 */
class SerialClass {
public:
    void begin(uint32_t baud) {
        (void)baud;
        // No-op, stdout is always available
    }
    
    void print(const char* str) {
        printf("%s", str);
    }
    
    void print(int value) {
        printf("%d", value);
    }
    
    void print(uint32_t value) {
        printf("%u", value);
    }
    
    void print(float value) {
        printf("%f", value);
    }
    
    void println(const char* str) {
        printf("%s\n", str);
    }
    
    void println(int value) {
        printf("%d\n", value);
    }
    
    void println(uint32_t value) {
        printf("%u\n", value);
    }
    
    void println(float value) {
        printf("%f\n", value);
    }
};

// Global instance to match Arduino's Serial
extern SerialClass Serial;

#endif // PLATFORM_NATIVE

#endif // MOCK_ARDUINO_H
