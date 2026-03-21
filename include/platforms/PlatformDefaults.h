/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#pragma once

#if defined(ARDUINO_ARCH_ESP32)
#include <sdkconfig.h>
#endif

// =============================================================================
// Feature Defaults
// =============================================================================
// The following defines enable or disable optional features in the engine.
// By default, all features are enabled.

// -----------------------------------------------------------------------------
// Audio Feature Defaults
// -----------------------------------------------------------------------------
// By default, the audio system is enabled.
#ifndef PIXELROOT32_ENABLE_AUDIO
#define PIXELROOT32_ENABLE_AUDIO 1
#endif

// -----------------------------------------------------------------------------
// Physics Feature Defaults
// -----------------------------------------------------------------------------
// By default, the physics system is enabled.
#ifndef PIXELROOT32_ENABLE_PHYSICS
#define PIXELROOT32_ENABLE_PHYSICS 1
#endif

// -----------------------------------------------------------------------------
// UI System Feature Defaults
// -----------------------------------------------------------------------------
// By default, the UI system is enabled.
#ifndef PIXELROOT32_ENABLE_UI_SYSTEM
#define PIXELROOT32_ENABLE_UI_SYSTEM 1
#endif

// -----------------------------------------------------------------------------
// Particle System Feature Defaults
// -----------------------------------------------------------------------------   
// By default, the particle system is enabled.
#ifndef PIXELROOT32_ENABLE_PARTICLES
#define PIXELROOT32_ENABLE_PARTICLES 1
#endif

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
// U8G2 support can be enabled via PIXELROOT32_USE_U8G2.
#if defined(ARDUINO_ARCH_ESP32)
    #if defined(PIXELROOT32_USE_U8G2)
        // U8G2 active, we usually disable TFT_eSPI to save space
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
// NOT available on: ESP32-S3, ESP32-C3, ESP32-S2, ESP32-C6
// Override: -D PIXELROOT32_NO_DAC_AUDIO to disable on classic ESP32.
#if defined(ARDUINO_ARCH_ESP32) && defined(CONFIG_IDF_TARGET_ESP32) && !defined(PIXELROOT32_NO_DAC_AUDIO)
#define PIXELROOT32_USE_DAC_AUDIO 1
#endif

// ESP32 I2S audio backend (Available on most ESP32 variants)
// Available on: ESP32 Classic, ESP32-S3, ESP32-C3, ESP32-S2, ESP32-C6
// Override: -D PIXELROOT32_NO_I2S_AUDIO to disable.

// =============================================================================
// Platform-Specific Notes
// =============================================================================
// FPU (Floating Point Unit) Availability:
// - Available: ESP32 Classic, ESP32-S3, Native platforms
// - Not Available: ESP32-C3, ESP32-S2, ESP32-C6 (uses Fixed16 automatically)
//
// When FPU is not available, the engine automatically uses Fixed16 math
// instead of float for the Scalar type, providing ~30% performance improvement
// over software floating-point emulation.
#if defined(ARDUINO_ARCH_ESP32) && !defined(PIXELROOT32_NO_I2S_AUDIO)
#define PIXELROOT32_USE_I2S_AUDIO 1
#endif