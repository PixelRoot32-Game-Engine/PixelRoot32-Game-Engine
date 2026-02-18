/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#include "graphics/ui/UIButton.h"
#include "graphics/FontManager.h"

namespace pixelroot32::graphics::ui {

    using namespace pixelroot32::input;
    using namespace pixelroot32::graphics;

    using namespace pixelroot32::math;

    UIButton::UIButton(std::string_view t, uint8_t index, Vector2 position, Vector2 size, std::function<void()> callback, TextAlignment textAlign, int fontSize)
        : UIElement(position, static_cast<int>(size.x), static_cast<int>(size.y), UIElementType::BUTTON), 
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
        return (px >= static_cast<int>(position.x) && px <= static_cast<int>(position.x + width) && 
                py >= static_cast<int>(position.y) && py <= static_cast<int>(position.y + height));
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
        int intY = static_cast<int>(position.y);
        int textPixelHeight = fontSize * 8;
        if (textPixelHeight > intHeight) textPixelHeight = intHeight;
        int textY = intY + (intHeight - textPixelHeight) / 2;

        // 1. Draw background only when hasBackground is enabled
        if (hasBackground) {
            // Filled background rectangle
            renderer.drawFilledRectangle(static_cast<int>(position.x), static_cast<int>(position.y), static_cast<int>(width), static_cast<int>(height), backgroundColor);
        } else {
            // Without background, indicate selection with a small marker
            if (isSelected) {
                renderer.drawText(">", static_cast<int>(position.x) - 10, textY, Color::Yellow, fontSize);
            }
        }

        // 2. Draw label text
        Color currentTextCol = (isSelected && !hasBackground) ? Color::Yellow : textColor;

        if (textAlign == TextAlignment::CENTER) {
            // Calculate text width using FontManager
            int textWidth = FontManager::textWidth(nullptr, label.c_str(), fontSize);
            int textX = static_cast<int>(position.x) + (static_cast<int>(width) - textWidth) / 2;
            renderer.drawText(label.c_str(), textX, textY, currentTextCol, fontSize);
        } else if (textAlign == TextAlignment::RIGHT) {
            renderer.drawText(label.c_str(), static_cast<int>(position.x + width) - 5, textY, currentTextCol, fontSize);
        } else {
            renderer.drawText(label.c_str(), static_cast<int>(position.x) + 5, textY, currentTextCol, fontSize);
        }

        // Restore bypass state
        if (fixedPosition) {
            renderer.setOffsetBypass(oldBypass);
        }
    }
}
