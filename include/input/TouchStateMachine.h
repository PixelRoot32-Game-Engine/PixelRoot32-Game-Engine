/*
 * PixelRoot32 Game Engine
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 *
 * TouchStateMachine.h - Touch state machine
 * Handles state transitions for touch gestures
 */
#pragma once

#include <cstdint>
#include <input/TouchEvent.h>
#include <input/TouchEventQueue.h>
#include <input/TouchEventTypes.h>

namespace pixelroot32::input {

/**
 * @enum TouchState
 * @brief Internal states for touch gesture detection
 */
enum class TouchState : uint8_t {
    Idle = 0,          ///< No active touch
    Pressed = 1,       ///< Touch is pressed, waiting for release
    LongPress = 2,     ///< Long press detected
    Dragging = 3       ///< Drag gesture in progress
};

/**
 * @struct TouchStateData
 * @brief Per-touch-ID state tracking
 */
struct TouchStateData {
    TouchState state;          ///< Current state
    uint32_t pressTime;        ///< When press started (ms)
    int16_t pressX;            ///< X position at press
    int16_t pressY;            ///< Y position at press
    int16_t lastX;             ///< Last known X position
    int16_t lastY;             ///< Last known Y position
    bool longPressFired;       ///< Long press already fired
    bool dragStarted;          ///< Drag already started
    
    TouchStateData()
        : state(TouchState::Idle)
        , pressTime(0)
        , pressX(0)
        , pressY(0)
        , lastX(0)
        , lastY(0)
        , longPressFired(false)
        , dragStarted(false) {}
};

/**
 * @class TouchStateMachine
 * @brief State machine for touch gesture detection
 * 
 * State transitions:
 * Idle → Pressed (on touch down)
 * Pressed → LongPress (after LONG_PRESS_THRESHOLD without release)
 * Pressed → Dragging (after DRAG_THRESHOLD movement)
 * Pressed → Idle (on touch up → generate Click/DoubleClick)
 * LongPress → Idle (on touch up)
 * Dragging → Idle (on touch up → generate DragEnd)
 * 
 * O(1) update - deterministic timing
 */
class TouchStateMachine {
public:
    static constexpr uint8_t MAX_TOUCH_IDS = 5;
    
    /**
     * @brief Default constructor
     */
    TouchStateMachine();
    
    /**
     * @brief Process a touch input update
     * @param touchId Touch identifier (0-4)
     * @param pressed True if touch is currently pressed
     * @param x Current X position
     * @param y Current Y position
     * @param timestamp Current timestamp in ms
     * @param outputQueue Queue to enqueue generated events
     */
    void update(uint8_t touchId, bool pressed, int16_t x, int16_t y, 
                uint32_t timestamp, TouchEventQueue& outputQueue);
    
    /**
     * @brief Reset state for a specific touch ID
     * @param touchId Touch identifier to reset
     */
    void reset(uint8_t touchId);
    
    /**
     * @brief Reset all touch states
     */
    void resetAll();
    
    /**
     * @brief Get current state for a touch ID
     * @param touchId Touch identifier
     * @return Current state
     */
    TouchState getState(uint8_t touchId) const;
    
    /**
     * @brief Check if a touch is in progress
     * @return true if any touch is active
     */
    bool isActive() const;
    
    /**
     * @brief Get time since press for a touch
     * @param touchId Touch identifier
     * @param currentTime Current timestamp in ms (same source as update())
     * @return Milliseconds since press started, 0 if not pressed
     */
    uint32_t getPressDuration(uint8_t touchId, uint32_t currentTime) const;
    
private:
    /// Per-touch state data
    TouchStateData touchStates[MAX_TOUCH_IDS];
    
    /// Last click timestamp for double-click detection
    uint32_t lastClickTime[MAX_TOUCH_IDS];
    
    /// Last click position for double-click detection
    int16_t lastClickX[MAX_TOUCH_IDS];
    int16_t lastClickY[MAX_TOUCH_IDS];
    
    /**
     * @brief Calculate Manhattan distance between two points
     */
    static int16_t distance(int16_t x1, int16_t y1, int16_t x2, int16_t y2);
    
    /**
     * @brief Handle touch down event
     */
    void handleTouchDown(uint8_t touchId, int16_t x, int16_t y, 
                        uint32_t timestamp, TouchEventQueue& queue);
    
    /**
     * @brief Handle touch up event
     */
    void handleTouchUp(uint8_t touchId, int16_t x, int16_t y, 
                      uint32_t timestamp, TouchEventQueue& queue);
    
    /**
     * @brief Handle touch move while pressed
     */
    void handleTouchMove(uint8_t touchId, int16_t x, int16_t y, 
                        uint32_t timestamp, TouchEventQueue& queue);
    
    /**
     * @brief Check and generate long press if needed
     */
    void checkLongPress(uint8_t touchId, uint32_t timestamp, TouchEventQueue& queue);
};

} // namespace pixelroot32::input
