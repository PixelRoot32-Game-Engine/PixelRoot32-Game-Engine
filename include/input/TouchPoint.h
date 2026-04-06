/*
 * PixelRoot32 Game Engine
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 *
 * Touch Point - Normalized touch data structure
 * Used by both XPT2046 and GT911 adapters
 */
#pragma once

#include <cstdint>

namespace pixelroot32::input {

/**
 * @struct TouchPoint
 * @brief Normalized touch data structure.
 *
 * This struct is the CONTRACT between TouchAdapter and the engine.
 * It MUST remain unchanged regardless of the underlying touch controller.
 *
 * Invariants:
 * - Coordinates always valid: 0 <= x <= W, 0 <= y <= H
 * - No extreme noise (filtered by adapter)
 * - pressed state consistent
 * - timestamps monotonic
 */
struct TouchPoint {
    int16_t x;          ///< X coordinate (0 = left edge)
    int16_t y;          ///< Y coordinate (0 = top edge)
    bool pressed;       ///< True if touch is active
    uint8_t id;         ///< Touch ID (0 for single-touch, 0-4 for multi-touch)
    uint32_t ts;        ///< Timestamp in milliseconds

    /**
     * @brief Default constructor - creates empty/invalid touch point
     */
    TouchPoint()
        : x(0), y(0), pressed(false), id(0), ts(0) {}

    /**
     * @brief Construct a touch point with all fields
     * @param xPos X coordinate
     * @param yPos Y coordinate
     * @param isPressed Touch state
     * @param touchId Touch identifier
     * @param timestamp Timestamp in ms
     */
    TouchPoint(int16_t xPos, int16_t yPos, bool isPressed, uint8_t touchId, uint32_t timestamp)
        : x(xPos), y(yPos), pressed(isPressed), id(touchId), ts(timestamp) {}

    /**
     * @brief Check if this touch point is valid (within bounds)
     * @param maxX Maximum X value (display width - 1)
     * @param maxY Maximum Y value (display height - 1)
     * @return true if coordinates are in valid range
     */
    bool isValid(int16_t maxX, int16_t maxY) const {
        return x >= 0 && x <= maxX && y >= 0 && y <= maxY;
    }

    /**
     * @brief Check if this is a null/empty touch point
     * @return true if not pressed
     */
    bool isEmpty() const {
        return !pressed;
    }
};

/**
 * @brief Maximum number of simultaneous touch points supported
 * XPT2046: 1 point, GT911: 5 points
 */
constexpr uint8_t TOUCH_MAX_POINTS = 5;

}
