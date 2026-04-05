/*
 * PixelRoot32 Game Engine
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 *
 * TouchStateMachine.cpp - State machine implementation
 */
#include <input/TouchStateMachine.h>

namespace pixelroot32::input {

TouchStateMachine::TouchStateMachine() {
    resetAll();
}

void TouchStateMachine::reset(uint8_t touchId) {
    if (touchId < MAX_TOUCH_IDS) {
        touchStates[touchId] = TouchStateData();
        lastClickTime[touchId] = 0;
        lastClickX[touchId] = 0;
        lastClickY[touchId] = 0;
    }
}

void TouchStateMachine::resetAll() {
    for (uint8_t i = 0; i < MAX_TOUCH_IDS; i++) {
        reset(i);
    }
}

TouchState TouchStateMachine::getState(uint8_t touchId) const {
    if (touchId >= MAX_TOUCH_IDS) {
        return TouchState::Idle;
    }
    return touchStates[touchId].state;
}

bool TouchStateMachine::isActive() const {
    for (uint8_t i = 0; i < MAX_TOUCH_IDS; i++) {
        if (touchStates[i].state != TouchState::Idle) {
            return true;
        }
    }
    return false;
}

uint32_t TouchStateMachine::getPressDuration(uint8_t touchId, uint32_t currentTime) const {
    if (touchId >= MAX_TOUCH_IDS) {
        return 0;
    }

    const TouchStateData& state = touchStates[touchId];
    if (state.state == TouchState::Idle) {
        return 0;
    }

    return currentTime - state.pressTime;
}

int16_t TouchStateMachine::distance(int16_t x1, int16_t y1, int16_t x2, int16_t y2) {
    int16_t dx = (x2 > x1) ? (x2 - x1) : (x1 - x2);
    int16_t dy = (y2 > y1) ? (y2 - y1) : (y1 - y2);
    return dx + dy;
}

void TouchStateMachine::update(uint8_t touchId, bool pressed, int16_t x, int16_t y,
                               uint32_t timestamp, TouchEventQueue& outputQueue) {
    if (touchId >= MAX_TOUCH_IDS) {
        return;
    }
    
    TouchStateData& state = touchStates[touchId];
    
    if (pressed) {
        // Touch is down
        if (state.state == TouchState::Idle) {
            // New touch press
            handleTouchDown(touchId, x, y, timestamp, outputQueue);
        } else {
            // Movement while pressed
            handleTouchMove(touchId, x, y, timestamp, outputQueue);
        }
        
        // Check for long press
        checkLongPress(touchId, timestamp, outputQueue);
    } else {
        // Touch is up
        if (state.state != TouchState::Idle) {
            handleTouchUp(touchId, x, y, timestamp, outputQueue);
        }
    }
}

void TouchStateMachine::handleTouchDown(uint8_t touchId, int16_t x, int16_t y,
                                       uint32_t timestamp, TouchEventQueue& queue) {
    TouchStateData& state = touchStates[touchId];
    
    state.state = TouchState::Pressed;
    state.pressTime = timestamp;
    state.pressX = x;
    state.pressY = y;
    state.lastX = x;
    state.lastY = y;
    state.longPressFired = false;
    state.dragStarted = false;
    
    // Create TouchDown event
    TouchEvent event(TouchEventType::TouchDown, touchId, x, y, timestamp);
    event.setPrimary();
    queue.enqueue(event);
}

void TouchStateMachine::handleTouchUp(uint8_t touchId, int16_t x, int16_t y,
                                     uint32_t timestamp, TouchEventQueue& queue) {
    TouchStateData& state = touchStates[touchId];
    uint32_t duration = timestamp - state.pressTime;

    if (state.state == TouchState::Dragging) {
        // Drag release: only DragEnd, no TouchUp
        TouchEvent dragEndEvent(TouchEventType::DragEnd, touchId, x, y, timestamp);
        queue.enqueue(dragEndEvent);
    } else {
        // Non-drag release: always emit TouchUp
        TouchEvent upEvent(TouchEventType::TouchUp, touchId, x, y, timestamp);
        queue.enqueue(upEvent);

        if (!state.longPressFired && duration <= TouchTiming::CLICK_MAX_DURATION) {
            uint32_t lastClick = lastClickTime[touchId];

            if (lastClick > 0 && (timestamp - lastClick) <= TouchTiming::DOUBLE_CLICK_INTERVAL) {
                int16_t dist = distance(x, y, lastClickX[touchId], lastClickY[touchId]);
                if (dist <= TouchTiming::DRAG_THRESHOLD * 2) {
                    TouchEvent dblClick(TouchEventType::DoubleClick, touchId, x, y, timestamp);
                    dblClick.setPrimary();
                    queue.enqueue(dblClick);
                    lastClickTime[touchId] = 0;
                } else {
                    TouchEvent click(TouchEventType::Click, touchId, x, y, timestamp);
                    click.setPrimary();
                    queue.enqueue(click);
                    lastClickTime[touchId] = timestamp;
                    lastClickX[touchId] = x;
                    lastClickY[touchId] = y;
                }
            } else {
                TouchEvent click(TouchEventType::Click, touchId, x, y, timestamp);
                click.setPrimary();
                queue.enqueue(click);
                lastClickTime[touchId] = timestamp;
                lastClickX[touchId] = x;
                lastClickY[touchId] = y;
            }
        }
    }

    state.state = TouchState::Idle;
    state.pressTime = 0;
}

void TouchStateMachine::handleTouchMove(uint8_t touchId, int16_t x, int16_t y,
                                        uint32_t timestamp, TouchEventQueue& queue) {
    TouchStateData& state = touchStates[touchId];
    
    // Calculate distance from press position
    int16_t dist = distance(x, y, state.pressX, state.pressY);
    
    if (!state.dragStarted && dist >= TouchTiming::DRAG_THRESHOLD) {
        state.dragStarted = true;
        state.state = TouchState::Dragging;

        TouchEvent dragStart(TouchEventType::DragStart, touchId, x, y, timestamp);
        queue.enqueue(dragStart);
    } else if (state.state == TouchState::Dragging) {
        TouchEvent dragMove(TouchEventType::DragMove, touchId, x, y, timestamp);
        queue.enqueue(dragMove);
    }
    
    state.lastX = x;
    state.lastY = y;
}

void TouchStateMachine::checkLongPress(uint8_t touchId, uint32_t timestamp, 
                                       TouchEventQueue& queue) {
    TouchStateData& state = touchStates[touchId];
    
    if (state.state == TouchState::Pressed && !state.longPressFired) {
        uint32_t duration = timestamp - state.pressTime;
        
        if (duration >= TouchTiming::LONG_PRESS_THRESHOLD) {
            state.longPressFired = true;
            state.state = TouchState::LongPress;
            
            // Generate LongPress event
            TouchEvent longPress(TouchEventType::LongPress, touchId, 
                                state.pressX, state.pressY, timestamp);
            longPress.setPrimary();
            queue.enqueue(longPress);
        }
    }
}

} // namespace pixelroot32::input
