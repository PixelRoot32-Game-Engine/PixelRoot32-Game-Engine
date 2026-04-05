/*
 * PixelRoot32 Game Engine
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 *
 * TouchAdapter - Template-based adapter interface (no virtuals for ESP32)
 * Provides static dispatch for different touch controllers
 */
#pragma once

#include <cstddef>
#include "TouchPoint.h"

namespace pixelroot32::input {

/**
 * @brief Touch controller types
 */
enum class TouchController {
    None = 0,
    XPT2046 = 1,
    GT911 = 2
};

/**
 * @brief Touch adapter filtering constants
 */
namespace filtering {
    constexpr uint8_t MEDIAN_WINDOW = 5;       ///< Median filter window size
    constexpr uint8_t DEBOUNCE_MS = 10;         ///< Debounce threshold in ms
    constexpr uint16_t NOISE_THRESHOLD = 50;     ///< Noise threshold for coordinate jitter
    constexpr uint16_t PRESSURE_THRESHOLD = 50; ///< Pressure threshold (ADC units) for XPT2046
}

/**
 * @enum DisplayPreset
 * @brief Display presets for common displays
 */
enum class DisplayPreset : uint8_t {
    None = 0,
    ILI9341_320x240,    ///< ILI9341 320x240 (2.2" - 2.8")
    ST7789_240x320,     ///< ST7789 240x320 (portrait)
    ST7789_240x240,     ///< ST7789 240x240 (round)
    ST7735_128x160,     ///< ST7735 128x160 (1.8")
    ST7735_128x128,    ///< ST7735 128x128 (1.44")
    ILI9488_320x480,   ///< ILI9488 320x480 (3.5")
    GC9A01_240x240,     ///< GC9A01 240x240 (round)
    Custom = 255       ///< Custom resolution
};

/**
 * @enum TouchRotation
 * @brief Display rotation modes for calibration
 */
enum class TouchRotation : uint8_t {
    Rotation0 = 0,   ///< 0 degrees (default)
    Rotation90 = 1,  ///< 90 degrees clockwise
    Rotation180 = 2, ///< 180 degrees
    Rotation270 = 3  ///< 270 degrees
};

/**
 * @class TouchCalibration
 * @brief Calibration parameters for coordinate transformation
 * 
 * Supports:
 * - Scale factors for X/Y axes
 * - Offset adjustments
 * - Display rotation transformation
 * - Factory presets for common displays
 */
class TouchCalibration {
public:
    TouchCalibration();
    
    /**
     * @brief Create calibration from preset
     * @param preset Display preset
     * @return TouchCalibration instance
     */
    static TouchCalibration fromPreset(DisplayPreset preset);
    
    /**
     * @brief Create calibration for custom resolution
     * @param width Display width
     * @param height Display height
     * @return TouchCalibration instance
     */
    static TouchCalibration forResolution(int16_t width, int16_t height);
    
    /**
     * @brief Transform raw coordinates to screen space
     * @param rawX Raw X coordinate
     * @param rawY Raw Y coordinate
     * @param pressed Touch pressed state
     * @param id Touch ID
     * @param ts Timestamp
     * @return Transformed TouchPoint
     */
    TouchPoint transform(int16_t rawX, int16_t rawY, bool pressed, uint8_t id, uint32_t ts) const;
    
    // Configuration
    float scaleX = 1.0f;      ///< X axis scale factor
    float scaleY = 1.0f;      ///< Y axis scale factor
    int16_t offsetX = 0;      ///< X axis offset
    int16_t offsetY = 0;      ///< Y axis offset
    int16_t displayWidth = 320;   ///< Display width for clamping
    int16_t displayHeight = 240;  ///< Display height for clamping
    TouchRotation rotation = TouchRotation::Rotation0;
    
    /**
     * @brief Set rotation mode
     */
    void setRotation(TouchRotation rot);
    
    /**
     * @brief Apply rotation transformation to coordinates
     * @param x Input X coordinate
     * @param y Input Y coordinate
     * @param outX Output X coordinate
     * @param outY Output Y coordinate
     */
    void applyRotation(int16_t x, int16_t y, int16_t& outX, int16_t& outY) const;
    
    /**
     * @brief Invert rotation (for opposite rotation)
     */
    TouchRotation inverted() const;
    
private:
    // Private helper to initialize from preset
    static TouchCalibration createForDisplay(DisplayPreset preset);
};

// =========================================================================
// Legacy struct for backward compatibility - now a typedef
// =========================================================================
using TouchCalibrationData = TouchCalibration;

/**
 * @class TouchAdapterBase
 * @brief Base class requirements for touch adapters (conceptual)
 *
 * This defines the interface that ALL touch adapters must implement.
 * Uses template static dispatch instead of virtual functions for ESP32.
 *
 * @tparam Adapter Concrete adapter type (XPT2046Adapter or GT911Adapter)
 */
template<typename Adapter>
class TouchAdapter {
public:
    /**
     * @brief Initialize the touch controller
     * @return true if initialization successful
     */
    static bool init() {
        return Adapter::initImpl();
    }

    /**
     * @brief Read touch points from controller
     * @param points Output buffer for touch points
     * @param count Number of touch points read
     * @return true if read successful
     */
    static bool read(TouchPoint* points, uint8_t& count) {
        return Adapter::readImpl(points, count);
    }

    /**
     * @brief Set calibration parameters
     * @param calib Calibration data
     */
    static void setCalibration(const TouchCalibration& calib) {
        Adapter::setCalibrationImpl(calib);
    }

    /**
     * @brief Check if controller is connected
     * @return true if controller responds
     */
    static bool isConnected() {
        return Adapter::isConnectedImpl();
    }
};

/**
 * @brief Compile-time feature flag for enabled touch controller
 * Define TOUCH_DRIVER_XPT2046 or TOUCH_DRIVER_GT911 in build flags
 */
#if defined(TOUCH_DRIVER_XPT2046)
    using ActiveTouchAdapter = TouchAdapter<struct XPT2046Adapter>;
#elif defined(TOUCH_DRIVER_GT911)
    using ActiveTouchAdapter = TouchAdapter<struct GT911Adapter>;
#else
    // No touch controller enabled
    using ActiveTouchAdapter = std::nullptr_t;
#endif

}
