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
// Projects can override via build flags (e.g. -D PIXELROOT32_NO_DAC_AUDIO).

// -----------------------------------------------------------------------------
// ESP32 DAC audio backend (classic ESP32 only)
// -----------------------------------------------------------------------------
// Internal DAC (GPIO 25/26) exists only on classic ESP32. Set by default for
// that target so existing projects keep working. For ESP32-S3 use
// ESP32_I2S_AudioBackend and do not define PIXELROOT32_USE_DAC_AUDIO.
// Override: -D PIXELROOT32_NO_DAC_AUDIO to disable on classic ESP32.
#if defined(ARDUINO_ARCH_ESP32) && defined(CONFIG_IDF_TARGET_ESP32) && !defined(PIXELROOT32_NO_DAC_AUDIO)
#define PIXELROOT32_USE_DAC_AUDIO 1
#endif
