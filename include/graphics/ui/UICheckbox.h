/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#pragma once
#include "graphics/ui/UIElement.h"
#include "graphics/Color.h"
#include <string>
#include <string_view>
#include <functional>
#include "input/InputManager.h"

namespace pixelroot32::graphics::ui {

/**
 * @class UICheckBox
 * @brief A clickable checkbox UI element.
 *
 * Supports both physical (keyboard/gamepad) and touch input.
 * Can trigger a callback function when its state changes.
 */
class UICheckBox : public UIElement {
public:
    /**
     * @brief Constructs a new UICheckBox.
     * @param label Checkbox label text.
     * @param index Navigation index (for D-pad navigation).
     * @param position Position.
     * @param size Size.
     * @param checked Initial checked state.
     * @param callback Function to call when the state changes.
     * @param fontSize Text size multiplier.
     */
    UICheckBox(std::string_view label, uint8_t index, pixelroot32::math::Vector2 position, pixelroot32::math::Vector2 size, bool checked = false, std::function<void(bool)> callback = nullptr, int fontSize = 2);

    /**
     * @brief Constructs a new UICheckBox.
     * @param label Checkbox label text.
     * @param index Navigation index (for D-pad navigation).
     * @param x X position.
     * @param y Y position.
     * @param w Width.
     * @param h Height.
     * @param checked Initial checked state.
     * @param callback Function to call when the state changes.
     * @param fontSize Text size multiplier.
     */
    UICheckBox(std::string_view label, uint8_t index, pixelroot32::math::Scalar x, pixelroot32::math::Scalar y, int w, int h, bool checked = false, std::function<void(bool)> callback = nullptr, int fontSize = 2);

    /**
     * @brief Configures the checkbox's visual style.
     * @param textCol Color of the text.
     * @param bgCol Color of the background.
     * @param drawBg Whether to draw the background rectangle.
     */
    void setStyle(Color textCol, Color bgCol, bool drawBg = false);

    /**
     * @brief Sets the checked state.
     * @param checked True if checked.
     */
    void setChecked(bool checked);

    /**
     * @brief Checks if the checkbox is currently checked.
     * @return true if checked.
     */
    bool isChecked() const;

    /**
     * @brief Sets the selection state (e.g., focused via D-pad).
     * @param selected True if selected.
     */
    void setSelected(bool selected);

    /**
     * @brief Checks if the checkbox is currently selected.
     * @return true if selected.
     */
    bool getSelected() const;

    /**
     * @brief Checks if the element is focusable.
     * @return true (Checkboxes are always focusable).
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
     * @brief Toggles the checkbox state.
     */
    void toggle();

private:
    std::string label;
    bool checked = false;
    bool drawBg = false;
    Color textColor;
    Color backgroundColor;
    bool isSelected = false;
    int fontSize = 2;
    uint8_t index;
    std::function<void(bool)> onCheckChanged;

    /**
     * @brief Internal helper to check if a point is inside the checkbox's bounds.
     * @param px Point X coordinate.
     * @param py Point Y coordinate.
     * @return true if point is inside.
     */
    bool isPointInside(int px, int py) const;
};

}
