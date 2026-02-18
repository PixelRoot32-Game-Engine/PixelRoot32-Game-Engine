/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#include "graphics/ui/UICheckbox.h"

namespace pixelroot32::graphics::ui {

    using namespace pixelroot32::input;
    using namespace pixelroot32::graphics;
    using namespace pixelroot32::math;

    UICheckBox::UICheckBox(std::string_view label, uint8_t index, Vector2 position, Vector2 size, bool checked, std::function<void(bool)> callback, int fontSize)
        : UIElement(position, static_cast<int>(size.x), static_cast<int>(size.y), UIElementType::CHECKBOX),
          label(label),
          checked(checked),
          fontSize(fontSize),
          index(index),
          onCheckChanged(callback) {
        
        textColor = Color::White;
        backgroundColor = Color::Black;
    }

    UICheckBox::UICheckBox(std::string_view label, uint8_t index, Scalar x, Scalar y, int w, int h, bool checked, std::function<void(bool)> callback, int fontSize)
        : UIElement(x, y, w, h, UIElementType::CHECKBOX),
          label(label),
          checked(checked),
          fontSize(fontSize),
          index(index),
          onCheckChanged(callback) {
        
        textColor = Color::White;
        backgroundColor = Color::Black;
    }

    void UICheckBox::setStyle(Color textCol, Color bgCol, bool drawBg) {
        textColor = textCol;
        backgroundColor = bgCol;
        this->drawBg = drawBg;
    }

    void UICheckBox::setChecked(bool checked) {
        if (this->checked != checked) {
            this->checked = checked;
            if (onCheckChanged) {
                onCheckChanged(this->checked);
            }
        }
    }

    bool UICheckBox::isChecked() const {
        return checked;
    }

    void UICheckBox::setSelected(bool selected) {
        isSelected = selected;
    }

    bool UICheckBox::getSelected() const {
        return isSelected;
    }

    void UICheckBox::toggle() {
        if (isEnabled) {
            setChecked(!checked);
        }
    }

    bool UICheckBox::isPointInside(int px, int py) const {
        return (px >= static_cast<int>(position.x) && px <= static_cast<int>(position.x + width) && 
                py >= static_cast<int>(position.y) && py <= static_cast<int>(position.y + height));
    }

    void UICheckBox::handleInput(const InputManager& input) {
        if (!isEnabled || !isVisible) return;

        // 1. Physical button activation (e.g., A / Enter) when the checkbox is focused
        if (isSelected && input.isButtonPressed(index)) {
            this->toggle();
        }

        // 2. Touch / mouse activation (if implemented in the input system)
        // if (input.isButtonClicked()) { 
        //     if (isPointInside(input.getMouseX(), input.getMouseY())) {
        //         this->toggle();
        //     }
        // }
    }

    void UICheckBox::update(unsigned long deltaTime) {
        (void)deltaTime;
    }

    void UICheckBox::draw(Renderer& renderer) {
        if (!isVisible) return;

        // Save current bypass state and apply fixedPosition if enabled
        bool oldBypass = renderer.isOffsetBypassEnabled();
        if (fixedPosition) {
            renderer.setOffsetBypass(true);
        }

        int intX = static_cast<int>(position.x);
        int intY = static_cast<int>(position.y);
        int intHeight = static_cast<int>(height);
        int intWidth = static_cast<int>(width);
        
        // 1. Draw background only when drawBg is enabled
        if (drawBg) {
            renderer.drawFilledRectangle(intX, intY, intWidth, intHeight, backgroundColor);
        } else {
            // Without background, indicate selection with a small marker
            if (isSelected) {
                renderer.drawText(">", intX - 10, intY + (intHeight - (fontSize * 8)) / 2, Color::Yellow, fontSize);
            }
        }

        // 2. Draw checkbox box
        int boxSize = fontSize * 8;
        int boxY = intY + (intHeight - boxSize) / 2;
        int boxX = intX + (drawBg ? 5 : 0); // Offset if background is present
        
        // Use highlighted color if selected and no background
        Color highlightColor = (isSelected && !drawBg) ? Color::Yellow : textColor;
        
        // Outer box
        renderer.drawRectangle(boxX, boxY, boxSize, boxSize, highlightColor);
        
        // Inner check mark if checked
        if (checked) {
            renderer.drawFilledRectangle(boxX + 2, boxY + 2, boxSize - 4, boxSize - 4, highlightColor);
        }

        // 3. Draw label text
        int textX = boxX + boxSize + 5;
        int textY = intY + (intHeight - (fontSize * 8)) / 2;
        renderer.drawText(label.c_str(), textX, textY, highlightColor, fontSize);

        // Restore bypass state
        if (fixedPosition) {
            renderer.setOffsetBypass(oldBypass);
        }
    }

}
