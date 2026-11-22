/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#pragma once

/**
 * @file PlatformSelect.h
 * @brief Separates *which machine this is built for* from *what that build can use*.
 *
 * `PLATFORM_NATIVE` used to answer both questions, which is why a web build had
 * no answer at all: Emscripten defines `__EMSCRIPTEN__`, never `PLATFORM_NATIVE`,
 * so every host guard evaluated false and the engine compiled as if it were an
 * ESP32 without a display driver. The two questions are now asked separately.
 *
 * ## Identity — one of these is 1
 *
 * - `PIXELROOT32_PLATFORM_ESP32`   firmware on the board
 * - `PIXELROOT32_PLATFORM_DESKTOP` native executable (Windows, macOS, Linux)
 * - `PIXELROOT32_PLATFORM_WEB`     Emscripten / WebAssembly
 *
 * ## Capability — independent of identity
 *
 * - `PIXELROOT32_HAS_SDL`          SDL2 is available to this build
 *
 * SDL2 is present on desktop and on the web (Emscripten ships an SDL2 port), so
 * a driver written against SDL2 belongs to the capability and not to the host.
 * Code that asks "may I use the SDL window?" must test `PIXELROOT32_HAS_SDL`;
 * code that asks "am I on the board?" must test an identity macro. Testing
 * `PLATFORM_NATIVE` for either question is what this header exists to end.
 *
 * `PLATFORM_NATIVE` stays defined by `platformio.ini [base_native]` for one
 * release as the legacy spelling of the desktop identity, so existing user
 * projects keep building unchanged.
 *
 * Every macro can be overridden from the build command line.
 */

// =============================================================================
// Identity
// =============================================================================

#if !defined(PIXELROOT32_PLATFORM_WEB)
    #if defined(__EMSCRIPTEN__)
        #define PIXELROOT32_PLATFORM_WEB 1
    #else
        #define PIXELROOT32_PLATFORM_WEB 0
    #endif
#endif

#if !defined(PIXELROOT32_PLATFORM_ESP32)
    #if defined(ARDUINO_ARCH_ESP32) || defined(ESP32)
        #define PIXELROOT32_PLATFORM_ESP32 1
    #else
        #define PIXELROOT32_PLATFORM_ESP32 0
    #endif
#endif

#if !defined(PIXELROOT32_PLATFORM_DESKTOP)
    #if !PIXELROOT32_PLATFORM_ESP32 && !PIXELROOT32_PLATFORM_WEB && defined(PLATFORM_NATIVE)
        #define PIXELROOT32_PLATFORM_DESKTOP 1
    #else
        #define PIXELROOT32_PLATFORM_DESKTOP 0
    #endif
#endif

// =============================================================================
// Capability
// =============================================================================

#if !defined(PIXELROOT32_HAS_SDL)
    #if PIXELROOT32_PLATFORM_DESKTOP || PIXELROOT32_PLATFORM_WEB
        #define PIXELROOT32_HAS_SDL 1
    #else
        #define PIXELROOT32_HAS_SDL 0
    #endif
#endif
