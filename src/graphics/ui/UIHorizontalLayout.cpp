/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#include "graphics/ui/UIHorizontalLayout.h"
#include "graphics/ui/UIButton.h"
#include "graphics/Renderer.h"
#include "core/Scene.h"
#include <algorithm>

namespace pixelroot32::graphics::ui {

UIHorizontalLayout::UIHorizontalLayout(float x, float y, float w, float h)
    : UILayout(x, y, w, h) {
    lastScrollOffset = 0.0f;
    needsClear = true; // Clear on first draw
}

void UIHorizontalLayout::addElement(UIElement* element) {
    if (!element) return;
    
    // Check if element is already in the layout
    auto it = std::find(elements.begin(), elements.end(), element);
    if (it != elements.end()) return;
    
    elements.push_back(element);
    updateLayout();
}

void UIHorizontalLayout::removeElement(UIElement* element) {
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

void UIHorizontalLayout::calculateContentWidth() {
    contentWidth = padding * 2.0f; // Left and right padding
    
    for (size_t i = 0; i < elements.size(); ++i) {
        contentWidth += static_cast<float>(elements[i]->width);
        if (i < elements.size() - 1) {
            contentWidth += spacing; // Spacing between elements (not after last)
        }
    }
}

void UIHorizontalLayout::updateLayout() {
    calculateContentWidth();
    
    // Check if scroll changed (for performance optimization: only clear when needed)
    // Use a small threshold to catch all scroll changes, including instant scrolls
    if (std::abs(scrollOffset - lastScrollOffset) > 0.01f) {
        needsClear = true;
        lastScrollOffset = scrollOffset;
    }
    
    float currentX = x + padding - scrollOffset;
    float viewportLeft = x;
    float viewportRight = x + static_cast<float>(width);
    
    for (size_t i = 0; i < elements.size(); ++i) {
        UIElement* elem = elements[i];
        
        // Set Y position (centered or top-aligned based on layout height)
        float elemY = y + padding;
        if (elem->height < height - (padding * 2)) {
            // Center element vertically if it's smaller than layout height
            elemY = y + (height - elem->height) / 2.0f;
        }
        
        elem->setPosition(currentX, elemY);
        
        // Update visibility immediately based on current position
        float elemLeft = currentX;
        float elemRight = currentX + static_cast<float>(elem->width);
        bool visible = (elemLeft < viewportRight && elemRight > viewportLeft);
        elem->setVisible(visible);
        
        currentX += static_cast<float>(elem->width) + spacing;
    }
    
    clampScrollOffset();
}

void UIHorizontalLayout::setButtonStyle(pixelroot32::graphics::Color selectedTextCol,
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

void UIHorizontalLayout::clampScrollOffset() {
    float maxScroll = contentWidth - static_cast<float>(width);
    if (maxScroll < 0.0f) maxScroll = 0.0f;
    
    if (scrollOffset < 0.0f) {
        scrollOffset = 0.0f;
    } else if (scrollOffset > maxScroll) {
        scrollOffset = maxScroll;
    }
    
    targetScrollOffset = scrollOffset;
}

void UIHorizontalLayout::updateElementVisibility() {
    float viewportLeft = x;
    float viewportRight = x + static_cast<float>(width);
    
    for (UIElement* elem : elements) {
        float elemLeft = elem->x;
        float elemRight = elem->x + static_cast<float>(elem->width);
        
        // Element is visible if it overlaps with viewport
        // Use strict bounds checking to prevent drawing outside viewport
        bool visible = (elemLeft < viewportRight && elemRight > viewportLeft);
        elem->setVisible(visible);
    }
}

void UIHorizontalLayout::ensureSelectedVisible() {
    if (selectedIndex < 0 || selectedIndex >= static_cast<int>(elements.size())) {
        return;
    }
    
    // Calculate absolute position of selected element in content space (from left of content)
    float absoluteX = padding;
    for (int i = 0; i < selectedIndex; ++i) {
        absoluteX += static_cast<float>(elements[i]->width) + spacing;
    }
    
    float elemWidth = static_cast<float>(elements[selectedIndex]->width);
    float elemLeft = absoluteX;
    float elemRight = absoluteX + elemWidth;
    
    float viewportWidth = static_cast<float>(width);
    
    // Calculate screen positions with current scroll
    float screenLeft = x + padding + elemLeft - scrollOffset;
    float screenRight = x + padding + elemRight - scrollOffset;
    float viewportLeft = x;
    float viewportRight = x + viewportWidth;
    
    // Calculate required scroll offset to make element visible
    float newScrollOffset = scrollOffset;
    bool needsScroll = false;
    
    // If element left is to the left of viewport, scroll left (decrease scroll offset)
    if (screenLeft < viewportLeft) {
        // Scroll so element left aligns with viewport left
        newScrollOffset = elemLeft;
        needsScroll = true;
    }
    // If element right is to the right of viewport, scroll right (increase scroll offset)
    else if (screenRight > viewportRight) {
        // Scroll so element right aligns with viewport right
        newScrollOffset = elemRight - (viewportWidth - padding * 2);
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

void UIHorizontalLayout::setScrollOffset(float offset) {
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

void UIHorizontalLayout::setSelectedIndex(int index) {
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

UIElement* UIHorizontalLayout::getSelectedElement() const {
    if (selectedIndex < 0 || selectedIndex >= static_cast<int>(elements.size())) {
        return nullptr;
    }
    return elements[selectedIndex];
}

void UIHorizontalLayout::handleInput(const pixelroot32::input::InputManager& input) {
    if (elements.empty()) {
        selectedIndex = -1;
        return;
    }
    
    bool selectionChanged = false;
    
    // Handle LEFT navigation with rising edge detection (workaround for InputManager debounce bug)
    bool isLeft = input.isButtonDown(navLeftButton);
    if (isLeft && !wasLeftPressed) { // LEFT Rising Edge
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
    wasLeftPressed = isLeft;
    
    // Handle RIGHT navigation with rising edge detection
    bool isRight = input.isButtonDown(navRightButton);
    if (isRight && !wasRightPressed) { // RIGHT Rising Edge
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
    wasRightPressed = isRight;
    
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

void UIHorizontalLayout::update(unsigned long deltaTime) {
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

void UIHorizontalLayout::draw(pixelroot32::graphics::Renderer& renderer) {
    if (!isVisible) return;
    
    // Performance optimization: Only clear layout area when scroll/selection changed
    // This avoids expensive fillRect() call every frame on ESP32
    // However, we must clear when scroll is enabled and there's content to scroll
    bool shouldClear = needsClear;
    if (UILayout::enableScroll && contentWidth > static_cast<float>(width)) {
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
