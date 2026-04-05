/*
 * PixelRoot32 Game Engine
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 *
 * TouchTypes - Legacy type definitions
 * NOTE: This file contains legacy types kept for backward compatibility.
 * New code should use: TouchEvent.h, TouchEventTypes.h, TouchPoint.h, TouchStateMachine.h
 */
#pragma once

#include <cstdint>
#include "TouchPoint.h"
#include "TouchEvent.h"
#include "TouchEventTypes.h"

namespace pixelroot32::input {

// NOTE: TouchState is now defined in TouchStateMachine.h
// NOTE: TouchEventType is now defined in TouchEventTypes.h  
// NOTE: TouchEvent is now defined in TouchEvent.h

/**
 * @brief Maximum number of touch events to store in history
 */
constexpr uint8_t TOUCH_EVENT_HISTORY_SIZE = 16;

/**
 * @struct TouchEventHistory
 * @brief Ring buffer for touch events (for gesture detection)
 * @deprecated Use TouchEventQueue from TouchEventQueue.h instead
 */
struct TouchEventHistory {
    TouchEvent events[TOUCH_EVENT_HISTORY_SIZE];
    uint8_t count;
    uint8_t head;
    
    TouchEventHistory() : count(0), head(0) {}
    
    /**
     * @brief Add event to history
     */
    void push(const TouchEvent& event) {
        events[head] = event;
        head = (head + 1) % TOUCH_EVENT_HISTORY_SIZE;
        if (count < TOUCH_EVENT_HISTORY_SIZE) {
            count++;
        }
    }
    
    /**
     * @brief Clear history
     */
    void clear() {
        count = 0;
        head = 0;
    }
    
    /**
     * @brief Get most recent event
     */
    const TouchEvent* mostRecent() const {
        if (count == 0) return nullptr;
        uint8_t idx = (head + TOUCH_EVENT_HISTORY_SIZE - 1) % TOUCH_EVENT_HISTORY_SIZE;
        return &events[idx];
    }
};

} // namespace pixelroot32::input