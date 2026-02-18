/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#include "graphics/ui/UILabel.h"
#include "graphics/FontManager.h"
#include "math/MathUtil.h"

namespace pixelroot32::graphics::ui {

    using namespace pixelroot32::math;
    using namespace pixelroot32::graphics;

    UILabel::UILabel(std::string_view t, Vector2 position, Color col, uint8_t sz)
        : UIElement(position, 0, 0, UIElementType::LABEL),
            text(t),
            color(col),
            size(sz) {
        recalcSize();
    }

    void UILabel::setText(std::string_view newText) {
        if (text == newText) return;
        text = newText;
        recalcSize();
    }

    void UILabel::centerX(int screenWidth) {
        recalcSize();
        this->position.x = toScalar(screenWidth - static_cast<int>(width)) * toScalar(0.5f);
    }

    void UILabel::update(unsigned long deltaTime) {
        (void)deltaTime;
    }

    void UILabel::draw(Renderer& renderer) {
        if (!isVisible) return;
        
        // Save current bypass state and apply fixedPosition if enabled
        bool oldBypass = renderer.isOffsetBypassEnabled();
        if (fixedPosition) {
            renderer.setOffsetBypass(true);
        }
        
        renderer.drawText(text.c_str(), static_cast<int>(position.x), static_cast<int>(position.y), color, size);
        
        // Restore bypass state
        if (fixedPosition) {
            renderer.setOffsetBypass(oldBypass);
        }
    }

    void UILabel::recalcSize() {
        const Font* font = FontManager::getDefaultFont();
        if (font) {
            this->width = FontManager::textWidth(font, text.c_str(), size);
            this->height = font->glyphHeight * size;
        } else {
            // Fallback if no font is set (6x8 default)
            this->width = static_cast<int>(text.length() * (6 * size));
            this->height = static_cast<int>(8 * size);
        }
    }
} // namespace pixelroot32::graphics::ui
