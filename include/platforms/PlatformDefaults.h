/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#pragma once

// =============================================================================
// Target-dependent feature defaults
// =============================================================================
// This header is the single place for defaults that depend on the build target
// (classic ESP32, ESP32-S3, native, etc.). Optional drivers/backends that are
// only available on certain targets get their enable defines set here by default.

// -----------------------------------------------------------------------------
// Core Affinity Defaults
// -----------------------------------------------------------------------------
// Default core for audio processing tasks (pinned to Core 0 by default on ESP32)
#ifndef PR32_DEFAULT_AUDIO_CORE
#define PR32_DEFAULT_AUDIO_CORE 0
#endif

// Default core for the main engine loop (pinned to Core 1 by default on ESP32)
#ifndef PR32_DEFAULT_MAIN_CORE
#define PR32_DEFAULT_MAIN_CORE 1
#endif

// -----------------------------------------------------------------------------
// Display Driver Selection
// -----------------------------------------------------------------------------
// Support for multiple display drivers. TFT_eSPI is the default for ESP32.
// Future support for U8G2 can be enabled via PIXELROOT32_USE_U8G2.
#if defined(ARDUINO_ARCH_ESP32)
    #if defined(PIXELROOT32_USE_U8G2)
        #define PIXELROOT32_USE_U8G2_DRIVER 1
    #else
        #ifndef PIXELROOT32_NO_TFT_ESPI
            #define PIXELROOT32_USE_TFT_ESPI_DRIVER 1
        #endif
    #endif
#endif

// -----------------------------------------------------------------------------
// Audio Backend Selection
// -----------------------------------------------------------------------------

// ESP32 DAC audio backend (classic ESP32 only)
// Internal DAC (GPIO 25/26) exists only on classic ESP32.
// Override: -D PIXELROOT32_NO_DAC_AUDIO to disable on classic ESP32.
#if defined(ARDUINO_ARCH_ESP32) && defined(CONFIG_IDF_TARGET_ESP32) && !defined(PIXELROOT32_NO_DAC_AUDIO)
#define PIXELROOT32_USE_DAC_AUDIO 1
#endif

// ESP32 I2S audio backend (Available on most ESP32 variants)
// Override: -D PIXELROOT32_NO_I2S_AUDIO to disable.
#if defined(ARDUINO_ARCH_ESP32) && !defined(PIXELROOT32_NO_I2S_AUDIO)
#define PIXELROOT32_USE_I2S_AUDIO 1
#endif
