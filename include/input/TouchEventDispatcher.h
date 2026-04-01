/*
 * PixelRoot32 Game Engine
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 *
 * TouchEventDispatcher.h - Pull-based event dispatcher
 * Combines state machine and event queue for consumer-driven event retrieval
 */
#pragma once

#include <cstdint>
#include <input/TouchEvent.h>
#include <input/TouchEventQueue.h>
#include <input/TouchStateMachine.h>
#include <input/TouchPoint.h>

namespace pixelroot32::input {

/**
 * @class TouchEventDispatcher
 * @brief Pull-based touch event dispatcher
 * 
 * This is the main API for consumers to receive touch events.
 * It combines the state machine and event queue into a unified interface.
 * 
 * Usage pattern (pull-based):
 * @code
 * TouchEvent events[16];
 * uint8_t count = dispatcher.getEvents(events, 16);
 * for (uint8_t i = 0; i < count; i++) {
 *     handleEvent(events[i]);
 * }
 * @endcode
 * 
 * Or for checking without consuming:
 * @code
 * if (dispatcher.hasEvents()) {
 *     TouchEvent event;
 *     dispatcher.peek(event);
 *     // inspect without removing
 * }
 * @endcode
 */
class TouchEventDispatcher {
public:
    /**
     * @brief Default constructor
     */
    TouchEventDispatcher();
    
    /**
     * @brief Process raw touch input
     * @param touchId Touch identifier (0-4)
     * @param pressed True if touch is pressed
     * @param x X position
     * @param y Y position
     * @param timestamp Current timestamp in ms
     * 
     * Call this every frame with the current touch state.
     */
    void processTouch(uint8_t touchId, bool pressed, int16_t x, int16_t y, 
                      uint32_t timestamp);
    
    /**
     * @brief Process multiple touch points
     * @param points Array of touch points
     * @param count Number of touch points
     * @param timestamp Current timestamp in ms
     */
    void processTouchPoints(const TouchPoint* points, uint8_t count, 
                           uint32_t timestamp);
    
    /**
     * @brief Get events using caller-provided buffer (pull-based)
     * @param events Caller-provided buffer for events
     * @param maxCount Maximum events to retrieve
     * @return Number of events retrieved (removed from queue)
     * 
     * This is the primary API for consumers.
     * Events are removed from the internal queue.
     */
    uint8_t getEvents(TouchEvent* events, uint8_t maxCount);
    
    /**
     * @brief Peek at events without removing them
     * @param events Caller-provided buffer
     * @param maxCount Maximum events to peek
     * @return Number of events peeked
     */
    uint8_t peekEvents(TouchEvent* events, uint8_t maxCount) const;
    
    /**
     * @brief Check if events are available
     * @return true if queue has events
     */
    bool hasEvents() const;
    
    /**
     * @brief Get number of pending events
     * @return Number of events in queue
     */
    uint8_t getEventCount() const;
    
    /**
     * @brief Clear all pending events
     */
    void clearEvents();
    
    /**
     * @brief Reset state machine (force all touches to idle)
     */
    void reset();
    
    /**
     * @brief Get current state for a touch ID
     * @param touchId Touch identifier
     * @return Current state
     */
    TouchState getTouchState(uint8_t touchId) const;
    
    /**
     * @brief Check if any touch is active
     * @return true if any touch is in progress
     */
    bool isTouchActive() const;
    
private:
    /// Event queue for generated events
    TouchEventQueue eventQueue;
    
    /// State machine for gesture detection
    TouchStateMachine stateMachine;
};

// Inline implementations

inline TouchEventDispatcher::TouchEventDispatcher() {}

inline void TouchEventDispatcher::processTouch(uint8_t touchId, bool pressed, 
                                               int16_t x, int16_t y, 
                                               uint32_t timestamp) {
    stateMachine.update(touchId, pressed, x, y, timestamp, eventQueue);
}

inline void TouchEventDispatcher::processTouchPoints(const TouchPoint* points, 
                                                      uint8_t count,
                                                      uint32_t timestamp) {
    if (points == nullptr || count == 0) {
        return;
    }
    
    for (uint8_t i = 0; i < count && i < TouchStateMachine::MAX_TOUCH_IDS; i++) {
        const TouchPoint& tp = points[i];
        stateMachine.update(tp.id, tp.pressed, tp.x, tp.y, timestamp, eventQueue);
    }
}

inline uint8_t TouchEventDispatcher::getEvents(TouchEvent* events, uint8_t maxCount) {
    return eventQueue.getEvents(events, maxCount);
}

inline uint8_t TouchEventDispatcher::peekEvents(TouchEvent* events, uint8_t maxCount) const {
    return eventQueue.peekMultiple(events, maxCount);
}

inline bool TouchEventDispatcher::hasEvents() const {
    return eventQueue.hasEvents();
}

inline uint8_t TouchEventDispatcher::getEventCount() const {
    return eventQueue.getCount();
}

inline void TouchEventDispatcher::clearEvents() {
    eventQueue.clear();
}

inline void TouchEventDispatcher::reset() {
    eventQueue.clear();
    stateMachine.resetAll();
}

inline TouchState TouchEventDispatcher::getTouchState(uint8_t touchId) const {
    return stateMachine.getState(touchId);
}

inline bool TouchEventDispatcher::isTouchActive() const {
    return stateMachine.isActive();
}

} // namespace pixelroot32::input
