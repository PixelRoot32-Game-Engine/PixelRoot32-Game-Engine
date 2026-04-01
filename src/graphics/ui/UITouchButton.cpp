/*
 * PixelRoot32 Game Engine
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 *
 * UITouchButton.cpp - Touch-optimized button widget implementation
 */
#include "core/EngineModules.h"
#if PIXELROOT32_ENABLE_UI_SYSTEM

#include "graphics/ui/UITouchButton.h"
#include "input/TouchEvent.h"

namespace pixelroot32::graphics::ui {

void UITouchButton::setOnDown(UITouchButton::ButtonCallback callback) {
    onDownCallback = callback;
}

void UITouchButton::setOnUp(UITouchButton::ButtonCallback callback) {
    onUpCallback = callback;
}

void UITouchButton::setOnClick(UITouchButton::ButtonCallback callback) {
    onClickCallback = callback;
}

UITouchButton::ButtonCallback UITouchButton::getOnDown() const {
    return onDownCallback;
}

UITouchButton::ButtonCallback UITouchButton::getOnUp() const {
    return onUpCallback;
}

UITouchButton::ButtonCallback UITouchButton::getOnClick() const {
    return onClickCallback;
}

bool UITouchButton::processEvent(const pixelroot32::input::TouchEvent& event) {
    if (!isEnabled() || !isVisible()) {
        return false;
    }
    
    // Only process touch events
    if (event.type != pixelroot32::input::TouchEventType::TouchDown &&
        event.type != pixelroot32::input::TouchEventType::TouchUp &&
        event.type != pixelroot32::input::TouchEventType::Click) {
        return false;
    }
    
    // Check if event is within our bounds
    if (!contains(event.x, event.y)) {
        // If we're in pressed/dragging state and touch moved outside, reset
        if (state == UIWidgetState::Pressed || state == UIWidgetState::Dragging) {
            state = UIWidgetState::Idle;
            clearActive();
        }
        return false;
    }
    
    switch (event.type) {
        case pixelroot32::input::TouchEventType::TouchDown:
            handleTouchDown(event);
            return true;
            
        case pixelroot32::input::TouchEventType::TouchUp:
            handleTouchUp(event);
            return true;
            
        case pixelroot32::input::TouchEventType::Click:
            handleClick(event);
            return true;
            
        default:
            return false;
    }
}

void UITouchButton::reset() {
    state = UIWidgetState::Idle;
    clearActive();
}

void UITouchButton::handleTouchDown(const pixelroot32::input::TouchEvent& event) {
    state = UIWidgetState::Pressed;
    setActive();
    pressStartX = event.x;
    pressStartY = event.y;
    
    if (onDownCallback) {
        onDownCallback();
    }
}

void UITouchButton::handleTouchUp(const pixelroot32::input::TouchEvent& event) {
    // Check if we dragged too far
    int16_t dx = event.x - pressStartX;
    int16_t dy = event.y - pressStartY;
    
    if (dx * dx + dy * dy > DRAG_THRESHOLD * DRAG_THRESHOLD) {
        // Dragged too far - no click
        state = UIWidgetState::Idle;
    } else {
        state = UIWidgetState::Idle;
    }
    
    clearActive();
    
    if (onUpCallback) {
        onUpCallback();
    }
}

void UITouchButton::handleClick(const pixelroot32::input::TouchEvent& event) {
    (void)event;
    if (onClickCallback) {
        onClickCallback();
    }
}

void UITouchButton::setActive() {
    flags = static_cast<UIWidgetFlags>(
        static_cast<uint8_t>(flags) | static_cast<uint8_t>(UIWidgetFlags::Active));
}

void UITouchButton::clearActive() {
    flags = static_cast<UIWidgetFlags>(
        static_cast<uint8_t>(flags) & ~static_cast<uint8_t>(UIWidgetFlags::Active));
}

} // namespace pixelroot32::graphics::ui

#endif // PIXELROOT32_ENABLE_UI_SYSTEM
