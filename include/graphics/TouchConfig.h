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
 * @enum TouchController
 * @brief Supported touch controller types
 */
enum class TouchController {
    None = 0,     ///< No touch controller
    XPT2046 = 1,  ///< SPI touch controller (common on dev boards)
    GT911 = 2     ///< I2C touch controller (Goodix)
};

/**
 * @struct TouchConfig
 * @brief Configuration for touch controller
 *
 * Add to DisplayConfig or use standalone.
 * Define one of TOUCH_DRIVER_XPT2046 or TOUCH_DRIVER_GT911 in build flags.
 */
struct TouchConfig {
    TouchController controller = TouchController::None;  ///< Active controller
    
    // SPI settings (for XPT2046)
    uint32_t spiClockHz = 2500000;    // 2.5 MHz
    uint8_t csPin = 5;               // SPI CS pin
    uint8_t irqPin = 4;              // Interrupt pin (optional)
    
    // I2C settings (for GT911)
    uint32_t i2cClockHz = 400000;     // 400 kHz
    uint8_t i2cAddress = 0x5D;        // GT911 I2C address
    
    // Calibration defaults
    float scaleX = 1.0f;
    float scaleY = 1.0f;
    int16_t offsetX = 0;
    int16_t offsetY = 0;
    
    // Display bounds for coordinate clamping
    uint16_t displayWidth = 320;
    uint16_t displayHeight = 240;

    /**
     * @brief Default constructor - no touch
     */
    TouchConfig() = default;

    /**
     * @brief Constructor for XPT2046
     */
    static TouchConfig createXPT2046(uint8_t cs, uint8_t irq = 255) {
        TouchConfig config;
        config.controller = TouchController::XPT2046;
        config.csPin = cs;
        config.irqPin = irq;
        return config;
    }

    /**
     * @brief Constructor for GT911
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
