/*
 * PixelRoot32 Game Engine
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 *
 * UIManager.cpp - Touch UI element manager implementation
 */
#include "core/EngineModules.h"
#if PIXELROOT32_ENABLE_UI_SYSTEM

#include "graphics/ui/UIManager.h"
#include "graphics/ui/UITouchElement.h"
#include "input/TouchEvent.h"

namespace pixelroot32::graphics::ui {

namespace {

UITouchElement* findElementOwningWidget(UITouchElement* const* pointers, const bool* inUse, UITouchWidget* widget) {
    if (widget == nullptr) {
        return nullptr;
    }
    for (uint8_t i = 0; i < UIManager::MAX_ELEMENTS; ++i) {
        if (inUse[i] && pointers[i] != nullptr && &pointers[i]->getWidgetData() == widget) {
            return pointers[i];
        }
    }
    return nullptr;
}

} // namespace

UIManager::UIManager()
    : elementCount(0)
    , nextElementId(1)
    , activeWidget(nullptr)
    , hoverWidget(nullptr)
    , capturedWidget(nullptr) {
    for (uint8_t i = 0; i < MAX_ELEMENTS; ++i) {
        elementPointers[i] = nullptr;
        slotInUse[i] = false;
    }
}

UIManager::~UIManager() {
    clear();
}

bool UIManager::addElement(UITouchElement* element) {
    if (element == nullptr) {
        return false;
    }
    for (uint8_t i = 0; i < MAX_ELEMENTS; ++i) {
        if (slotInUse[i] && elementPointers[i] == element) {
            return false;
        }
    }
    const int8_t slot = findFreeSlot();
    if (slot < 0) {
        return false;
    }
    UITouchWidget& wd = element->getWidgetData();
    if (wd.id == 0) {
        wd.id = nextElementId;
        if (++nextElementId == 0) {
            nextElementId = 1;
        }
    }
    elementPointers[static_cast<uint8_t>(slot)] = element;
    slotInUse[static_cast<uint8_t>(slot)] = true;
    elementCount++;
    return true;
}

bool UIManager::removeElement(uint8_t id) {
    for (uint8_t i = 0; i < MAX_ELEMENTS; ++i) {
        if (slotInUse[i] && elementPointers[i] != nullptr && elementPointers[i]->getWidgetData().id == id) {
            if (capturedWidget == &elementPointers[i]->getWidgetData()) {
                capturedWidget = nullptr;
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
        elementPointers[i] = nullptr;
        slotInUse[i] = false;
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
            const auto evtType = event.getType();

            if (evtType == pixelroot32::input::TouchEventType::DragMove ||
                evtType == pixelroot32::input::TouchEventType::DragEnd ||
                evtType == pixelroot32::input::TouchEventType::TouchUp) {

                UITouchElement* owner = findElementOwningWidget(elementPointers, slotInUse, capturedWidget);
                bool eventConsumed = false;
                if (owner != nullptr) {
                    eventConsumed = owner->processEvent(event);
                }
                UITouchWidget* hitWidget = capturedWidget;

                if (eventConsumed && hitWidget != nullptr) {
                    hitWidget->consume();
                    event.consume();
                    consumed++;
                }

                // If widget ignored the drag event (e.g., UITouchButton doesn't handle DragMove),
                // reset the widget state and release capture
                if (!eventConsumed && evtType == pixelroot32::input::TouchEventType::DragMove && hitWidget != nullptr) {
                    hitWidget->state = pixelroot32::graphics::ui::UIWidgetState::Idle;
                    hitWidget->flags = static_cast<pixelroot32::graphics::ui::UIWidgetFlags>(
                        static_cast<uint8_t>(hitWidget->flags) & ~static_cast<uint8_t>(pixelroot32::graphics::ui::UIWidgetFlags::Active));
                    capturedWidget = nullptr;
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
            const bool eventConsumed = hit->processEvent(event);
            UITouchWidget& widgetData = hit->getWidgetData();

            if (event.getType() == pixelroot32::input::TouchEventType::TouchDown) {
                capturedWidget = &widgetData;
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
}

void UIManager::draw(pixelroot32::graphics::Renderer& renderer) {
    (void)renderer;
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
