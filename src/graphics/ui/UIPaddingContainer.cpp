/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#include "graphics/ui/UIPaddingContainer.h"
#include "graphics/Renderer.h"

namespace pixelroot32::graphics::ui {

UIPaddingContainer::UIPaddingContainer(float x, float y, float w, float h)
    : UIElement(x, y, w, h) {
}

void UIPaddingContainer::setChild(UIElement* element) {
    child = element;
    if (child) {
        updateChildPosition();
    }
}

void UIPaddingContainer::setPadding(float p) {
    paddingLeft = p;
    paddingRight = p;
    paddingTop = p;
    paddingBottom = p;
    updateChildPosition();
}

void UIPaddingContainer::setPadding(float left, float right, float top, float bottom) {
    paddingLeft = left;
    paddingRight = right;
    paddingTop = top;
    paddingBottom = bottom;
    updateChildPosition();
}

void UIPaddingContainer::setPosition(float newX, float newY) {
    UIElement::setPosition(newX, newY);
    updateChildPosition();
}

void UIPaddingContainer::updateChildPosition() {
    if (!child) return;
    
    // Position child element with padding offset
    float childX = x + paddingLeft;
    float childY = y + paddingTop;
    
    child->setPosition(childX, childY);
    
    // Optionally adjust child size to fit within container (if child is larger than available space)
    // This is optional - we'll let the child keep its original size by default
    // If needed, child size adjustment can be added here
}

void UIPaddingContainer::update(unsigned long deltaTime) {
    if (!isEnabled) return;
    
    // Update child element
    if (child && child->isEnabled) {
        child->update(deltaTime);
    }
}

void UIPaddingContainer::draw(pixelroot32::graphics::Renderer& renderer) {
    if (!isVisible) return;
    
    // Draw child element
    if (child && child->isVisible) {
        child->draw(renderer);
    }
}

}
