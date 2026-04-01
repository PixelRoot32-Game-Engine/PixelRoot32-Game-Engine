/*
 * PixelRoot32 Game Engine
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 *
 * UITouchButton.h - Touch-optimized button widget
 * Supports touch events: OnDown, OnUp, OnClick
 */
#pragma once

#include "core/EngineModules.h"
#if PIXELROOT32_ENABLE_UI_SYSTEM

#include <cstdint>
#include "graphics/ui/UITouchWidget.h"
#include "input/TouchEvent.h"

namespace pixelroot32::graphics::ui {

/**
 * @class UITouchButton
 * @brief Touch-optimized button widget
 * 
 * Provides button functionality with touch input support.
 * States: Idle, Pressed, Hover
 * Events: OnDown, OnUp, OnClick
 */
class UITouchButton : public UITouchWidget {
public:
    // Callback function types (no std::function for memory efficiency)
    using ButtonCallback = void(*)();
    
private:
    ButtonCallback onDownCallback;     ///< Called when touch goes down on button
    ButtonCallback onUpCallback;       ///< Called when touch goes up on button
    ButtonCallback onClickCallback;    ///< Called when button is clicked
    
    int16_t pressStartX;               ///< X position where press started
    int16_t pressStartY;               ///< Y position where press started
    
    static constexpr int16_t DRAG_THRESHOLD = 10;  ///< Drag threshold in pixels
    
public:
    /**
     * @brief Construct a new UITouchButton
     * @param buttonId Unique button ID
     * @param xPos X position (top-left)
     * @param yPos Y position (top-left)
     * @param w Button width
     * @param h Button height
     */
    UITouchButton(uint8_t buttonId, int16_t xPos, int16_t yPos, uint16_t w, uint16_t h)
        : UITouchWidget(UIWidgetType::Button, buttonId, xPos, yPos, w, h)
        , onDownCallback(nullptr)
        , onUpCallback(nullptr)
        , onClickCallback(nullptr)
        , pressStartX(0)
        , pressStartY(0) {}
    
    /**
     * @brief Set the OnDown callback
     * @param callback Function to call when touch goes down
     */
    void setOnDown(ButtonCallback callback);
    
    /**
     * @brief Set the OnUp callback
     * @param callback Function to call when touch goes up
     */
    void setOnUp(ButtonCallback callback);
    
    /**
     * @brief Set the OnClick callback
     * @param callback Function to call when button is clicked
     */
    void setOnClick(ButtonCallback callback);
    
    /**
     * @brief Get the OnDown callback
     * @return The current OnDown callback
     */
    ButtonCallback getOnDown() const;
    
    /**
     * @brief Get the OnUp callback
     * @return The current OnUp callback
     */
    ButtonCallback getOnUp() const;
    
    /**
     * @brief Get the OnClick callback
     * @return The current OnClick callback
     */
    ButtonCallback getOnClick() const;
    
    /**
     * @brief Process a touch event
     * @param event The touch event to process
     * @return true if event was consumed by this button
     */
    bool processEvent(const pixelroot32::input::TouchEvent& event);
    
    /**
     * @brief Reset button state
     */
    void reset();
    
private:
    /**
     * @brief Handle touch down event
     */
    void handleTouchDown(const pixelroot32::input::TouchEvent& event);
    
    /**
     * @brief Handle touch up event
     */
    void handleTouchUp(const pixelroot32::input::TouchEvent& event);
    
    /**
     * @brief Handle click event
     */
    void handleClick(const pixelroot32::input::TouchEvent& event);
    
    /**
     * @brief Set active flag
     */
    void setActive();
    
    /**
     * @brief Clear active flag
     */
    void clearActive();
};

} // namespace pixelroot32::graphics::ui

#endif // PIXELROOT32_ENABLE_UI_SYSTEM
