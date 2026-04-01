/*
 * PixelRoot32 Game Engine
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 *
 * TouchEvent.h - Touch event data structure
 * Compact 12-byte event structure for the touch event system
 */
#pragma once

#include <cstdint>
#include "TouchEventTypes.h"

namespace pixelroot32::input {

/**
 * @struct TouchEvent
 * @brief Compact touch event structure (12 bytes total, packed)
 * 
 * Memory layout (packed):
 * - type:    1 byte (offset 0)
 * - flags:   1 byte (offset 1)
 * - id:      1 byte (offset 2)
 * - x:       2 bytes (offset 4)
 * - y:       2 bytes (offset 6)
 * - timestamp: 4 bytes (offset 8)
 * Total: 12 bytes (with packing)
 * 
 * Invariants:
 * - timestamp always monotonically increasing per touch ID
 * - x, y always valid (within display bounds)
 * - type always non-None when queued
 */
struct TouchEvent {
    TouchEventType type;      ///< Event type
    TouchEventFlags flags;    ///< Event flags
    uint8_t id;               ///< Touch ID (0-4)
    int16_t x;                ///< X coordinate
    int16_t y;                ///< Y coordinate
    uint32_t timestamp;       ///< Timestamp in milliseconds
    
    /**
     * @brief Default constructor - creates empty event
     */
    TouchEvent()
        : type(TouchEventType::None)
        , flags(TouchEventFlags::None)
        , id(0)
        , x(0)
        , y(0)
        , timestamp(0) {}
    
    /**
     * @brief Construct touch event with all fields
     * @param eventType Type of event
     * @param touchId Touch identifier
     * @param xPos X coordinate
     * @param yPos Y coordinate
     * @param ts Timestamp in ms
     * @param eventFlags Event flags
     */
    TouchEvent(TouchEventType eventType, uint8_t touchId, int16_t xPos, int16_t yPos, 
               uint32_t ts, TouchEventFlags eventFlags = TouchEventFlags::None)
        : type(eventType)
        , flags(eventFlags)
        , id(touchId)
        , x(xPos)
        , y(yPos)
        , timestamp(ts) {}
    
    /**
     * @brief Check if event is valid (has a type)
     * @return true if type is not None
     */
    bool isValid() const {
        return type != TouchEventType::None;
    }
    
    /**
     * @brief Check if this is a primary touch
     * @return true if Primary flag is set
     */
    bool isPrimary() const {
        return (static_cast<uint8_t>(flags) & static_cast<uint8_t>(TouchEventFlags::Primary)) != 0;
    }
    
    /**
     * @brief Check if event was consumed
     * @return true if Consumed flag is set
     */
    bool isConsumed() const {
        return (static_cast<uint8_t>(flags) & static_cast<uint8_t>(TouchEventFlags::Consumed)) != 0;
    }
    
    /**
     * @brief Mark event as consumed
     */
    void consume() {
        flags = static_cast<TouchEventFlags>(
            static_cast<uint8_t>(flags) | static_cast<uint8_t>(TouchEventFlags::Consumed));
    }
    
    /**
     * @brief Set primary flag
     */
    void setPrimary() {
        flags = static_cast<TouchEventFlags>(
            static_cast<uint8_t>(flags) | static_cast<uint8_t>(TouchEventFlags::Primary));
    }
} __attribute__((packed));

/**
 * @brief Maximum number of events in the queue
 */
constexpr uint8_t TOUCH_EVENT_QUEUE_SIZE = 16;

/**
 * @brief Total memory for event queue (192 bytes)
 */
constexpr uint16_t TOUCH_EVENT_QUEUE_MEMORY = sizeof(TouchEvent) * TOUCH_EVENT_QUEUE_SIZE;

} // namespace pixelroot32::input
