/*
 * PixelRoot32 Game Engine
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 *
 * UITouchButton.h - Touch-optimized button widget
 * Supports touch events: OnDown, OnUp, OnClick
 * Now inherits from UITouchElement for Entity interface and draw() support
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
 * @class UITouchButton
 * @brief Touch-optimized button widget
 * 
 * Provides button functionality with touch input support.
 * Inherits from UITouchElement for Entity interface (update/draw).
 * Construct with position/size; register with UIManager::addElement for touch routing.
 * States: Idle, Pressed, Hover
 * Events: OnDown, OnUp, OnClick
 */
class UITouchButton : public UITouchElement {
public:
    // Callback function types (no std::function for memory efficiency)
    using ButtonCallback = void(*)();
    
    /**
     * @brief Construct a new UITouchButton (new normalized constructor)
     * @param t Button label
     * @param position Position as Vector2
     * @param size Size as Vector2 (width, height)
     * @param callback OnClick callback
     * @param textAlign Text alignment (default: CENTER)
     * @param fontSize Font size in pixels/8 (default: 2)
     */
    UITouchButton(
        std::string_view t, 
        pixelroot32::math::Vector2 position, 
        pixelroot32::math::Vector2 size,
        ButtonCallback callback = nullptr,
        TextAlignment textAlign = TextAlignment::CENTER,
        int fontSize = 2
    );

    /**
     * @brief Construct a new UITouchButton (legacy constructor for backward compatibility)
     * @param t Button label
     * @param x X position
     * @param y Y position
     * @param w Width
     * @param h Height
     */
    UITouchButton(std::string_view t, int16_t x, int16_t y, uint16_t w, uint16_t h);
    
    /**
     * @brief Set the button label
     * @param label String view to the label (no allocation)
     */
    void setLabel(std::string_view label);

    /**
     * @brief Get the current label
     * @return String view to the label (no allocation)
     */
    std::string_view getLabel() const { return label; }

    /**
     * @brief Set button colors
     * @param normal Color for normal state
     * @param pressed Color for pressed state
     * @param disabled Color for disabled state
     */
    void setColors(Color normal, Color pressed, Color disabled);
    
    /**
     * @brief Get normal color
     * @return Normal state color
     */
    Color getNormalColor() const { return normalColor; }
    
    /**
     * @brief Get pressed color
     * @return Pressed state color
     */
    Color getPressedColor() const { return pressedColor; }
    
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
     * @brief Set font size for text rendering
     * @param size Font size (pixels/8, typically 1-4)
     */
    void setFontSize(int size) { fontSize = size; }

    /**
     * @brief Get current font size
     * @return Font size
     */
    int getFontSize() const { return fontSize; }

    /**
     * @brief Set text alignment
     * @param align Text alignment (LEFT, CENTER, RIGHT)
     */
    void setTextAlignment(TextAlignment align) { textAlign = align; }

    /**
     * @brief Get current text alignment
     * @return Text alignment
     */
    TextAlignment getTextAlignment() const { return textAlign; }
    
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
    bool processEvent(const pixelroot32::input::TouchEvent& event) override;
    
    /**
     * @brief Render the button
     * @param renderer Reference to the renderer
     */
    void draw(pixelroot32::graphics::Renderer& renderer) override;
    
    /**
     * @brief Reset button state
     */
    void reset();
    
private:
    ButtonCallback onDownCallback = nullptr;     ///< Called when touch goes down on button
    ButtonCallback onUpCallback = nullptr;       ///< Called when touch goes up on button
    ButtonCallback onClickCallback = nullptr;      ///< Called when button is clicked
    
    pixelroot32::math::Vector2 pressStartPosition;         ///< position where press started
    
    // Rendering properties
    std::string_view label;            ///< Button label (no allocation)
    Color normalColor = Color::White;               ///< Color for normal state
    Color pressedColor = Color::Gray;              ///< Color for pressed state
    Color disabledColor = Color::DarkGray;         ///< Color for disabled state
    Color borderColor = Color::Gray;               ///< Color for button border
    Color disabledBorderColor = Color::DarkGray;        ///< Color for disabled state border
    int fontSize = 2;                      ///< Font size for text rendering
    TextAlignment textAlign = TextAlignment::CENTER;           ///< Text alignment

    static constexpr int16_t DRAG_THRESHOLD = 10;  ///< Drag threshold in pixels
    
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
    
    /**
     * @brief Get color based on current state
     */
    Color getCurrentColor() const;
};

} // namespace pixelroot32::graphics::ui

#endif // PIXELROOT32_ENABLE_UI_SYSTEM
