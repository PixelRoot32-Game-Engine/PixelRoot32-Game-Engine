/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#include "graphics/ui/UIButton.h"
#include "graphics/FontManager.h"

namespace pixelroot32::graphics::ui {

    using namespace pixelroot32::input;
    using namespace pixelroot32::graphics;

    UIButton::UIButton(std::string t, uint8_t index, float x, float y, float w, float h, std::function<void()> callback, TextAlignment textAlign, int fontSize)
        : UIElement(x, y, w, h, UIElementType::BUTTON), 
            label(t), 
            index(index),
            textAlign(textAlign),
            fontSize(fontSize),
            onClick(callback) {
        
        textColor = Color::White;
        backgroundColor = Color::Black;
        hasBackground = true;
    }

    void UIButton::setStyle(Color textCol, Color bgCol, bool drawBg) {
        textColor = textCol;
        backgroundColor = bgCol;
        hasBackground = drawBg;
    }

    void UIButton::setSelected(bool selected) {
        isSelected = selected;
    }

    bool UIButton::getSelected() const {
        return isSelected;
    }

    void UIButton::press() {
        if (isEnabled && onClick) {
            onClick();
        }
    }

    bool UIButton::isPointInside(int px, int py) const {
        return (px >= x && px <= x + width && 
                py >= y && py <= y + height);
    }

    void UIButton::handleInput(const InputManager& input) {
        if (!isEnabled || !isVisible) return;

        // 1. Physical button activation (e.g., A / Enter) when the button is focused
        if (isSelected && input.isButtonPressed(index)) {
            this->press();
        }

        // 2. Touch / mouse activation (if implemented in the input system)
        // if (input.isButtonClicked()) { 
        //     if (isPointInside(input.getMouseX(), input.getMouseY())) {
        //         this->press();
        //     }
        // }
    }

    void UIButton::update(unsigned long deltaTime) {
        (void)deltaTime;

        // Optional: add visual feedback (e.g., pulsing) when isSelected is true
    }

    void UIButton::draw(Renderer& renderer) {
        if (!isVisible) return;

        // Save current bypass state and apply fixedPosition if enabled
        bool oldBypass = renderer.isOffsetBypassEnabled();
        if (fixedPosition) {
            renderer.setOffsetBypass(true);
        }

        int intHeight = static_cast<int>(height);
        int intY = static_cast<int>(y);
        int textPixelHeight = fontSize * 8;
        if (textPixelHeight > intHeight) textPixelHeight = intHeight;
        int textY = intY + (intHeight - textPixelHeight) / 2;

        // 1. Draw background only when hasBackground is enabled
        if (hasBackground) {
            // Filled background rectangle
            renderer.drawFilledRectangle(x, y, width, height, backgroundColor);
        } else {
            // Without background, indicate selection with a small marker
            if (isSelected) {
                renderer.drawText(">", x - 10, textY, Color::Yellow, fontSize);
            }
        }

        // 2. Draw label text
        Color currentTextCol = (isSelected && !hasBackground) ? Color::Yellow : textColor;

        if (textAlign == TextAlignment::CENTER) {
            // Calculate text width using FontManager
            int textWidth = FontManager::textWidth(nullptr, label.c_str(), fontSize);
            int textX = static_cast<int>(x) + (static_cast<int>(width) - textWidth) / 2;
            renderer.drawText(label.c_str(), textX, textY, currentTextCol, fontSize);
        } else if (textAlign == TextAlignment::RIGHT) {
            renderer.drawText(label.c_str(), x + width - 5, textY, currentTextCol, fontSize);
        } else {
            renderer.drawText(label.c_str(), x + 5, textY, currentTextCol, fontSize);
        }

        // Restore bypass state
        if (fixedPosition) {
            renderer.setOffsetBypass(oldBypass);
        }
    }
}
