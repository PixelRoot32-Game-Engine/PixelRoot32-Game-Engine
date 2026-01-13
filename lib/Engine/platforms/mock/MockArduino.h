#ifndef MOCK_ARDUINO_H
#define MOCK_ARDUINO_H

#ifdef PLATFORM_NATIVE

// Tell SDL2 not to replace main() with SDL_main/WinMain
#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <cstdint>
#include <cstdio>
#include <cstring>

// Mock Arduino.h for native platform

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

// Serial mock (outputs to stdout)
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
    
    void println() {
        printf("\n");
    }
    
    int available() {
        return 0; // No serial input in mock
    }
    
    int read() {
        return -1; // No serial input in mock
    }
};

// Global Serial instance (declared here, defined in MockArduino.cpp)
extern SerialClass Serial;

// Random functions
inline void randomSeed(uint32_t seed) {
    srand(seed);
}

inline long random(long min, long max) {
    if (min >= max) return min;
    return min + (rand() % (max - min));
}

inline long random(long max) {
    return random(0, max);
}

// ESP32 specific (mock)
inline uint32_t esp_random() {
    return (uint32_t)rand();
}

// Math functions (usually in math.h, but Arduino.h includes them)
#include <cmath>
#include <cstdlib>

#endif // PLATFORM_NATIVE

#endif // MOCK_ARDUINO_H
