/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#include "graphics/ui/UILabel.h"

namespace pixelroot32::graphics::ui {

    using namespace pixelroot32::graphics;

    UILabel::UILabel(std::string t, float x, float y, Color col, uint8_t sz)
        : UIElement(x, y, 0, 0),
            text(t),
            color(col),
            size(sz) {
        recalcSize();
    }

    void UILabel::setText(const std::string& newText) {
        if (text == newText) return;
        text = newText;
        dirty = true;
    }

    void UILabel::centerX(int screenWidth) {
        this->x = (screenWidth - width) * 0.5f;
    }

    void UILabel::update(unsigned long deltaTime) {
        (void)deltaTime;

        if (dirty) {
            recalcSize();
            dirty = false;
        }
    }

    void UILabel::draw(Renderer& renderer) {
        if (!isVisible) return;
        renderer.drawText(text.c_str(), x, y, color, size);
    }
}