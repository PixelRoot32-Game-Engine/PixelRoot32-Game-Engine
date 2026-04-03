/*
 * PixelRoot32 Game Engine
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 *
 * UITouchCheckbox.h - Touch-optimized checkbox widget
 * Supports touch events and state change callbacks
 */
#pragma once

#include "core/EngineModules.h"
#if PIXELROOT32_ENABLE_UI_SYSTEM

#include <cstdint>
#include <string_view>
#include "graphics/ui/UITouchElement.h"
#include "graphics/Color.h"
#include "input/TouchEvent.h"
#include "math/Vector2.h"

namespace pixelroot32::graphics::ui {

/**
 * @class UITouchCheckbox
 * @brief Touch-optimized checkbox widget
 * 
 * Provides checkbox functionality with touch input support.
 * Inherits from UITouchElement for Entity interface (update/draw).
 * Construct with position/size; register with UIManager::addElement for touch routing.
 * States: Idle, Pressed (transient), checked/unchecked
 * Events: OnChanged (when checked state changes)
 */
class UITouchCheckbox : public UITouchElement {
public:
    // Callback function type (no std::function for memory efficiency)
    using CheckboxCallback = void(*)(bool checked);
    
private:
    CheckboxCallback onChangedCallback;  ///< Called when checked state changes
    
    // Checkbox state
    bool checked;                        ///< Current checked state
    
    // Rendering properties
    std::string_view label;            ///< Checkbox label (no allocation)
    Color normalColor;                 ///< Color for normal state
    Color checkedColor;                ///< Color for checked state
    Color disabledColor;               ///< Color for disabled state
    Color borderColor;                 ///< Color for checkbox border
    Color disabledBorderColor;         ///< Color for disabled state border
    
    int fontSize;                      ///< Font size for label rendering
    
    pixelroot32::math::Vector2 pressStartPosition;  ///< Position where press started
    static constexpr int16_t DRAG_THRESHOLD = 10;    ///< Drag threshold in pixels
    
public:
    /**
     * @brief Construct a new UITouchCheckbox
     * @param label Checkbox label text
     * @param x X position
     * @param y Y position
     * @param w Width
     * @param h Height
     * @param initialChecked Initial checked state (default: false)
     */
    explicit UITouchCheckbox(std::string_view label, int16_t x, int16_t y, 
                             uint16_t w, uint16_t h, bool initialChecked = false);
    
    /**
     * @brief Set the checkbox label
     * @param label String view to the label (no allocation)
     */
    void setLabel(std::string_view label);
    
    /**
     * @brief Get the current label
     * @return String view to the label (no allocation)
     */
    std::string_view getLabel() const { return label; }
    
    /**
     * @brief Set the checked state
     * @param checked True to check, false to uncheck
     */
    void setChecked(bool checked);
    
    /**
     * @brief Get the current checked state
     * @return True if checked
     */
    bool isChecked() const { return checked; }
    
    /**
     * @brief Toggle the checked state
     */
    void toggle();
    
    /**
     * @brief Set checkbox colors
     * @param normal Color for normal/unchecked state
     * @param checked Color for checked state
     * @param disabled Color for disabled state
     */
    void setColors(Color normal, Color checked, Color disabled);
    
    /**
     * @brief Get normal color
     * @return Normal state color
     */
    Color getNormalColor() const { return normalColor; }
    
    /**
     * @brief Get checked color
     * @return Checked state color
     */
    Color getCheckedColor() const { return checkedColor; }
    
    /**
     * @brief Get disabled color
     * @return Disabled state color
     */
    Color getDisabledColor() const { return disabledColor; }
    
    /**
     * @brief Get border color
     * @return Border color
     */
    Color getBorderColor() const { return borderColor; }
    
    /**
     * @brief Get disabled border color
     * @return Disabled border color
     */
    Color getDisabledBorderColor() const { return disabledBorderColor; }
    
    /**
     * @brief Set the OnChanged callback
     * @param callback Function to call when checked state changes
     */
    void setOnChanged(CheckboxCallback callback);
    
    /**
     * @brief Get the OnChanged callback
     * @return The current OnChanged callback
     */
    CheckboxCallback getOnChanged() const;
    
    /**
     * @brief Process a touch event
     * @param event The touch event to process
     * @return true if event was consumed by this checkbox
     */
    bool processEvent(const pixelroot32::input::TouchEvent& event) override;
    
    /**
     * @brief Render the checkbox
     * @param renderer Reference to the renderer
     */
    void draw(pixelroot32::graphics::Renderer& renderer) override;
    
    /**
     * @brief Reset checkbox state
     */
    void reset();
    
private:
    /**
     * @brief Handle touch down event
     */
    void handleTouchDown(const pixelroot32::input::TouchEvent& event);
    
    /**
     * @brief Handle touch up event (triggers toggle if within bounds)
     */
    void handleTouchUp(const pixelroot32::input::TouchEvent& event);
    
    /**
     * @brief Set active flag (visual pressed state)
     */
    void setActive();
    
    /**
     * @brief Clear active flag
     */
    void clearActive();
    
    /**
     * @brief Get color based on current state
     */
    Color getCurrentColor() const;
};

} // namespace pixelroot32::graphics::ui

#endif // PIXELROOT32_ENABLE_UI_SYSTEM
