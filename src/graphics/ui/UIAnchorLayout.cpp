/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#include "graphics/ui/UIAnchorLayout.h"
#include "graphics/Renderer.h"
#include <algorithm>

namespace pixelroot32::graphics::ui {

UIAnchorLayout::UIAnchorLayout(float x, float y, float w, float h)
    : UILayout(x, y, w, h) {
    screenWidth = w;
    screenHeight = h;
}

void UIAnchorLayout::setScreenSize(float screenW, float screenH) {
    screenWidth = screenW;
    screenHeight = screenH;
    width = static_cast<int>(screenW);
    height = static_cast<int>(screenH);
    updateLayout();
}

void UIAnchorLayout::addElement(UIElement* element, Anchor anchor) {
    if (!element) return;
    
    // Check if element is already in the layout
    auto it = std::find_if(anchoredElements.begin(), anchoredElements.end(),
        [element](const std::pair<UIElement*, Anchor>& pair) {
            return pair.first == element;
        });
    
    if (it != anchoredElements.end()) return;
    
    anchoredElements.push_back(std::make_pair(element, anchor));
    elements.push_back(element); // Also add to base class vector for compatibility
    updateLayout();
}

void UIAnchorLayout::addElement(UIElement* element) {
    // Default to TOP_LEFT if no anchor specified
    addElement(element, Anchor::TOP_LEFT);
}

void UIAnchorLayout::removeElement(UIElement* element) {
    if (!element) return;
    
    // Remove from anchored elements
    auto it = std::find_if(anchoredElements.begin(), anchoredElements.end(),
        [element](const std::pair<UIElement*, Anchor>& pair) {
            return pair.first == element;
        });
    
    if (it != anchoredElements.end()) {
        anchoredElements.erase(it);
    }
    
    // Remove from base class vector
    auto baseIt = std::find(elements.begin(), elements.end(), element);
    if (baseIt != elements.end()) {
        elements.erase(baseIt);
    }
    
    updateLayout();
}

void UIAnchorLayout::calculateAnchorPosition(UIElement* element, Anchor anchor, float& outX, float& outY) const {
    float elemWidth = static_cast<float>(element->width);
    float elemHeight = static_cast<float>(element->height);
    
    switch (anchor) {
        case Anchor::TOP_LEFT:
            outX = 0.0f;
            outY = 0.0f;
            break;
            
        case Anchor::TOP_RIGHT:
            outX = screenWidth - elemWidth;
            outY = 0.0f;
            break;
            
        case Anchor::BOTTOM_LEFT:
            outX = 0.0f;
            outY = screenHeight - elemHeight;
            break;
            
        case Anchor::BOTTOM_RIGHT:
            outX = screenWidth - elemWidth;
            outY = screenHeight - elemHeight;
            break;
            
        case Anchor::CENTER:
            outX = (screenWidth - elemWidth) / 2.0f;
            outY = (screenHeight - elemHeight) / 2.0f;
            break;
            
        case Anchor::TOP_CENTER:
            outX = (screenWidth - elemWidth) / 2.0f;
            outY = 0.0f;
            break;
            
        case Anchor::BOTTOM_CENTER:
            outX = (screenWidth - elemWidth) / 2.0f;
            outY = screenHeight - elemHeight;
            break;
            
        case Anchor::LEFT_CENTER:
            outX = 0.0f;
            outY = (screenHeight - elemHeight) / 2.0f;
            break;
            
        case Anchor::RIGHT_CENTER:
            outX = screenWidth - elemWidth;
            outY = (screenHeight - elemHeight) / 2.0f;
            break;
    }
}

void UIAnchorLayout::updateLayout() {
    // Calculate positions for all anchored elements
    for (auto& pair : anchoredElements) {
        UIElement* elem = pair.first;
        Anchor anchor = pair.second;
        
        float elemX, elemY;
        calculateAnchorPosition(elem, anchor, elemX, elemY);
        
        elem->setPosition(elemX, elemY);
        elem->setVisible(isVisible); // Inherit visibility from layout
    }
}

void UIAnchorLayout::handleInput(const pixelroot32::input::InputManager& input) {
    // Anchor layout doesn't handle input - elements handle their own input
    // This is intentional: HUD elements typically don't need layout-level navigation
    (void)input; // Suppress unused parameter warning
}

void UIAnchorLayout::update(unsigned long deltaTime) {
    // Update child elements
    for (UIElement* elem : elements) {
        if (elem->isEnabled) {
            elem->update(deltaTime);
        }
    }
}

void UIAnchorLayout::draw(pixelroot32::graphics::Renderer& renderer) {
    if (!isVisible) return;
    
    // Auto-update screen size if logical resolution changed in renderer
    if (static_cast<float>(renderer.getLogicalWidth()) != screenWidth || 
        static_cast<float>(renderer.getLogicalHeight()) != screenHeight) {
        setScreenSize(static_cast<float>(renderer.getLogicalWidth()), 
                      static_cast<float>(renderer.getLogicalHeight()));
    }
    
    // Draw all elements (no viewport culling needed for HUD elements)
    for (UIElement* elem : elements) {
        if (elem->isVisible) {
            elem->draw(renderer);
        }
    }
}

}
