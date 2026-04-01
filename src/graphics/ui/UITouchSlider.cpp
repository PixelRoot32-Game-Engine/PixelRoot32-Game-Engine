/*
 * PixelRoot32 Game Engine
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 *
 * UITouchSlider.cpp - Touch-optimized slider widget implementation
 */
#include "core/EngineModules.h"
#if PIXELROOT32_ENABLE_UI_SYSTEM

#include "graphics/ui/UITouchSlider.h"
#include "input/TouchEvent.h"

namespace pixelroot32::graphics::ui {

void UITouchSlider::setOnValueChanged(UITouchSlider::SliderCallback callback) {
    onValueChangedCallback = callback;
}

void UITouchSlider::setOnDragStart(UITouchSlider::SliderCallback callback) {
    onDragStartCallback = callback;
}

void UITouchSlider::setOnDragEnd(UITouchSlider::SliderCallback callback) {
    onDragEndCallback = callback;
}

UITouchSlider::SliderCallback UITouchSlider::getOnValueChanged() const {
    return onValueChangedCallback;
}

UITouchSlider::SliderCallback UITouchSlider::getOnDragStart() const {
    return onDragStartCallback;
}

UITouchSlider::SliderCallback UITouchSlider::getOnDragEnd() const {
    return onDragEndCallback;
}

uint8_t UITouchSlider::getValue() const {
    return value;
}

void UITouchSlider::setValue(uint8_t newValue) {
    previousValue = value;
    value = (newValue > MAX_VALUE) ? MAX_VALUE : newValue;
    
    if (value != previousValue && onValueChangedCallback) {
        onValueChangedCallback(value);
    }
}

uint8_t UITouchSlider::getPreviousValue() const {
    return previousValue;
}

bool UITouchSlider::hasValueChanged() const {
    return value != previousValue;
}

bool UITouchSlider::processEvent(const pixelroot32::input::TouchEvent& event) {
    if (!isEnabled() || !isVisible()) {
        return false;
    }
    
    // Only process touch down/move/up events
    if (event.type != pixelroot32::input::TouchEventType::TouchDown &&
        event.type != pixelroot32::input::TouchEventType::DragMove &&
        event.type != pixelroot32::input::TouchEventType::TouchUp) {
        return false;
    }
    
    switch (event.type) {
        case pixelroot32::input::TouchEventType::TouchDown:
            return handleTouchDown(event);
            
        case pixelroot32::input::TouchEventType::DragMove:
            return handleDragMove(event);
            
        case pixelroot32::input::TouchEventType::TouchUp:
            return handleTouchUp(event);
            
        default:
            return false;
    }
}

void UITouchSlider::reset() {
    state = UIWidgetState::Idle;
    clearActive();
}

bool UITouchSlider::handleTouchDown(const pixelroot32::input::TouchEvent& event) {
    // Check if touch is within slider bounds
    if (!contains(event.x, event.y)) {
        return false;
    }
    
    state = UIWidgetState::Dragging;
    setActive();
    dragStartX = event.x;
    currentDragX = event.x;
    
    // Calculate initial value based on touch position
    updateValueFromPosition(event.x);
    
    if (onDragStartCallback) {
        onDragStartCallback(value);
    }
    
    return true;
}

bool UITouchSlider::handleDragMove(const pixelroot32::input::TouchEvent& event) {
    if (state != UIWidgetState::Dragging) {
        return false;
    }
    
    currentDragX = event.x;
    updateValueFromPosition(event.x);
    
    return true;
}

bool UITouchSlider::handleTouchUp(const pixelroot32::input::TouchEvent& event) {
    (void)event;
    if (state != UIWidgetState::Dragging) {
        return false;
    }
    
    state = UIWidgetState::Idle;
    clearActive();
    
    if (onDragEndCallback) {
        onDragEndCallback(value);
    }
    
    return true;
}

void UITouchSlider::updateValueFromPosition(int16_t xPos) {
    // Calculate position within slider (accounting for padding)
    int16_t sliderLeft = x + 4;   // 4 pixel padding
    int16_t sliderRight = x + static_cast<int16_t>(width) - 4;
    
    if (xPos <= sliderLeft) {
        setValue(MIN_VALUE);
    } else if (xPos >= sliderRight) {
        setValue(MAX_VALUE);
    } else {
        // Map position to value
        int16_t range = sliderRight - sliderLeft;
        int16_t offset = xPos - sliderLeft;
        uint8_t newValue = static_cast<uint8_t>((offset * MAX_VALUE) / range);
        setValue(newValue);
    }
}

void UITouchSlider::setActive() {
    flags = static_cast<UIWidgetFlags>(
        static_cast<uint8_t>(flags) | static_cast<uint8_t>(UIWidgetFlags::Active));
}

void UITouchSlider::clearActive() {
    flags = static_cast<UIWidgetFlags>(
        static_cast<uint8_t>(flags) & ~static_cast<uint8_t>(UIWidgetFlags::Active));
}

} // namespace pixelroot32::graphics::ui

#endif // PIXELROOT32_ENABLE_UI_SYSTEM
