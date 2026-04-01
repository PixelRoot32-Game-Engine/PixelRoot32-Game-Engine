/*
 * PixelRoot32 Game Engine
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 *
 * UIManager.cpp - Touch UI element manager implementation
 */
#include "core/EngineModules.h"
#if PIXELROOT32_ENABLE_UI_SYSTEM

#include <new>  // For placement new
#include "graphics/ui/UIManager.h"
#include "graphics/ui/UITouchButton.h"
#include "graphics/ui/UITouchSlider.h"
#include "input/TouchEvent.h"

namespace pixelroot32::graphics::ui {

UIManager::UIManager()
    : elementCount(0)
    , nextElementId(1)
    , activeWidget(nullptr)
    , hoverWidget(nullptr)
    , capturedWidget(nullptr) {
    // Initialize storage and pointers
    for (uint8_t i = 0; i < MAX_ELEMENTS; ++i) {
        elementPointers[i] = nullptr;
        slotInUse[i] = false;
    }
}

UIManager::~UIManager() {
    clear();
}

void UIManager::destroyWidgetAt(UITouchWidget* widget) {
    if (widget == nullptr) {
        return;
    }
    switch (widget->type) {
        case UIWidgetType::Button:
            static_cast<UITouchButton*>(widget)->~UITouchButton();
            break;
        case UIWidgetType::Slider:
            static_cast<UITouchSlider*>(widget)->~UITouchSlider();
            break;
        default:
            widget->~UITouchWidget();
            break;
    }
}

UITouchButton* UIManager::addButton(int16_t x, int16_t y, uint16_t w, uint16_t h) {
    int8_t slot = findFreeSlot();
    if (slot < 0) {
        return nullptr;
    }
    
    void* ptr = &elementStorage[ELEMENT_SLOT_BYTES * static_cast<std::size_t>(slot)];
    auto* button = new (ptr) UITouchButton(nextElementId++, x, y, w, h);
    elementPointers[slot] = button;
    slotInUse[slot] = true;
    elementCount++;
    return button;
}

UITouchSlider* UIManager::addSlider(int16_t x, int16_t y, uint16_t w, uint16_t h, uint8_t initialValue) {
    int8_t slot = findFreeSlot();
    if (slot < 0) {
        return nullptr;
    }
    
    void* ptr = &elementStorage[ELEMENT_SLOT_BYTES * static_cast<std::size_t>(slot)];
    auto* slider = new (ptr) UITouchSlider(nextElementId++, x, y, w, h, initialValue);
    elementPointers[slot] = slider;
    slotInUse[slot] = true;
    elementCount++;
    return slider;
}

bool UIManager::addElement(UITouchWidget* widget) {
    if (widget == nullptr) {
        return false;
    }
    
    int8_t slot = findFreeSlot();
    if (slot < 0) {
        return false;
    }
    
    elementPointers[slot] = widget;
    slotInUse[slot] = true;
    elementCount++;
    return true;
}

bool UIManager::removeElement(uint8_t id) {
    for (uint8_t i = 0; i < MAX_ELEMENTS; ++i) {
        if (slotInUse[i] && elementPointers[i] != nullptr && elementPointers[i]->id == id) {
            // Call destructor for proper cleanup
            elementPointers[i]->~UITouchWidget();
            
            elementPointers[i] = nullptr;
            slotInUse[i] = false;
            elementCount--;
            return true;
        }
    }
    return false;
}

bool UIManager::removeElement(UITouchWidget* widget) {
    if (widget == nullptr) {
        return false;
    }
    return removeElement(widget->id);
}

UITouchWidget* UIManager::getElement(uint8_t id) const {
    for (uint8_t i = 0; i < MAX_ELEMENTS; ++i) {
        if (slotInUse[i] && elementPointers[i] != nullptr && elementPointers[i]->id == id) {
            return elementPointers[i];
        }
    }
    return nullptr;
}

UITouchWidget* UIManager::getElementAt(uint8_t index) const {
    if (index >= MAX_ELEMENTS || !slotInUse[index]) {
        return nullptr;
    }
    return elementPointers[index];
}

uint8_t UIManager::getElementCount() const {
    return elementCount;
}

uint8_t UIManager::getMaxElements() const {
    return MAX_ELEMENTS;
}

bool UIManager::isFull() const {
    return elementCount >= MAX_ELEMENTS;
}

void UIManager::clear() {
    for (uint8_t i = 0; i < MAX_ELEMENTS; ++i) {
        if (slotInUse[i] && elementPointers[i] != nullptr) {
            destroyWidgetAt(elementPointers[i]);
            elementPointers[i] = nullptr;
            slotInUse[i] = false;
        }
    }
    elementCount = 0;
    activeWidget = nullptr;
    hoverWidget = nullptr;
    capturedWidget = nullptr;
}

uint8_t UIManager::processEvents(pixelroot32::input::TouchEvent* events, uint8_t count) {
    uint8_t consumed = 0;
    
    const UITouchWidget* widgets[MAX_ELEMENTS];
    uint8_t activeCount = 0;
    
    for (uint8_t i = 0; i < MAX_ELEMENTS; ++i) {
        if (slotInUse[i] && elementPointers[i] != nullptr) {
            widgets[activeCount++] = elementPointers[i];
        }
    }
    
    for (uint8_t i = 0; i < count; ++i) {
        auto& event = events[i];
        
        if (event.isConsumed()) {
            continue;
        }
        
        if (capturedWidget != nullptr) {
            const auto evtType = event.type;
            
            if (evtType == pixelroot32::input::TouchEventType::DragMove ||
                evtType == pixelroot32::input::TouchEventType::DragEnd ||
                evtType == pixelroot32::input::TouchEventType::TouchUp) {
                
                bool eventConsumed = false;
                UITouchWidget* hit = capturedWidget;
                
                if (hit->type == UIWidgetType::Button) {
                    auto* button = reinterpret_cast<UITouchButton*>(hit);
                    eventConsumed = button->processEvent(event);
                } else if (hit->type == UIWidgetType::Slider) {
                    auto* slider = reinterpret_cast<UITouchSlider*>(hit);
                    eventConsumed = slider->processEvent(event);
                }
                
                if (eventConsumed) {
                    hit->consume();
                    event.consume();
                    consumed++;
                }
                
                if (evtType == pixelroot32::input::TouchEventType::DragEnd ||
                    evtType == pixelroot32::input::TouchEventType::TouchUp) {
                    capturedWidget = nullptr;
                }
                
                continue;
            }
        }
        
        const UITouchWidget* hit = UIHitTest::findHit(widgets, activeCount, event.x, event.y);
        
        if (hit != nullptr) {
            bool eventConsumed = false;
            
            if (hit->type == UIWidgetType::Button) {
                auto* button = const_cast<UITouchButton*>(reinterpret_cast<const UITouchButton*>(hit));
                eventConsumed = button->processEvent(event);
                
                if (event.type == pixelroot32::input::TouchEventType::TouchDown) {
                    capturedWidget = const_cast<UITouchWidget*>(hit);
                }
            } else if (hit->type == UIWidgetType::Slider) {
                auto* slider = const_cast<UITouchSlider*>(reinterpret_cast<const UITouchSlider*>(hit));
                eventConsumed = slider->processEvent(event);
                
                if (event.type == pixelroot32::input::TouchEventType::TouchDown) {
                    capturedWidget = const_cast<UITouchWidget*>(hit);
                }
            }
            
            if (eventConsumed) {
                const_cast<UITouchWidget*>(hit)->consume();
                event.consume();
                consumed++;
            }
        }
    }
    
    return consumed;
}

bool UIManager::processEvent(pixelroot32::input::TouchEvent& event) {
    return processEvents(&event, 1) > 0;
}

UITouchWidget* UIManager::getActiveWidget() const {
    return activeWidget;
}

UITouchWidget* UIManager::getHoverWidget() const {
    return hoverWidget;
}

UITouchWidget** UIManager::getElements() {
    return elementPointers;
}

UITouchWidget* const* UIManager::getElements() const {
    return elementPointers;
}

void UIManager::updateHover(int16_t x, int16_t y) {
    // Collect active widgets
    const UITouchWidget* widgets[MAX_ELEMENTS];
    uint8_t activeCount = 0;
    
    for (uint8_t i = 0; i < MAX_ELEMENTS; ++i) {
        if (slotInUse[i] && elementPointers[i] != nullptr) {
            widgets[activeCount++] = elementPointers[i];
        }
    }
    
    hoverWidget = const_cast<UITouchWidget*>(UIHitTest::findHit(widgets, activeCount, x, y));
}

void UIManager::clearConsumeFlags() {
    for (uint8_t i = 0; i < MAX_ELEMENTS; ++i) {
        if (slotInUse[i] && elementPointers[i] != nullptr) {
            elementPointers[i]->clearConsume();
        }
    }
}

UITouchWidget* UIManager::getCapturedWidget() const {
    return capturedWidget;
}

void UIManager::releaseCapture() {
    capturedWidget = nullptr;
}

int8_t UIManager::findFreeSlot() const {
    for (uint8_t i = 0; i < MAX_ELEMENTS; ++i) {
        if (!slotInUse[i]) {
            return static_cast<int8_t>(i);
        }
    }
    return -1;
}

} // namespace pixelroot32::graphics::ui

#endif // PIXELROOT32_ENABLE_UI_SYSTEM
