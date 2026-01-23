/*
 * Copyright (c) 2026 Gabriel Perez
 * Licensed under the GNU GPL v3
 */
#include "graphics/ui/UIPanel.h"
#include "graphics/Renderer.h"

namespace pixelroot32::graphics::ui {

UIPanel::UIPanel(float x, float y, float w, float h)
    : UIElement(x, y, w, h) {
}

void UIPanel::setChild(UIElement* element) {
    child = element;
    if (child) {
        updateChildPosition();
    }
}

void UIPanel::setPosition(float newX, float newY) {
    UIElement::setPosition(newX, newY);
    updateChildPosition();
}

void UIPanel::updateChildPosition() {
    if (!child) return;
    
    // Position child element at panel position (child can be smaller or larger)
    // The child's position is relative to the panel
    child->setPosition(x, y);
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
    
    // Draw background (filled rectangle)
    if (backgroundColor != pixelroot32::graphics::Color::Transparent) {
        renderer.drawFilledRectangle(
            static_cast<int>(x),
            static_cast<int>(y),
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
                static_cast<int>(x),
                static_cast<int>(y),
                width,
                borderWidth,
                borderColor
            );
        }
        // Bottom
        if (borderWidth > 0) {
            renderer.drawFilledRectangle(
                static_cast<int>(x),
                static_cast<int>(y + height - borderWidth),
                width,
                borderWidth,
                borderColor
            );
        }
        // Left
        if (borderWidth > 0) {
            renderer.drawFilledRectangle(
                static_cast<int>(x),
                static_cast<int>(y + borderWidth),
                borderWidth,
                height - (borderWidth * 2),
                borderColor
            );
        }
        // Right
        if (borderWidth > 0) {
            renderer.drawFilledRectangle(
                static_cast<int>(x + width - borderWidth),
                static_cast<int>(y + borderWidth),
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
}

}
