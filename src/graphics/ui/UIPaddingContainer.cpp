/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#include "graphics/ui/UIPaddingContainer.h"
#include "graphics/Renderer.h"

namespace pixelroot32::graphics::ui {

using pixelroot32::math::Scalar;
using pixelroot32::math::toScalar;
using Vector2 = pixelroot32::math::Vector2;

UIPaddingContainer::UIPaddingContainer(Scalar x, Scalar y, int w, int h)
    : UIElement(x, y, w, h) {
}

UIPaddingContainer::UIPaddingContainer(Vector2 position, int w, int h)
    : UIElement(position, w, h) {
}

void UIPaddingContainer::setChild(UIElement* element) {
    child = element;
    if (child) {
        updateChildPosition();
    }
}

void UIPaddingContainer::setPadding(Scalar p) {
    paddingLeft = p;
    paddingRight = p;
    paddingTop = p;
    paddingBottom = p;
    updateChildPosition();
}

void UIPaddingContainer::setPadding(Scalar left, Scalar right, Scalar top, Scalar bottom) {
    paddingLeft = left;
    paddingRight = right;
    paddingTop = top;
    paddingBottom = bottom;
    updateChildPosition();
}

void UIPaddingContainer::setPosition(Scalar newX, Scalar newY) {
    UIElement::setPosition(newX, newY);
    updateChildPosition();
}

void UIPaddingContainer::updateChildPosition() {
    if (!child) return;
    
    // Position child element with padding offset
    Scalar childX = position.x + paddingLeft;
    Scalar childY = position.y + paddingTop;
    
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
    
    // Save current bypass state and apply fixedPosition if enabled
    bool oldBypass = renderer.isOffsetBypassEnabled();
    if (fixedPosition) {
        renderer.setOffsetBypass(true);
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

} // namespace pixelroot32::graphics::ui
