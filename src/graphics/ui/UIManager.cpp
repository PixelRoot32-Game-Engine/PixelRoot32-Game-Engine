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

UITouchButton* UIManager::addButton(std::string_view t, int16_t x, int16_t y, uint16_t w, uint16_t h) {
    int8_t slot = findFreeSlot();
    if (slot < 0) {
        return nullptr;
    }
    
    // Create UITouchButton directly with position/size (no separate widget)
    void* elementPtr = &elementStorage[ELEMENT_SLOT_BYTES * static_cast<std::size_t>(slot)];
    auto* button = new (elementPtr) UITouchButton(t, x, y, w, h);
    
    elementPointers[slot] = button;  // Store pointer to element (UITouchElement*)
    slotInUse[slot] = true;
    elementCount++;
    return button;
}

UITouchSlider* UIManager::addSlider(int16_t x, int16_t y, uint16_t w, uint16_t h, uint8_t initialValue) {
    int8_t slot = findFreeSlot();
    if (slot < 0) {
        return nullptr;
    }
    
    // Create UITouchSlider directly with position/size (no separate widget)
    void* elementPtr = &elementStorage[ELEMENT_SLOT_BYTES * static_cast<std::size_t>(slot)];
    auto* slider = new (elementPtr) UITouchSlider(x, y, w, h, initialValue);
    
    elementPointers[slot] = slider;  // Store pointer to element (UITouchElement*)
    slotInUse[slot] = true;
    elementCount++;
    return slider;
}

bool UIManager::removeElement(uint8_t id) {
    for (uint8_t i = 0; i < MAX_ELEMENTS; ++i) {
        if (slotInUse[i] && elementPointers[i] != nullptr && elementPointers[i]->getWidgetData().id == id) {
            // Destroy Entity based on widget type
            UITouchWidget& widgetData = elementPointers[i]->getWidgetData();
            void* entityPtr = &elementStorage[ELEMENT_SLOT_BYTES * static_cast<std::size_t>(i)];
            
            switch (widgetData.type) {
                case UIWidgetType::Button: {
                    auto* button = static_cast<UITouchButton*>(entityPtr);
                    button->~UITouchButton();
                    break;
                }
                case UIWidgetType::Slider: {
                    auto* slider = static_cast<UITouchSlider*>(entityPtr);
                    slider->~UITouchSlider();
                    break;
                }
                default:
                    // Destroy as base UITouchElement
                    static_cast<UITouchElement*>(entityPtr)->~UITouchElement();
                    break;
            }
            
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

UITouchElement* UIManager::getElement(uint8_t id) const {
    for (uint8_t i = 0; i < MAX_ELEMENTS; ++i) {
        if (slotInUse[i] && elementPointers[i] != nullptr && elementPointers[i]->getWidgetData().id == id) {
            return elementPointers[i];
        }
    }
    return nullptr;
}

UITouchElement* UIManager::getElementAt(uint8_t index) const {
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
            // Destroy based on widget type
            UITouchWidget& widgetData = elementPointers[i]->getWidgetData();
            void* entityPtr = &elementStorage[ELEMENT_SLOT_BYTES * static_cast<std::size_t>(i)];
            
            switch (widgetData.type) {
                case UIWidgetType::Button: {
                    auto* button = static_cast<UITouchButton*>(entityPtr);
                    button->~UITouchButton();
                    break;
                }
                case UIWidgetType::Slider: {
                    auto* slider = static_cast<UITouchSlider*>(entityPtr);
                    slider->~UITouchSlider();
                    break;
                }
                default:
                    // Destroy as base UITouchElement
                    static_cast<UITouchElement*>(entityPtr)->~UITouchElement();
                    break;
            }
            
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
    
    UITouchElement* elements[MAX_ELEMENTS];
    uint8_t activeCount = 0;
    
    for (uint8_t i = 0; i < MAX_ELEMENTS; ++i) {
        if (slotInUse[i] && elementPointers[i] != nullptr) {
            elements[activeCount++] = elementPointers[i];
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
                UITouchWidget* hitWidget = capturedWidget;
                
                // Find the element that owns this widget
                for (uint8_t j = 0; j < MAX_ELEMENTS; ++j) {
                    if (slotInUse[j] && elementPointers[j] != nullptr && 
                        &elementPointers[j]->getWidgetData() == hitWidget) {
                        
                        if (hitWidget->type == UIWidgetType::Button) {
                            auto* button = static_cast<UITouchButton*>(elementPointers[j]);
                            eventConsumed = button->processEvent(event);
                        } else if (hitWidget->type == UIWidgetType::Slider) {
                            auto* slider = static_cast<UITouchSlider*>(elementPointers[j]);
                            eventConsumed = slider->processEvent(event);
                        }
                        break;
                    }
                }
                
                if (eventConsumed) {
                    hitWidget->consume();
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
        
        UITouchElement* hit = UIHitTest::findHit(elements, activeCount, event.x, event.y);
        
        if (hit != nullptr) {
            bool eventConsumed = false;
            UITouchWidget& widgetData = hit->getWidgetData();
            
            if (widgetData.type == UIWidgetType::Button) {
                auto* button = static_cast<UITouchButton*>(hit);
                eventConsumed = button->processEvent(event);
                
                if (event.type == pixelroot32::input::TouchEventType::TouchDown) {
                    capturedWidget = &widgetData;
                }
            } else if (widgetData.type == UIWidgetType::Slider) {
                auto* slider = static_cast<UITouchSlider*>(hit);
                eventConsumed = slider->processEvent(event);
                
                if (event.type == pixelroot32::input::TouchEventType::TouchDown) {
                    capturedWidget = &widgetData;
                }
            }
            
            if (eventConsumed) {
                widgetData.consume();
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

UITouchElement** UIManager::getElements() {
    return elementPointers;
}

UITouchElement* const* UIManager::getElements() const {
    return elementPointers;
}

void UIManager::updateHover(int16_t x, int16_t y) {
    // Collect active elements
    const UITouchElement* elements[MAX_ELEMENTS];
    uint8_t activeCount = 0;
    
    for (uint8_t i = 0; i < MAX_ELEMENTS; ++i) {
        if (slotInUse[i] && elementPointers[i] != nullptr) {
            elements[activeCount++] = elementPointers[i];
        }
    }
    
    const UITouchElement* hit = UIHitTest::findHit(elements, activeCount, x, y);
    if (hit != nullptr) {
        hoverWidget = const_cast<UITouchWidget*>(&hit->getWidgetData());
    } else {
        hoverWidget = nullptr;
    }
}

void UIManager::clearConsumeFlags() {
    for (uint8_t i = 0; i < MAX_ELEMENTS; ++i) {
        if (slotInUse[i] && elementPointers[i] != nullptr) {
            elementPointers[i]->getWidgetData().clearConsume();
        }
    }
}

UITouchWidget* UIManager::getCapturedWidget() const {
    return capturedWidget;
}

void UIManager::releaseCapture() {
    capturedWidget = nullptr;
}

void UIManager::update(unsigned long deltaTime) {
    (void)deltaTime;
    // When manualRenderUpdate is true, Scene/Layout handles the update
    if (manualRenderUpdate) {
        return;
    }
    // Update all active elements directly
    for (uint8_t i = 0; i < MAX_ELEMENTS; ++i) {
        if (slotInUse[i] && elementPointers[i] != nullptr) {
            elementPointers[i]->update(deltaTime);
        }
    }
}

void UIManager::draw(pixelroot32::graphics::Renderer& renderer) {
    // When manualRenderUpdate is true, Scene/Layout handles the draw
    if (manualRenderUpdate) {
        return;
    }
    // Draw all active elements
    for (uint8_t i = 0; i < MAX_ELEMENTS; ++i) {
        if (slotInUse[i] && elementPointers[i] != nullptr) {
            elementPointers[i]->draw(renderer);
        }
    }
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
