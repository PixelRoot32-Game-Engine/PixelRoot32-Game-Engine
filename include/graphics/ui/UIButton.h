/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#pragma once
#include "graphics/ui/UIElement.h"
#include "graphics/Color.h"
#include <string>
#include <functional>
#include "input/InputManager.h"

namespace pixelroot32::graphics::ui {

/**
    * @class UIButton
    * @brief A clickable button UI element.
    *
    * Supports both physical (keyboard/gamepad) and touch input.
    * Can trigger a callback function when pressed.
    */
class UIButton : public UIElement {
private:
    std::string label;
    Color textColor;
    Color backgroundColor;
    bool isSelected = false;
    bool hasBackground = true;
    uint8_t index;
    TextAlignment textAlign = TextAlignment::CENTER;
    int fontSize = 2;
    std::function<void()> onClick;

    /**
        * @brief Internal helper to check if a point is inside the button's bounds.
        * @param px Point X coordinate.
        * @param py Point Y coordinate.
        * @return true if point is inside.
        */
    bool isPointInside(int px, int py) const;

public:
    /**
        * @brief Constructs a new UIButton.
        * @param t Button label text.
        * @param textAlign Text alignment.
        * @param fontSize Text size multiplier.
        * @param index Navigation index (for D-pad navigation).
        * @param x X position.
        * @param y Y position.
        * @param w Width.
        * @param h Height.
        * @param callback Function to call when clicked/pressed.
        */
    UIButton(std::string t, uint8_t index, float x, float y, float w, float h, std::function<void()> callback, TextAlignment textAlign = TextAlignment::CENTER, int fontSize = 2);

    /**
        * @brief Configures the button's visual style.
        * @param textCol Color of the text.
        * @param bgCol Color of the background.
        * @param drawBg Whether to draw the background rectangle.
        */
    void setStyle(Color textCol, Color bgCol, bool drawBg);

    /**
        * @brief Sets the selection state (e.g., focused via D-pad).
        * @param selected True if selected.
        */
    void setSelected(bool selected);

    /**
        * @brief Checks if the button is currently selected.
        * @return true if selected.
        */
    bool getSelected() const;
    
    /**
     * @brief Checks if the element is focusable.
     * @return true (Buttons are always focusable).
     */
    bool isFocusable() const override { return true; }
    
    /**
        * @brief Handles input events.
        * Checks for touch events within bounds or confirmation buttons if selected.
        * @param input The input manager instance.
        */
    void handleInput(const pixelroot32::input::InputManager& input);

    void update(unsigned long deltaTime) override;
    void draw(pixelroot32::graphics::Renderer& renderer) override;

    /**
        * @brief Manually triggers the button's action.
        */
    void press();
};
}
