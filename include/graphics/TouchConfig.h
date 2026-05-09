/*
 * PixelRoot32 Game Engine
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 *
 * TouchConfig - Touch controller configuration
 * Extends DisplayConfig with touch-specific settings
 */
#pragma once

#include <cstdint>

namespace pixelroot32::graphics {

/**
 * @file TouchConfig.h
 * @brief Touch controller configuration for XPT2046 (SPI) and GT911 (I2C).
 *
 * Defines TouchConfig as a simple struct with calibration, SPI/I2C settings,
 * and coordinate transformation (scale + offset). Add to DisplayConfig
 * or use standalone.
 *
 * Compile-time activation:
 *   -DTOUCH_DRIVER_XPT2046  → XPT2046 SPI at 2.5 MHz default
 *   -DTOUCH_DRIVER_GT911    → GT911 I2C at 400 kHz default
 */

/**
 * @enum TouchController
 * @brief Supported touch controller types.
 */
enum class TouchController {
    None = 0,    ///< No touch hardware.
    XPT2046 = 1, ///< SPI resistive controller (common on ESP32 dev boards).
    GT911 = 2    ///< I2C capacitive controller (Goodix, up to 5-point touch).
};

/**
 * @struct TouchConfig
 * @brief Configuration for a touch controller (XPT2046 or GT911).
 *
 * Set controller, communication parameters, and calibration transform.
 * Coordinate mapping: screenX = rawX * scaleX + offsetX (and same for Y).
 * Raw coordinates outside display bounds are clamped before mapping.
 */
struct TouchConfig {
    TouchController controller = TouchController::None;  ///< Active controller type.

    // SPI settings (XPT2046)
    /** SPI clock frequency in Hz (default 2.5 MHz). */
    uint32_t spiClockHz = 2500000;
    /** SPI chip-select pin. */
    uint8_t csPin = 5;
    /** Touch interrupt pin (255 = unused). */
    uint8_t irqPin = 4;

    // I2C settings (GT911)
    /** I2C clock frequency in Hz (default 400 kHz). */
    uint32_t i2cClockHz = 400000;
    /** GT911 I2C address (default 0x5D; alternate 0x14). */
    uint8_t i2cAddress = 0x5D;

    // Calibration transform
    /** Horizontal scale (raw → display coordinate multiplier). */
    float scaleX = 1.0f;
    /** Vertical scale (raw → display coordinate multiplier). */
    float scaleY = 1.0f;
    /** Horizontal offset in display pixels after scaling. */
    int16_t offsetX = 0;
    /** Vertical offset in display pixels after scaling. */
    int16_t offsetY = 0;

    // Display bounds for coordinate clamping
    /** Physical display width in pixels. */
    uint16_t displayWidth = 320;
    /** Physical display height in pixels. */
    uint16_t displayHeight = 240;

    /** Default constructor — no controller, defaults for all fields. */
    TouchConfig() = default;

    /**
     * @brief Factory: XPT2046 SPI configuration.
     * @param cs SPI chip-select pin.
     * @param irq Interrupt pin (255 = unused).
     * @return Configured TouchConfig.
     */
    static TouchConfig createXPT2046(uint8_t cs, uint8_t irq = 255) {
        TouchConfig config;
        config.controller = TouchController::XPT2046;
        config.csPin = cs;
        config.irqPin = irq;
        return config;
    }

    /**
     * @brief Factory: GT911 I2C configuration.
     * @param irq Interrupt pin (default 4).
     * @return Configured TouchConfig.
     */
    static TouchConfig createGT911(uint8_t irq = 4) {
        TouchConfig config;
        config.controller = TouchController::GT911;
        config.irqPin = irq;
        return config;
    }
};

// =========================================================================
// Build flag helpers
// =========================================================================

/**
 * @brief Compile-time check for enabled touch controller
 */
#if defined(TOUCH_DRIVER_XPT2046)
    #define PIXELROOT32_TOUCH_ENABLED 1
    #define PIXELROOT32_TOUCH_CONTROLLER pixelroot32::graphics::TouchController::XPT2046
#elif defined(TOUCH_DRIVER_GT911)
    #define PIXELROOT32_TOUCH_ENABLED 1
    #define PIXELROOT32_TOUCH_CONTROLLER pixelroot32::graphics::TouchController::GT911
#else
    #define PIXELROOT32_TOUCH_ENABLED 0
    #define PIXELROOT32_TOUCH_CONTROLLER pixelroot32::graphics::TouchController::None
#endif

} // namespace pixelroot32::graphics
