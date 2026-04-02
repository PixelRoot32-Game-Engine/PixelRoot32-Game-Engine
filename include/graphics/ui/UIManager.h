/*
 * PixelRoot32 Game Engine
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 *
 * UIManager.h - Touch UI element registry
 * Holds non-owning pointers (max 16) for hit-testing and touch dispatch.
 * Widget lifetime is owned by the Scene (e.g. std::unique_ptr, arena).
 */
#pragma once

#include "core/EngineModules.h"
#if PIXELROOT32_ENABLE_UI_SYSTEM

#include <cstdint>
#include "graphics/ui/UIHitTest.h"
#include "input/TouchEvent.h"

namespace pixelroot32::graphics {
class Renderer;
}

namespace pixelroot32::graphics::ui {

class UITouchElement;
class UITouchWidget;

/**
 * @class UIManager
 * @brief Registry of touch UI elements for event routing (non-owning pointers).
 *
 * The scene (or another owner) constructs widgets and registers them with addElement.
 * clear/removeElement only unregister pointers — they never destroy objects.
 */
class UIManager {
public:
    static constexpr uint8_t MAX_ELEMENTS = 16;

private:
    UITouchElement* elementPointers[MAX_ELEMENTS];
    uint8_t elementCount;
    uint8_t nextElementId;
    UITouchWidget* activeWidget;
    UITouchWidget* hoverWidget;
    UITouchWidget* capturedWidget;
    bool slotInUse[MAX_ELEMENTS];

public:
    UIManager();
    ~UIManager();

    /**
     * @brief Register an element for touch hit-testing and processEvents.
     * @param element Non-null; must outlive registration (or until remove/clear).
     * @return false if full, duplicate pointer, or element is null
     */
    bool addElement(UITouchElement* element);

    bool removeElement(uint8_t id);
    bool removeElement(UITouchWidget* widget);

    UITouchElement* getElement(uint8_t id) const;
    UITouchElement* getElementAt(uint8_t index) const;

    uint8_t getElementCount() const;
    uint8_t getMaxElements() const;
    bool isFull() const;

    void clear();

    uint8_t processEvents(pixelroot32::input::TouchEvent* events, uint8_t count);
    bool processEvent(pixelroot32::input::TouchEvent& event);

    UITouchWidget* getActiveWidget() const;
    UITouchWidget* getHoverWidget() const;

    UITouchElement** getElements();
    UITouchElement* const* getElements() const;

    void updateHover(int16_t x, int16_t y);
    void clearConsumeFlags();

    UITouchWidget* getCapturedWidget() const;
    void releaseCapture();

    /** @deprecated Touch widgets are updated via Entity/Scene; kept as no-op for compatibility. */
    void update(unsigned long deltaTime);

    /** @deprecated Touch widgets are drawn via Entity/Scene; kept as no-op for compatibility. */
    void draw(pixelroot32::graphics::Renderer& renderer);

private:
    int8_t findFreeSlot() const;
};

} // namespace pixelroot32::graphics::ui

#endif // PIXELROOT32_ENABLE_UI_SYSTEM
