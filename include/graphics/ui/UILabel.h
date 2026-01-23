/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#pragma once
#include "UIElement.h"
#include "graphics/Color.h"
#include <string>

namespace pixelroot32::graphics::ui {

    /**
    * @class UILabel
    * @brief A simple text label UI element.
    *
    * Displays a string of text on the screen. Auto-calculates its bounds based on text length and size.
    */
class UILabel : public UIElement {
public:
    /**
        * @brief Constructs a new UILabel.
        * @param t Initial text.
        * @param x X position.
        * @param y Y position.
        * @param col Text color.
        * @param sz Text size multiplier.
        */
    UILabel(std::string t, float x, float y, Color col, uint8_t sz);

    /**
        * @brief Updates the label's text.
        * Recalculates dimensions if text changes.
        * @param t New text.
        */
    void setText(const std::string& t);

    /**
        * @brief Sets visibility.
        * @param v True to show, false to hide.
        */
    void setVisible(bool v) { isVisible = v; }

    /**
        * @brief Centers the label horizontally on the screen.
        * @param screenWidth Width of the screen/container.
        */
    void centerX(int screenWidth);

    void update(unsigned long deltaTime) override;
    void draw(pixelroot32::graphics::Renderer& renderer) override;

private:
    std::string text;
    Color color;
    uint8_t size;
    bool dirty = false;

    /**
        * @brief Recalculates width and height based on current text and font size.
        */
    inline void recalcSize() {
        this->width  = (float)(text.length() * (6 * size));
        this->height = (float)(8 * size);
    }
};
}