/*
 * PixelRoot32 Game Engine
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 *
 * UITouchSlider.h - Touch-optimized slider widget
 * Supports value range 0-100 with drag interaction
 * Now inherits from UITouchElement for Entity interface and draw() support
 */
#pragma once

#include "core/EngineModules.h"
#if PIXELROOT32_ENABLE_UI_SYSTEM

#include <cstdint>
#include "graphics/ui/UITouchElement.h"
#include "graphics/Color.h"
#include "input/TouchEvent.h"

namespace pixelroot32::graphics::ui {

/**
 * @class UITouchSlider
 * @brief Touch-optimized slider widget
 * 
 * Provides slider functionality with touch input support.
 * Inherits from UITouchElement for Entity interface (update/draw).
 * Construct with position/size; register with UIManager::addElement for touch routing.
 * Value range: 0-100
 * States: Idle, Dragging
 * Events: OnValueChanged, OnDragStart, OnDragEnd
 */
class UITouchSlider : public UITouchElement {
public:
    // Callback function types (no std::function for memory efficiency)
    using SliderCallback = void(*)(uint8_t);
    
private:
    SliderCallback onValueChangedCallback;  ///< Called when value changes
    SliderCallback onDragStartCallback;    ///< Called when drag starts
    SliderCallback onDragEndCallback;      ///< Called when drag ends
    
    uint8_t value;                         ///< Current value (0-100)
    uint8_t previousValue;                 ///< Previous value for change detection
    
    pixelroot32::math::Vector2 dragStartPosition;             ///< Position where drag started
    pixelroot32::math::Vector2 currentDragPosition;           ///< Current position during drag
    
    // Rendering properties
    Color trackColor;                     ///< Color for track (line)
    Color thumbColor;                     ///< Color for thumb (handle)
    Color disabledColor;                  ///< Color for disabled state
    
    static constexpr uint8_t MIN_VALUE = 0;
    static constexpr uint8_t MAX_VALUE = 100;
    
public:
    /**
     * @brief Construct a new UITouchSlider
     * @param x X position
     * @param y Y position
     * @param w Width
     * @param h Height
     * @param initialValue Initial value (0-100)
     */
    explicit UITouchSlider(int16_t x, int16_t y, uint16_t w, uint16_t h, uint8_t initialValue);
    
    /**
     * @brief Set track and thumb colors
     * @param track Color for track
     * @param thumb Color for thumb
     */
    void setColors(Color track, Color thumb);
    
    /**
     * @brief Set the OnValueChanged callback
     * @param callback Function to call when value changes (receives new value)
     */
    void setOnValueChanged(SliderCallback callback);
    
    /**
     * @brief Set the OnDragStart callback
     * @param callback Function to call when drag starts
     */
    void setOnDragStart(SliderCallback callback);
    
    /**
     * @brief Set the OnDragEnd callback
     * @param callback Function to call when drag ends
     */
    void setOnDragEnd(SliderCallback callback);
    
    /**
     * @brief Get the OnValueChanged callback
     * @return The current OnValueChanged callback
     */
    SliderCallback getOnValueChanged() const;
    
    /**
     * @brief Get the OnDragStart callback
     * @return The current OnDragStart callback
     */
    SliderCallback getOnDragStart() const;
    
    /**
     * @brief Get the OnDragEnd callback
     * @return The current OnDragEnd callback
     */
    SliderCallback getOnDragEnd() const;
    
    /**
     * @brief Get track color
     * @return Current track color
     */
    Color getTrackColor() const { return trackColor; }
    
    /**
     * @brief Get thumb color
     * @return Current thumb color
     */
    Color getThumbColor() const { return thumbColor; }
    
    /**
     * @brief Get disabled color
     * @return Current disabled color
     */
    Color getDisabledColor() const { return disabledColor; }
    
    /**
     * @brief Get the current value
     * @return Current value (0-100)
     */
    uint8_t getValue() const;
    
    /**
     * @brief Set the value
     * @param newValue New value (0-100)
     */
    void setValue(uint8_t newValue);
    
    /**
     * @brief Get the previous value
     * @return Previous value (0-100)
     */
    uint8_t getPreviousValue() const;
    
    /**
     * @brief Check if value changed since last frame
     * @return true if value changed
     */
    bool hasValueChanged() const;
    
    /**
     * @brief Process a touch event
     * @param event The touch event to process
     * @return true if event was consumed by this slider
     */
    bool processEvent(const pixelroot32::input::TouchEvent& event) override;
    
    /**
     * @brief Render the slider
     * @param renderer Reference to the renderer
     */
    void draw(pixelroot32::graphics::Renderer& renderer) override;
    
    /**
     * @brief Reset slider state
     */
    void reset();
    
private:
    /**
     * @brief Handle touch down event
     * @return true if handled
     */
    bool handleTouchDown(const pixelroot32::input::TouchEvent& event);
    
    /**
     * @brief Handle drag move event
     * @return true if handled
     */
    bool handleDragMove(const pixelroot32::input::TouchEvent& event);
    
    /**
     * @brief Handle touch up event
     * @return true if handled
     */
    bool handleTouchUp(const pixelroot32::input::TouchEvent& event);
    
    /**
     * @brief Update value based on X position
     * @param xPos X position
     */
    void updateValueFromPosition(int16_t xPos);
    
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
