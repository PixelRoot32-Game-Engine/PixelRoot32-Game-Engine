/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#include "graphics/ui/UIVerticalLayout.h"
#include "graphics/ui/UIButton.h"
#include "graphics/Renderer.h"
#include "core/Scene.h"
#include <algorithm>

namespace pixelroot32::graphics::ui {

UIVerticalLayout::UIVerticalLayout(float x, float y, float w, float h)
    : UILayout(x, y, w, h) {
    lastScrollOffset = 0.0f;
    needsClear = true; // Clear on first draw
}

void UIVerticalLayout::addElement(UIElement* element) {
    if (!element) return;
    
    // Check if element is already in the layout
    auto it = std::find(elements.begin(), elements.end(), element);
    if (it != elements.end()) return;
    
    elements.push_back(element);
    updateLayout();
}

void UIVerticalLayout::removeElement(UIElement* element) {
    if (!element) return;
    
    auto it = std::find(elements.begin(), elements.end(), element);
    if (it != elements.end()) {
        elements.erase(it);
        // If removed element was selected, adjust selection
        if (selectedIndex >= static_cast<int>(elements.size())) {
            selectedIndex = static_cast<int>(elements.size()) - 1;
        }
        updateLayout();
    }
}

void UIVerticalLayout::calculateContentHeight() {
    contentHeight = padding * 2.0f; // Top and bottom padding
    
    for (size_t i = 0; i < elements.size(); ++i) {
        contentHeight += static_cast<float>(elements[i]->height);
        if (i < elements.size() - 1) {
            contentHeight += spacing; // Spacing between elements (not after last)
        }
    }
}

void UIVerticalLayout::updateLayout() {
    calculateContentHeight();
    
    // Check if scroll changed (for performance optimization: only clear when needed)
    // Use a small threshold to catch all scroll changes, including instant scrolls
    if (std::abs(scrollOffset - lastScrollOffset) > 0.01f) {
        needsClear = true;
        lastScrollOffset = scrollOffset;
    }
    
    float currentY = y + padding - scrollOffset;
    float viewportTop = y;
    float viewportBottom = y + static_cast<float>(height);
    
    for (size_t i = 0; i < elements.size(); ++i) {
        UIElement* elem = elements[i];
        
        // Set X position (centered or left-aligned based on layout width)
        float elemX = x + padding;
        if (elem->width < width - (padding * 2)) {
            // Center element if it's smaller than layout width
            elemX = x + (width - elem->width) / 2.0f;
        }
        
        elem->setPosition(elemX, currentY);
        
        // Update visibility immediately based on current position
        float elemTop = currentY;
        float elemBottom = currentY + static_cast<float>(elem->height);
        bool visible = (elemTop < viewportBottom && elemBottom > viewportTop);
        elem->setVisible(visible);
        
        currentY += static_cast<float>(elem->height) + spacing;
    }
    
    clampScrollOffset();
}

void UIVerticalLayout::setButtonStyle(pixelroot32::graphics::Color selectedTextCol,
                                     pixelroot32::graphics::Color selectedBgCol,
                                     pixelroot32::graphics::Color unselectedTextCol,
                                     pixelroot32::graphics::Color unselectedBgCol) {
    selectedTextColor = selectedTextCol;
    selectedBgColor = selectedBgCol;
    unselectedTextColor = unselectedTextCol;
    unselectedBgColor = unselectedBgCol;
    
    // Update styles of existing buttons
    setSelectedIndex(selectedIndex); // This will update all button styles
}

void UIVerticalLayout::clampScrollOffset() {
    float maxScroll = contentHeight - static_cast<float>(height);
    if (maxScroll < 0.0f) maxScroll = 0.0f;
    
    if (scrollOffset < 0.0f) {
        scrollOffset = 0.0f;
    } else if (scrollOffset > maxScroll) {
        scrollOffset = maxScroll;
    }
    
    targetScrollOffset = scrollOffset;
}

void UIVerticalLayout::updateElementVisibility() {
    float viewportTop = y;
    float viewportBottom = y + static_cast<float>(height);
    
    for (UIElement* elem : elements) {
        float elemTop = elem->y;
        float elemBottom = elem->y + static_cast<float>(elem->height);
        
        // Element is visible if it overlaps with viewport
        // Use strict bounds checking to prevent drawing outside viewport
        bool visible = (elemTop < viewportBottom && elemBottom > viewportTop);
        elem->setVisible(visible);
    }
}

void UIVerticalLayout::ensureSelectedVisible() {
    if (selectedIndex < 0 || selectedIndex >= static_cast<int>(elements.size())) {
        return;
    }
    
    // Calculate absolute position of selected element in content space (from top of content)
    float absoluteY = padding;
    for (int i = 0; i < selectedIndex; ++i) {
        absoluteY += static_cast<float>(elements[i]->height) + spacing;
    }
    
    float elemHeight = static_cast<float>(elements[selectedIndex]->height);
    float elemTop = absoluteY;
    float elemBottom = absoluteY + elemHeight;
    
    float viewportHeight = static_cast<float>(height);
    
    // Calculate screen positions with current scroll
    float screenTop = y + padding + elemTop - scrollOffset;
    float screenBottom = y + padding + elemBottom - scrollOffset;
    float viewportTop = y;
    float viewportBottom = y + viewportHeight;
    
    // Calculate required scroll offset to make element visible
    float newScrollOffset = scrollOffset;
    bool needsScroll = false;
    
    // If element top is above viewport, scroll up (decrease scroll offset)
    if (screenTop < viewportTop) {
        // Scroll so element top aligns with viewport top
        newScrollOffset = elemTop;
        needsScroll = true;
    }
    // If element bottom is below viewport, scroll down (increase scroll offset)
    else if (screenBottom > viewportBottom) {
        // Scroll so element bottom aligns with viewport bottom
        newScrollOffset = elemBottom - (viewportHeight - padding * 2);
        needsScroll = true;
    }
    
    // Apply scroll immediately (NES-style: instant scroll on selection change)
    if (needsScroll) {
        // Mark for clearing before changing scroll (important for instant scroll)
        needsClear = true;
        scrollOffset = newScrollOffset;
        targetScrollOffset = newScrollOffset;
        lastScrollOffset = newScrollOffset; // Update immediately to prevent false detection
        clampScrollOffset();
        updateLayout();
    }
}

void UIVerticalLayout::setScrollOffset(float offset) {
    // Mark for clearing when scroll changes
    if (std::abs(offset - scrollOffset) > 0.01f) {
        needsClear = true;
    }
    scrollOffset = offset;
    targetScrollOffset = offset;
    lastScrollOffset = offset; // Update immediately
    clampScrollOffset();
    updateLayout();
}

void UIVerticalLayout::setSelectedIndex(int index) {
    if (index < -1) index = -1;
    if (index >= static_cast<int>(elements.size())) {
        index = static_cast<int>(elements.size()) - 1;
    }
    
    // Mark for clearing if selection changed (scroll will happen)
    if (selectedIndex != index) {
        needsClear = true;
    }
    
    selectedIndex = index;
    
    // Update button selection states and styles
    for (size_t i = 0; i < elements.size(); ++i) {
        // Use static_cast instead of dynamic_cast for ESP32 compatibility (no RTTI)
        // We assume all elements in a button layout are UIButtons
        UIButton* btn = static_cast<UIButton*>(elements[i]);
        if (btn) {
            bool isSelected = (static_cast<int>(i) == selectedIndex);
            btn->setSelected(isSelected);
            // Update style based on selection
            if (isSelected) {
                btn->setStyle(selectedTextColor, selectedBgColor, true);
            } else {
                btn->setStyle(unselectedTextColor, unselectedBgColor, false);
            }
        }
    }
    
    if (selectedIndex >= 0) {
        ensureSelectedVisible();
    }
}

UIElement* UIVerticalLayout::getSelectedElement() const {
    if (selectedIndex < 0 || selectedIndex >= static_cast<int>(elements.size())) {
        return nullptr;
    }
    return elements[selectedIndex];
}

void UIVerticalLayout::handleInput(const pixelroot32::input::InputManager& input) {
    if (elements.empty()) {
        selectedIndex = -1;
        return;
    }
    
    bool selectionChanged = false;
    
    // Handle UP navigation with rising edge detection (workaround for InputManager debounce bug)
    bool isUp = input.isButtonDown(navUpButton);
    if (isUp && !wasUpPressed) { // UP Rising Edge
        if (selectedIndex > 0) {
            selectedIndex--;
            selectionChanged = true;
        } else if (selectedIndex == -1) {
            selectedIndex = static_cast<int>(elements.size()) - 1;
            selectionChanged = true;
        } else {
            selectedIndex = static_cast<int>(elements.size()) - 1; // Wrap to last
            selectionChanged = true;
        }
    }
    wasUpPressed = isUp;
    
    // Handle DOWN navigation with rising edge detection
    bool isDown = input.isButtonDown(navDownButton);
    if (isDown && !wasDownPressed) { // DOWN Rising Edge
        if (selectedIndex < static_cast<int>(elements.size()) - 1) {
            selectedIndex++;
            selectionChanged = true;
        } else if (selectedIndex == -1) {
            selectedIndex = 0;
            selectionChanged = true;
        } else {
            selectedIndex = 0; // Wrap to first
            selectionChanged = true;
        }
    }
    wasDownPressed = isDown;
    
    if (selectionChanged) {
        setSelectedIndex(selectedIndex);
    }
    
    // Forward input to selected element (for button callbacks)
    if (selectedIndex >= 0 && selectedIndex < static_cast<int>(elements.size())) {
        // Use static_cast instead of dynamic_cast for ESP32 compatibility (no RTTI)
        // We assume all elements in a button layout are UIButtons
        UIButton* btn = static_cast<UIButton*>(elements[selectedIndex]);
        if (btn) {
            btn->handleInput(input);
        }
    }
}

void UIVerticalLayout::update(unsigned long deltaTime) {
    // Smooth scroll interpolation (only for manual scrolling, not for selection-based scrolling)
    // Selection-based scrolling is instant (NES-style)
    if (UILayout::enableScroll && std::abs(targetScrollOffset - scrollOffset) > 0.1f) {
        float delta = targetScrollOffset - scrollOffset;
        float maxDelta = scrollSpeed * static_cast<float>(deltaTime);
        
        if (std::abs(delta) <= maxDelta) {
            scrollOffset = targetScrollOffset;
        } else {
            scrollOffset += (delta > 0.0f ? maxDelta : -maxDelta);
        }
        
        updateLayout();
    }
    
    // Update child elements
    for (UIElement* elem : elements) {
        if (elem->isEnabled) {
            elem->update(deltaTime);
        }
    }
}

void UIVerticalLayout::draw(pixelroot32::graphics::Renderer& renderer) {
    if (!isVisible) return;
    
    // Performance optimization: Only clear layout area when scroll/selection changed
    // This avoids expensive fillRect() call every frame on ESP32
    // However, we must clear when scroll is enabled and there's content to scroll
    bool shouldClear = needsClear;
    if (UILayout::enableScroll && contentHeight > static_cast<float>(height)) {
        // If scroll is enabled and content exceeds viewport, always clear to prevent artifacts
        // This is necessary because elements can move outside viewport
        shouldClear = true;
    }
    
    if (shouldClear) {
        renderer.drawFilledRectangle(static_cast<int>(x), static_cast<int>(y), 
                                     width, height, pixelroot32::graphics::Color::Black);
        needsClear = false;
    }
    
    // Draw only visible elements (visibility already calculated in updateLayout)
    // Skip double-checking for performance - we trust updateLayout() visibility calculation
    for (UIElement* elem : elements) {
        if (elem->isVisible) {
            elem->draw(renderer);
        }
    }
}

}
