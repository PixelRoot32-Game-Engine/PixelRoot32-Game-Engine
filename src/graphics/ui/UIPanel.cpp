/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#include "graphics/ui/UIPanel.h"
#include "graphics/Renderer.h"

namespace pixelroot32::graphics::ui {

UIPanel::UIPanel(pixelroot32::math::Scalar x, pixelroot32::math::Scalar y, int w, int h)
    : UIElement(x, y, w, h) {
}

UIPanel::UIPanel(pixelroot32::math::Vector2 position, int w, int h)
    : UIElement(position, w, h) {
}

void UIPanel::setChild(UIElement* element) {
    child = element;
    if (child) {
        updateChildPosition();
    }
}

void UIPanel::setPosition(pixelroot32::math::Scalar newX, pixelroot32::math::Scalar newY) {
    UIElement::setPosition(newX, newY);
    updateChildPosition();
}

void UIPanel::updateChildPosition() {
    if (!child) return;
    
    // Position child element at panel position (child can be smaller or larger)
    // The child's position is relative to the panel
    child->setPosition(position.x, position.y);
}

void UIPanel::update(unsigned long deltaTime) {
    if (!isEnabled) return;
    
    // Update child element
    if (child && child->isEnabled) {
        child->update(deltaTime);
    }
}

void UIPanel::draw(pixelroot32::graphics::Renderer& renderer) {
    if (!isVisible) return;
    
    // Save current bypass state and apply fixedPosition if enabled
    bool oldBypass = renderer.isOffsetBypassEnabled();
    if (fixedPosition) {
        renderer.setOffsetBypass(true);
    }
    
    // Draw background (filled rectangle)
    if (backgroundColor != pixelroot32::graphics::Color::Transparent) {
        renderer.drawFilledRectangle(
            static_cast<int>(position.x),
            static_cast<int>(position.y),
            width,
            height,
            backgroundColor
        );
    }
    
    // Draw border (rectangle outline)
    if (borderWidth > 0 && borderColor != pixelroot32::graphics::Color::Transparent) {
        // Draw border by drawing rectangles for each side
        // Top
        if (borderWidth > 0) {
            renderer.drawFilledRectangle(
                static_cast<int>(position.x),
                static_cast<int>(position.y),
                width,
                borderWidth,
                borderColor
            );
        }
        // Bottom
        if (borderWidth > 0) {
            renderer.drawFilledRectangle(
                static_cast<int>(position.x),
                static_cast<int>(position.y + height - borderWidth),
                width,
                borderWidth,
                borderColor
            );
        }
        // Left
        if (borderWidth > 0) {
            renderer.drawFilledRectangle(
                static_cast<int>(position.x),
                static_cast<int>(position.y + borderWidth),
                borderWidth,
                height - (borderWidth * 2),
                borderColor
            );
        }
        // Right
        if (borderWidth > 0) {
            renderer.drawFilledRectangle(
                static_cast<int>(position.x + width - borderWidth),
                static_cast<int>(position.y + borderWidth),
                borderWidth,
                height - (borderWidth * 2),
                borderColor
            );
        }
    }
    
    // Draw child element
    if (child && child->isVisible) {
        child->draw(renderer);
    }
    
    // Restore bypass state
    if (fixedPosition) {
        renderer.setOffsetBypass(oldBypass);
    }
}

}
