/*
 * PixelRoot32 Game Engine
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 *
 * UITouchWidget.h - Base touch widget structure
 * Compact 12-byte structure for touch UI elements
 */
#pragma once

#include "core/EngineModules.h"
#if PIXELROOT32_ENABLE_UI_SYSTEM

#include <cstdint>
#include "input/TouchIncludes.h"

namespace pixelroot32::graphics::ui {

/**
 * @enum UIWidgetType
 * @brief Types of touch UI widgets
 */
enum class UIWidgetType : uint8_t {
    Generic = 0,
    Button = 1,
    Slider = 2,
    Checkbox = 3
};

/**
 * @enum UIWidgetState
 * @brief Current state of a touch widget
 */
enum class UIWidgetState : uint8_t {
    Idle = 0,
    Pressed = 1,
    Hover = 2,
    Dragging = 3
};

/**
 * @enum UIWidgetFlags
 * @brief Flags for widget behavior
 */
enum class UIWidgetFlags : uint8_t {
    None = 0,
    Enabled = 1 << 0,
    Visible = 1 << 1,
    Consumed = 1 << 2,    ///< Event was consumed by this widget
    Active = 1 << 3      ///< Widget is currently being interacted with
};

/**
 * @struct UITouchWidget
 * @brief Base touch widget structure
 */
struct UITouchWidget {
    UIWidgetType type;      ///< Widget type
    UIWidgetState state;    ///< Current state
    UIWidgetFlags flags;   ///< Widget flags
    uint8_t id;             ///< Unique widget ID
    int16_t x;              ///< X position (top-left)
    int16_t y;              ///< Y position (top-left)
    uint16_t width;         ///< Widget width
    uint16_t height;        ///< Widget height
    
    /**
     * @brief Default constructor - creates empty widget
     */
    UITouchWidget()
        : type(UIWidgetType::Generic)
        , state(UIWidgetState::Idle)
        , flags(UIWidgetFlags::None)
        , id(0)
        , x(0)
        , y(0)
        , width(0)
        , height(0) {}
    
    /**
     * @brief Construct touch widget with all fields
     */
    UITouchWidget(UIWidgetType widgetType, uint8_t widgetId, int16_t xPos, int16_t yPos, uint16_t w, uint16_t h)
        : type(widgetType)
        , state(UIWidgetState::Idle)
        , flags(static_cast<UIWidgetFlags>(static_cast<uint8_t>(UIWidgetFlags::Enabled) | 
                                           static_cast<uint8_t>(UIWidgetFlags::Visible)))
        , id(widgetId)
        , x(xPos)
        , y(yPos)
        , width(w)
        , height(h) {}
    
    /**
     * @brief Check if widget is enabled
     * @return true if Enabled flag is set
     */
    bool isEnabled() const {
        return (static_cast<uint8_t>(flags) & static_cast<uint8_t>(UIWidgetFlags::Enabled)) != 0;
    }
    
    /**
     * @brief Check if widget is visible
     * @return true if Visible flag is set
     */
    bool isVisible() const {
        return (static_cast<uint8_t>(flags) & static_cast<uint8_t>(UIWidgetFlags::Visible)) != 0;
    }
    
    /**
     * @brief Check if widget is active (being interacted with)
     * @return true if Active flag is set
     */
    bool isActive() const {
        return (static_cast<uint8_t>(flags) & static_cast<uint8_t>(UIWidgetFlags::Active)) != 0;
    }
    
    /**
     * @brief Check if event was consumed
     * @return true if Consumed flag is set
     */
    bool isConsumed() const {
        return (static_cast<uint8_t>(flags) & static_cast<uint8_t>(UIWidgetFlags::Consumed)) != 0;
    }
    
    /**
     * @brief Enable or disable the widget
     * @param enabled True to enable
     */
    void setEnabled(bool enabled) {
        if (enabled) {
            flags = static_cast<UIWidgetFlags>(
                static_cast<uint8_t>(flags) | static_cast<uint8_t>(UIWidgetFlags::Enabled));
        } else {
            flags = static_cast<UIWidgetFlags>(
                static_cast<uint8_t>(flags) & ~static_cast<uint8_t>(UIWidgetFlags::Enabled));
        }
    }
    
    /**
     * @brief Show or hide the widget
     * @param visible True to show
     */
    void setVisible(bool visible) {
        if (visible) {
            flags = static_cast<UIWidgetFlags>(
                static_cast<uint8_t>(flags) | static_cast<uint8_t>(UIWidgetFlags::Visible));
        } else {
            flags = static_cast<UIWidgetFlags>(
                static_cast<uint8_t>(flags) & ~static_cast<uint8_t>(UIWidgetFlags::Visible));
        }
    }
    
    /**
     * @brief Mark event as consumed
     */
    void consume() {
        flags = static_cast<UIWidgetFlags>(
            static_cast<uint8_t>(flags) | static_cast<uint8_t>(UIWidgetFlags::Consumed));
    }
    
    /**
     * @brief Clear consumed flag for next frame
     */
    void clearConsume() {
        flags = static_cast<UIWidgetFlags>(
            static_cast<uint8_t>(flags) & ~static_cast<uint8_t>(UIWidgetFlags::Consumed));
    }
    
    /**
     * @brief Check if point is inside widget bounds (AABB)
     */
    bool contains(int16_t px, int16_t py) const {
        return px >= x && px < (x + static_cast<int16_t>(width)) &&
               py >= y && py < (y + static_cast<int16_t>(height));
    }
};

/**
 * @brief Maximum number of touch widgets in pool
 */
constexpr uint8_t MAX_UI_ELEMENTS = 16;

// Forward declarations
class UITouchElement;

} // namespace pixelroot32::graphics::ui

#endif // PIXELROOT32_ENABLE_UI_SYSTEM
