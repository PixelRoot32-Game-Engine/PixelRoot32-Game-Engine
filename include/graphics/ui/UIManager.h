/*
 * PixelRoot32 Game Engine
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 *
 * UIManager.h - Touch UI element manager
 * Manages element pool (max 16) with add/remove/get operations
 * Uses static allocation - NO dynamic memory
 */
#pragma once

#include "core/EngineModules.h"
#if PIXELROOT32_ENABLE_UI_SYSTEM

#include <cstddef>
#include <cstdint>
#include "graphics/ui/UITouchButton.h"
#include "graphics/ui/UITouchSlider.h"
#include "graphics/ui/UIHitTest.h"
#include "input/TouchEvent.h"

namespace pixelroot32::graphics::ui {

/**
 * @class UIManager
 * @brief Manages touch UI element pool
 * 
 * Static element pool (max 16 elements) for zero-allocation UI.
 * Uses placement new for in-place construction - NO heap allocation.
 * Provides addElement, removeElement, getElement, and processEvents operations.
 */
class UIManager {
public:
    // Maximum number of elements in pool
    static constexpr uint8_t MAX_ELEMENTS = 16;
    /** Bytes per pool slot — must fit the largest concrete widget (placement new). */
    static constexpr std::size_t ELEMENT_SLOT_BYTES =
        (sizeof(UITouchButton) > sizeof(UITouchSlider)) ? sizeof(UITouchButton) : sizeof(UITouchSlider);
    
private:
    // Element pool - static allocation, no dynamic memory
    alignas(UITouchButton) alignas(UITouchSlider) char elementStorage[ELEMENT_SLOT_BYTES * MAX_ELEMENTS];
    
    // Pointers to elements in the pool
    UITouchWidget* elementPointers[MAX_ELEMENTS];
    
    // Number of active elements
    uint8_t elementCount;
    
    // Next available element ID
    uint8_t nextElementId;
    
    // Currently focused/active widget
    UITouchWidget* activeWidget;
    
    // Widget being hovered
    UITouchWidget* hoverWidget;
    
    // Captured widget for drag tracking - element that received initial TouchDown
    UITouchWidget* capturedWidget;
    
    // Track which slots are in use
    bool slotInUse[MAX_ELEMENTS];
    
public:
    /**
     * @brief Construct a new UIManager
     */
    UIManager();
    
    /**
     * @brief Destructor - clears all elements
     */
    ~UIManager();
    
    /**
     * @brief Add a button to the manager (in-place construction)
     * @param x X position (top-left)
     * @param y Y position (top-left)
     * @param w Button width
     * @param h Button height
     * @return Pointer to added button, or nullptr if pool is full
     */
    UITouchButton* addButton(int16_t x, int16_t y, uint16_t w, uint16_t h);
    
    /**
     * @brief Add a slider to the manager (in-place construction)
     * @param x X position (top-left)
     * @param y Y position (top-left)
     * @param w Slider width
     * @param h Slider height
     * @param initialValue Initial value (0-100)
     * @return Pointer to added slider, or nullptr if pool is full
     */
    UITouchSlider* addSlider(int16_t x, int16_t y, uint16_t w, uint16_t h, uint8_t initialValue = 50);
    
    /**
     * @brief Add a widget directly to the pool (in-place construction)
     * @param widget Pointer to widget to add
     * @return true if added successfully
     */
    bool addElement(UITouchWidget* widget);
    
    /**
     * @brief Remove an element by ID
     * @param id Element ID to remove
     * @return true if removed successfully
     */
    bool removeElement(uint8_t id);
    
    /**
     * @brief Remove a specific widget
     * @param widget Pointer to widget to remove
     * @return true if removed successfully
     */
    bool removeElement(UITouchWidget* widget);
    
    /**
     * @brief Get an element by ID
     * @param id Element ID to find
     * @return Pointer to widget, or nullptr if not found
     */
    UITouchWidget* getElement(uint8_t id) const;
    
    /**
     * @brief Get an element by index
     * @param index Element index (0-based)
     * @return Pointer to widget, or nullptr if index out of range or empty
     */
    UITouchWidget* getElementAt(uint8_t index) const;
    
    /**
     * @brief Get the number of active elements
     * @return Current element count
     */
    uint8_t getElementCount() const;
    
    /**
     * @brief Get the maximum number of elements
     * @return Maximum element capacity
     */
    uint8_t getMaxElements() const;
    
    /**
     * @brief Check if pool is full
     * @return true if no more elements can be added
     */
    bool isFull() const;
    
    /**
     * @brief Clear all elements from the manager
     */
    void clear();
    
    /**
     * @brief Process touch events through all widgets (UI-first dispatch).
     *
     * Events handled by a widget are marked consumed via TouchEvent::consume()
     * so downstream consumers (e.g. ActorTouchController) can skip them.
     *
     * @param events Mutable array — consumed flag is set in-place
     * @param count  Number of events
     * @return Number of events consumed by UI widgets
     */
    uint8_t processEvents(pixelroot32::input::TouchEvent* events, uint8_t count);
    
    /**
     * @brief Process a single touch event
     * @param event Mutable ref — consumed flag set if handled
     * @return true if event was consumed
     */
    bool processEvent(pixelroot32::input::TouchEvent& event);
    
    /**
     * @brief Get the currently active widget
     * @return Pointer to active widget, or nullptr
     */
    UITouchWidget* getActiveWidget() const;
    
    /**
     * @brief Get the widget under the cursor
     * @return Pointer to hover widget, or nullptr
     */
    UITouchWidget* getHoverWidget() const;
    
    /**
     * @brief Get all elements as array (for iteration)
     * @return Pointer to first element in pool
     */
    UITouchWidget** getElements();
    
    /**
     * @brief Get all elements as const array
     * @return Pointer to first element in pool
     */
    UITouchWidget* const* getElements() const;
    
    /**
     * @brief Update hover state for a position
     * @param x X position
     * @param y Y position
     */
    void updateHover(int16_t x, int16_t y);
    
    /**
     * @brief Clear consume flags for all widgets (call each frame)
     */
    void clearConsumeFlags();
    
    /**
     * @brief Get the currently captured widget (for drag tracking)
     * @return Pointer to captured widget, or nullptr
     */
    UITouchWidget* getCapturedWidget() const;
    
    /**
     * @brief Release captured widget
     */
    void releaseCapture();
    
private:
    /**
     * @brief Find a free slot in the pool
     * @return Slot index, or -1 if no free slots
     */
    int8_t findFreeSlot() const;

    static void destroyWidgetAt(UITouchWidget* widget);
};

} // namespace pixelroot32::graphics::ui

#endif // PIXELROOT32_ENABLE_UI_SYSTEM
