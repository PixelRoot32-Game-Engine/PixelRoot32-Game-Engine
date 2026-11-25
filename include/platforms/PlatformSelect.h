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
 * - `PIXELROOT32_HAS_ARDUINO`      the Arduino API is available to this build
 * - `PIXELROOT32_HAS_HOST_FS`      a POSIX-shaped file system is available
 *
 * SDL2 is present on desktop and on the web (Emscripten ships an SDL2 port), so
 * a driver written against SDL2 belongs to the capability and not to the host.
 * The Arduino API is the opposite case: it is present on the board and absent
 * everywhere else, so the SPI stub and the millis()/delay() mock belong to its
 * absence and not to the host either.
 *
 * Code that asks "may I use the SDL window?" must test `PIXELROOT32_HAS_SDL`;
 * code that asks "may I call the Arduino API?" must test
 * `PIXELROOT32_HAS_ARDUINO`; code that asks "am I on the board?" must test an
 * identity macro. Testing `PLATFORM_NATIVE` for any of these questions is what
 * this header exists to end -- and using one capability as a stand-in for
 * another is the same mistake with a newer name.
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

// A file system with fopen()/stat()/mkdir()/rename() and paths: desktop, and
// the browser through Emscripten's virtual FS. `HostFileKeyValueStore` asks
// this one, because a file adapter is not a PC feature -- it needs a file
// system and nothing else. Durability differs by host (rename() is atomic on a
// POSIX disk, and on the web the mount has to be flushed to IndexedDB with
// FS.syncfs()), which is what WebStorage.h covers.
#if !defined(PIXELROOT32_HAS_HOST_FS)
    #if PIXELROOT32_PLATFORM_DESKTOP || PIXELROOT32_PLATFORM_WEB
        #define PIXELROOT32_HAS_HOST_FS 1
    #else
        #define PIXELROOT32_HAS_HOST_FS 0
    #endif
#endif

// Probed, not inferred: `ARDUINO` is what the Arduino framework itself defines
// when it provides the core, so this answers "is <Arduino.h> really there?"
// instead of "is this a board?". The SPI stub and the Arduino mock ask this one:
// defining a global `SPI` next to the real core would clash, and a build with no
// core and no SDL2 has no millis() to mock.
#if !defined(PIXELROOT32_HAS_ARDUINO)
    #if defined(ARDUINO)
        #define PIXELROOT32_HAS_ARDUINO 1
    #else
        #define PIXELROOT32_HAS_ARDUINO 0
    #endif
#endif
