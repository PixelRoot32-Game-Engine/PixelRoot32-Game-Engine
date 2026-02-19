/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#include "graphics/ui/UIVerticalLayout.h"
#include "graphics/ui/UIButton.h"
#include "graphics/ui/UICheckbox.h"
#include "graphics/Renderer.h"
#include "core/Scene.h"
#include "math/MathUtil.h"
#include <algorithm>

namespace pixelroot32::graphics::ui {

using namespace pixelroot32::math;

    UIVerticalLayout::UIVerticalLayout(Scalar x, Scalar y, int w, int h)
        : UILayout(x, y, w, h) {
        lastScrollOffset = toScalar(0);
        needsClear = true; // Clear on first draw
    }

    UIVerticalLayout::UIVerticalLayout(Vector2 position, int w, int h)
        : UILayout(position, w, h) {
        lastScrollOffset = toScalar(0);
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
        contentHeight = padding * toScalar(2); // Top and bottom padding
        
        for (size_t i = 0; i < elements.size(); ++i) {
            contentHeight += elements[i]->height;
            if (i < elements.size() - 1) {
                contentHeight += spacing; // Spacing between elements (not after last)
            }
        }
    }

    void UIVerticalLayout::updateLayout() {
        calculateContentHeight();
        
        // Check if scroll changed (for performance optimization: only clear when needed)
        // Use a small threshold to catch all scroll changes, including instant scrolls
        if (math::abs(scrollOffset - lastScrollOffset) > toScalar(0.01f)) {
            needsClear = true;
            lastScrollOffset = scrollOffset;
        }
        
        Scalar currentY = position.y + padding - scrollOffset;
        Scalar viewportTop = position.y;
        Scalar viewportBottom = position.y + height;
        
        for (size_t i = 0; i < elements.size(); ++i) {
            UIElement* elem = elements[i];
            
            // Set X position (centered or left-aligned based on layout width)
            Scalar elemX = position.x + padding;
            if (toScalar(elem->width) < toScalar(width) - (padding * toScalar(2))) {
                // Center element if it's smaller than layout width
                elemX = position.x + (toScalar(width) - toScalar(elem->width)) * toScalar(0.5f);
            }
            
            elem->setPosition(elemX, currentY);
            
            // Update visibility immediately based on current position
            Scalar elemTop = currentY;
            Scalar elemBottom = currentY + toScalar(elem->height);
            bool visible = (elemTop < viewportBottom && elemBottom > viewportTop);
            elem->setVisible(visible);
            
            currentY += toScalar(elem->height) + spacing;
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
        Scalar maxScroll = contentHeight - toScalar(height);
        if (maxScroll < toScalar(0.0f)) maxScroll = toScalar(0);
        
        if (scrollOffset < toScalar(0.0f)) {
            scrollOffset = toScalar(0.0f);
        } else if (scrollOffset > maxScroll) {
            scrollOffset = maxScroll;
        }
        
        targetScrollOffset = scrollOffset;
    }

    void UIVerticalLayout::updateElementVisibility() {
        Scalar viewportTop = position.y;
        Scalar viewportBottom = position.y + toScalar(height);
        
        for (UIElement* elem : elements) {
            Scalar elemTop = elem->position.y;
            Scalar elemBottom = elem->position.y + toScalar(elem->height);
            
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
        Scalar absoluteY = padding;
        for (int i = 0; i < selectedIndex; ++i) {
            absoluteY += toScalar(elements[i]->height) + spacing;
        }
        
        Scalar elemHeight = toScalar(elements[selectedIndex]->height);
        Scalar elemTop = absoluteY;
        Scalar elemBottom = absoluteY + elemHeight;
        
        Scalar viewportHeight = toScalar(height);
        
        // Calculate screen positions with current scroll
        Scalar screenTop = position.y + padding + elemTop - scrollOffset;
        Scalar screenBottom = position.y + padding + elemBottom - scrollOffset;
        Scalar viewportTop = position.y;
        Scalar viewportBottom = position.y + viewportHeight;

        
        // Calculate required scroll offset to make element visible
        Scalar newScrollOffset = scrollOffset;
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
            newScrollOffset = elemBottom - (viewportHeight - padding * toScalar(2));
            needsScroll = true;
        }
        
        // Apply scroll immediately (NES-style: instant scroll on selection change)
        if (needsScroll && UILayout::enableScroll) {
            // Mark for clearing before changing scroll (important for instant scroll)
            needsClear = true;
            scrollOffset = newScrollOffset;
            targetScrollOffset = newScrollOffset;
            lastScrollOffset = newScrollOffset; // Update immediately to prevent false detection
            clampScrollOffset();
            updateLayout();
        }
    }

    void UIVerticalLayout::setScrollOffset(Scalar offset) {
        // Mark for clearing when scroll changes
        if (math::abs(offset - scrollOffset) > toScalar(0.01f)) {
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
        if (index >= static_cast<int>(elements.size())) index = static_cast<int>(elements.size()) - 1;
        
        int prevIndex = selectedIndex;
        selectedIndex = index;
        
        // Update button styles
        for (size_t i = 0; i < elements.size(); ++i) {
            bool isSelected = (static_cast<int>(i) == selectedIndex);

            if (elements[i]->getType() == UIElement::UIElementType::BUTTON) {
                UIButton* btn = static_cast<UIButton*>(elements[i]);
                btn->setSelected(isSelected);
                if (isSelected) {
                     btn->setStyle(selectedTextColor, selectedBgColor, true);
                } else {
                    btn->setStyle(unselectedTextColor, unselectedBgColor, false);
                }
            } else if (elements[i]->getType() == UIElement::UIElementType::CHECKBOX) {
                UICheckBox* cb = static_cast<UICheckBox*>(elements[i]);
                cb->setSelected(isSelected);
                if (isSelected) {
                    cb->setStyle(selectedTextColor, selectedBgColor, true);
                } else {
                    cb->setStyle(unselectedTextColor, unselectedBgColor, false);
                }
            }
        }
        
        // If selection changed, ensure visible
        if (prevIndex != selectedIndex) {
            ensureSelectedVisible();
        }
    }

    UIElement* UIVerticalLayout::getSelectedElement() const {
        if (selectedIndex >= 0 && selectedIndex < static_cast<int>(elements.size())) {
            return elements[selectedIndex];
        }
        return nullptr;
    }

    void UIVerticalLayout::handleInput(const pixelroot32::input::InputManager& input) {
        if (elements.empty()) return;
        
        // Navigation
        if (input.isButtonPressed(navUpButton)) {
            if (selectedIndex > 0) {
                setSelectedIndex(selectedIndex - 1);
            } else if (selectedIndex == -1 && !elements.empty()) {
                setSelectedIndex(static_cast<int>(elements.size()) - 1); // Wrap to bottom
            }
        } else if (input.isButtonPressed(navDownButton)) {
            if (selectedIndex < static_cast<int>(elements.size()) - 1) {
                setSelectedIndex(selectedIndex + 1);
            } else if (selectedIndex == -1 && !elements.empty()) {
                setSelectedIndex(0);
            }
        }
        
        // Pass input to selected element
        UIElement* selected = getSelectedElement();
        if (selected) {
            // Handle specific element input logic here if needed
            // For buttons/checkboxes, they usually handle their own input via their update/handleInput methods
            // But since we are managing selection, we might need to trigger actions here
            
            // For now, let's assume the scene or main loop calls handleInput on the selected element too?
            // Or we should forward it?
            // Since UIElement doesn't have handleInput in base, we cast?
            
            if (selected->getType() == UIElement::UIElementType::BUTTON) {
                static_cast<UIButton*>(selected)->handleInput(input);
            } else if (selected->getType() == UIElement::UIElementType::CHECKBOX) {
                static_cast<UICheckBox*>(selected)->handleInput(input);
            }
        }
    }

    void UIVerticalLayout::update(unsigned long deltaTime) {
        // Smooth scrolling
        if (this->UILayout::enableScroll && math::abs(targetScrollOffset - scrollOffset) > toScalar(0.5f)) {
            Scalar diff = targetScrollOffset - scrollOffset;
            Scalar move = diff * toScalar(0.2f); // Simple ease-out
            
            // Clamp min movement
            if (math::abs(move) < toScalar(0.5f)) {
                move = (diff > toScalar(0)) ? toScalar(0.5f) : toScalar(-0.5f);
            }
            
            scrollOffset += move;
            if (math::abs(targetScrollOffset - scrollOffset) < toScalar(1.0f)) {
                scrollOffset = targetScrollOffset;
            }
            
            clampScrollOffset();
            updateLayout();
        }
        
        // Update children
        for (auto* elem : elements) {
            elem->update(deltaTime);
        }
    }

    void UIVerticalLayout::draw(pixelroot32::graphics::Renderer& renderer) {
        // Optional: Draw background or border
        
        // Draw visible elements
        // Since we handle visibility in updateLayout/updateElementVisibility, 
        // we can just draw visible elements.
        // However, for clipping, we might need a scissor test or similar if the renderer supports it.
        // The current renderer is simple, so we rely on visibility toggling.
        
        // If we need to clear the area (e.g. for scrolling artifacts on some displays), do it here
        if (needsClear) {
            // This is a hacky way to "clear" the previous positions if we don't redraw the whole screen
            // ideally the Scene clears the background.
            // If the background is static, we might leave trails.
            // Assuming Scene clears screen every frame, we don't need this.
            needsClear = false; 
        }
        
        for (auto* elem : elements) {
            if (elem->isVisible) {
                elem->draw(renderer);
            }
        }
    }

}
