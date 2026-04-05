/*
 * PixelRoot32 Game Engine
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 *
 * TouchEventTypes.h - Touch event type enumerations
 * Defines high-level gesture event types for the touch event system
 */
#pragma once

#include <cstdint>

namespace pixelroot32::input {

/**
 * @enum TouchEventType
 * @brief High-level touch event types for gesture detection
 * 
 * These events represent the semantic meaning of touch interactions,
 * not the raw hardware events (which are handled by TouchAdapter).
 */
enum class TouchEventType : uint8_t {
    None = 0,           ///< No event
    
    // Press/Release events
    TouchDown = 1,      ///< Touch pressed down
    TouchUp = 2,        ///< Touch released
    
    // Click events
    Click = 3,          ///< Single click (press + release within threshold)
    DoubleClick = 4,    ///< Double click (two clicks within interval)
    
    // Hold events
    LongPress = 5,      ///< Long press (held beyond threshold)
    
    // Drag events
    DragStart = 6,      ///< Drag started (moved beyond drag threshold)
    DragMove = 7,       ///< Dragging (continuing movement)
    DragEnd = 8         ///< Drag ended (released while dragging)
};

/**
 * @brief Timing thresholds for touch gesture detection
 * All values in milliseconds or pixels
 */
namespace TouchTiming {
    /// Maximum duration for a click (press + release)
    constexpr uint16_t CLICK_MAX_DURATION = 300;
    
    /// Maximum interval between clicks for double-click detection
    constexpr uint16_t DOUBLE_CLICK_INTERVAL = 400;
    
    /// Duration to hold before triggering long press
    constexpr uint16_t LONG_PRESS_THRESHOLD = 800;
    
    /// Movement threshold to trigger drag
    constexpr uint16_t DRAG_THRESHOLD = 10;
}

/**
 * @enum TouchEventFlags
 * @brief Flags for touch events
 */
enum class TouchEventFlags : uint8_t {
    None = 0,
    Primary = 1 << 0,    ///< Primary touch (first finger)
    Consumed = 1 << 1    ///< Event was consumed by a handler
};

} // namespace pixelroot32::input
