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
 * @brief Compact touch event structure (12 bytes total, naturally aligned)
 * 
 * Memory layout (naturally aligned, no packing needed):
 * - timestamp: 4 bytes (offset 0)
 * - x:         2 bytes (offset 4)
 * - y:         2 bytes (offset 6)
 * - type:      1 byte (offset 8)
 * - flags:     1 byte (offset 9)
 * - id:        1 byte (offset 10)
 * - _padding:  1 byte (offset 11)
 * Total: 12 bytes
 * 
 * Invariants:
 * - timestamp always monotonically increasing per touch ID
 * - x, y always valid (within display bounds)
 * - type always non-None when queued
 */
struct TouchEvent {
    uint32_t timestamp;       ///< Timestamp in milliseconds
    int16_t x;                ///< X coordinate
    int16_t y;                ///< Y coordinate
    uint8_t type;             ///< Event type
    uint8_t flags;            ///< Event flags
    uint8_t id;               ///< Touch ID (0-4)
    uint8_t _padding;         ///< Explicit padding for alignment
    
    /**
     * @brief Default constructor - creates empty event
     */
    TouchEvent()
        : timestamp(0)
        , x(0)
        , y(0)
        , type(0)
        , flags(0)
        , id(0)
        , _padding(0) {}
    
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
        : timestamp(ts)
        , x(xPos)
        , y(yPos)
        , type(static_cast<uint8_t>(eventType))
        , flags(static_cast<uint8_t>(eventFlags))
        , id(touchId)
        , _padding(0) {}
    
    /**
     * @brief Get event type as enum
     * @return Event type
     */
    TouchEventType getType() const {
        return static_cast<TouchEventType>(type);
    }
    
    /**
     * @brief Get event flags as enum
     * @return Event flags
     */
    TouchEventFlags getFlags() const {
        return static_cast<TouchEventFlags>(flags);
    }
    
    /**
     * @brief Set event type from enum
     * @param eventType Type of event
     */
    void setType(TouchEventType eventType) {
        type = static_cast<uint8_t>(eventType);
    }
    
    /**
     * @brief Set event flags from enum
     * @param eventFlags Event flags
     */
    void setFlags(TouchEventFlags eventFlags) {
        flags = static_cast<uint8_t>(eventFlags);
    }
    
    /**
     * @brief Check if event is valid (has a type)
     * @return true if type is not None
     */
    bool isValid() const {
        return static_cast<TouchEventType>(type) != TouchEventType::None;
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
        flags = static_cast<uint8_t>(flags) | static_cast<uint8_t>(TouchEventFlags::Consumed);
    }
    
    /**
     * @brief Set primary flag
     */
    void setPrimary() {
        flags = static_cast<uint8_t>(flags) | static_cast<uint8_t>(TouchEventFlags::Primary);
    }
};

/**
 * @brief Maximum number of events in the queue
 */
constexpr uint8_t TOUCH_EVENT_QUEUE_SIZE = 16;

/**
 * @brief Total memory for event queue (192 bytes)
 */
constexpr uint16_t TOUCH_EVENT_QUEUE_MEMORY = sizeof(TouchEvent) * TOUCH_EVENT_QUEUE_SIZE;

} // namespace pixelroot32::input
