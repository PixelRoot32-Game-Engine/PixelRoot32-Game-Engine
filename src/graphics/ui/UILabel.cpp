/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#include "graphics/ui/UILabel.h"
#include "graphics/FontManager.h"

namespace pixelroot32::graphics::ui {

    using namespace pixelroot32::graphics;

    UILabel::UILabel(std::string t, float x, float y, Color col, uint8_t sz)
        : UIElement(x, y, 0, 0, UIElementType::LABEL),
            text(t),
            color(col),
            size(sz) {
        recalcSize();
    }

    void UILabel::setText(const std::string& newText) {
        if (text == newText) return;
        text = newText;
        recalcSize();
    }

    void UILabel::centerX(int screenWidth) {
        recalcSize();
        this->x = (screenWidth - width) * 0.5f;
    }

    void UILabel::update(unsigned long deltaTime) {
        (void)deltaTime;
    }

    void UILabel::draw(Renderer& renderer) {
        if (!isVisible) return;
        renderer.drawText(text.c_str(), x, y, color, size);
    }

    void UILabel::recalcSize() {
        const Font* font = FontManager::getDefaultFont();
        if (font) {
            this->width = (float)FontManager::textWidth(font, text.c_str(), size);
            this->height = (float)(font->glyphHeight * size);
        } else {
            // Fallback if no font is set (6x8 default)
            this->width = (float)(text.length() * (6 * size));
            this->height = (float)(8 * size);
        }
    }
} // namespace pixelroot32::graphics::ui