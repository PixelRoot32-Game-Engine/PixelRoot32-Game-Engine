/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#include "graphics/ui/UIGridLayout.h"
#include "graphics/ui/UIButton.h"
#include "graphics/ui/UICheckbox.h"
#include "graphics/Renderer.h"
#include "core/Scene.h"
#include <algorithm>
#include <cmath>

namespace pixelroot32::graphics::ui {

    using namespace pixelroot32::math;

    UIGridLayout::UIGridLayout(Scalar x, Scalar y, Scalar w, Scalar h)
        : UILayout(x, y, w, h) {
        if (columns == 0) columns = 1; // Ensure at least 1 column
    }

    void UIGridLayout::addElement(UIElement* element) {
        if (!element) return;
        
        // Check if element is already in the layout
        auto it = std::find(elements.begin(), elements.end(), element);
        if (it != elements.end()) return;
        
        elements.push_back(element);
        updateLayout();
    }

    void UIGridLayout::removeElement(UIElement* element) {
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

    void UIGridLayout::setColumns(uint8_t cols) {
        if (cols == 0) cols = 1; // Ensure at least 1 column
        columns = cols;
        updateLayout();
    }

    void UIGridLayout::calculateRows() {
        if (columns == 0) {
            rows = 0;
            return;
        }
        // Calculate rows: ceil(elements.size() / columns)
        // Use integer arithmetic to avoid float/Scalar conversion for simple ceiling
        rows = static_cast<uint8_t>((elements.size() + columns - 1) / columns);
    }

    void UIGridLayout::calculateCellDimensions() {
        if (columns == 0 || rows == 0) {
            cellWidth = Scalar(0);
            cellHeight = Scalar(0);
            return;
        }
        
        // Calculate cell width: (width - padding * 2 - spacing * (columns - 1)) / columns
        Scalar totalSpacingX = spacing * Scalar(columns - 1);
        cellWidth = (Scalar(width) - padding * Scalar(2) - totalSpacingX) / Scalar(columns);
        
        // Calculate cell height: (height - padding * 2 - spacing * (rows - 1)) / rows
        Scalar totalSpacingY = spacing * Scalar(rows - 1);
        cellHeight = (Scalar(height) - padding * Scalar(2) - totalSpacingY) / Scalar(rows);
        
        // Ensure non-negative dimensions
        if (cellWidth < Scalar(0)) cellWidth = Scalar(0);
        if (cellHeight < Scalar(0)) cellHeight = Scalar(0);
    }

    void UIGridLayout::updateLayout() {
        calculateRows();
        calculateCellDimensions();
        
        // Position each element based on its index
        for (size_t i = 0; i < elements.size(); ++i) {
            UIElement* elem = elements[i];
            
            // Calculate row and column from index
            int row = static_cast<int>(i) / static_cast<int>(columns);
            int col = static_cast<int>(i) % static_cast<int>(columns);
            
            // Calculate position
            Scalar elemX = x + padding + Scalar(col) * (cellWidth + spacing);
            Scalar elemY = y + padding + Scalar(row) * (cellHeight + spacing);
            
            // Center element within cell if it's smaller than the cell
            Scalar elemXCentered = elemX;
            Scalar elemYCentered = elemY;
            
            if (Scalar(elem->width) < cellWidth) {
                elemXCentered = elemX + (cellWidth - Scalar(elem->width)) / Scalar(2);
            }
            if (Scalar(elem->height) < cellHeight) {
                elemYCentered = elemY + (cellHeight - Scalar(elem->height)) / Scalar(2);
            }
            
            elem->setPosition(elemXCentered, elemYCentered);
            
            // Update visibility (viewport culling)
            Scalar viewportLeft = x;
            Scalar viewportRight = x + Scalar(width);
            Scalar viewportTop = y;
            Scalar viewportBottom = y + Scalar(height);
            
            Scalar elemLeft = elemXCentered;
            Scalar elemRight = elemXCentered + Scalar(elem->width);
            Scalar elemTop = elemYCentered;
            Scalar elemBottom = elemYCentered + Scalar(elem->height);
            
            bool visible = (elemLeft < viewportRight && elemRight > viewportLeft &&
                        elemTop < viewportBottom && elemBottom > viewportTop);
            elem->setVisible(visible);
        }
    }

    void UIGridLayout::setButtonStyle(pixelroot32::graphics::Color selectedTextCol,
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

    void UIGridLayout::setSelectedIndex(int index) {
        if (index < -1) index = -1;
        if (index >= static_cast<int>(elements.size())) {
            index = static_cast<int>(elements.size()) - 1;
        }
        
        selectedIndex = index;
        
        // Update element selection states and styles
        for (size_t i = 0; i < elements.size(); ++i) {
            UIElement* elem = elements[i];
            bool isSelected = (static_cast<int>(i) == selectedIndex);
            
            if (elem->getType() == UIElement::UIElementType::BUTTON) {
                UIButton* btn = static_cast<UIButton*>(elem);
                if (btn) {
                    btn->setSelected(isSelected);
                    if (isSelected) {
                        btn->setStyle(selectedTextColor, selectedBgColor, true);
                    } else {
                        btn->setStyle(unselectedTextColor, unselectedBgColor, false);
                    }
                }
            } else if (elem->getType() == UIElement::UIElementType::CHECKBOX) {
                UICheckBox* cb = static_cast<UICheckBox*>(elem);
                if (cb) {
                    cb->setSelected(isSelected);
                    if (isSelected) {
                        cb->setStyle(selectedTextColor, selectedBgColor, true);
                    } else {
                        cb->setStyle(unselectedTextColor, unselectedBgColor, false);
                    }
                }
            }
        }
    }

    UIElement* UIGridLayout::getSelectedElement() const {
        if (selectedIndex < 0 || selectedIndex >= static_cast<int>(elements.size())) {
            return nullptr;
        }
        return elements[selectedIndex];
    }

    void UIGridLayout::handleInput(const pixelroot32::input::InputManager& input) {
        if (elements.empty()) {
            selectedIndex = -1;
            return;
        }
        
        bool selectionChanged = false;
        
        // Handle UP navigation with rising edge detection
        bool isUp = input.isButtonDown(navUpButton);
        if (isUp && !wasUpPressed) { // UP Rising Edge
            if (selectedIndex >= static_cast<int>(columns)) {
                selectedIndex -= columns;
                selectionChanged = true;
            } else if (selectedIndex == -1) {
                // If nothing selected, select last element in last row
                int lastRow = (static_cast<int>(elements.size()) - 1) / static_cast<int>(columns);
                selectedIndex = lastRow * static_cast<int>(columns) + 
                            ((static_cast<int>(elements.size()) - 1) % static_cast<int>(columns));
                selectionChanged = true;
            } else if (selectedIndex >= 0 && selectedIndex < static_cast<int>(columns)) {
                // Wrap to last row, same column
                int col = selectedIndex % static_cast<int>(columns);
                int lastRow = (static_cast<int>(elements.size()) - 1) / static_cast<int>(columns);
                int lastRowStart = lastRow * static_cast<int>(columns);
                int lastRowEnd = std::min(lastRowStart + static_cast<int>(columns) - 1, 
                                        static_cast<int>(elements.size()) - 1);
                selectedIndex = std::min(lastRowStart + col, lastRowEnd);
                selectionChanged = true;
            }
        }
        wasUpPressed = isUp;
        
        // Handle DOWN navigation with rising edge detection
        bool isDown = input.isButtonDown(navDownButton);
        if (isDown && !wasDownPressed) { // DOWN Rising Edge
            int currentRow = selectedIndex >= 0 ? selectedIndex / static_cast<int>(columns) : -1;
            int maxRow = (static_cast<int>(elements.size()) - 1) / static_cast<int>(columns);
            
            if (selectedIndex >= 0 && currentRow < maxRow) {
                int newIndex = selectedIndex + columns;
                if (newIndex < static_cast<int>(elements.size())) {
                    selectedIndex = newIndex;
                    selectionChanged = true;
                }
            } else if (selectedIndex == -1) {
                selectedIndex = 0;
                selectionChanged = true;
            } else {
                // Wrap to first row, same column
                int col = selectedIndex % static_cast<int>(columns);
                selectedIndex = col;
                selectionChanged = true;
            }
        }
        wasDownPressed = isDown;
        
        // Handle LEFT navigation with rising edge detection
        bool isLeft = input.isButtonDown(navLeftButton);
        if (isLeft && !wasLeftPressed) { // LEFT Rising Edge
            if (selectedIndex > 0 && selectedIndex % static_cast<int>(columns) != 0) {
                selectedIndex--;
                selectionChanged = true;
            } else if (selectedIndex == -1) {
                selectedIndex = static_cast<int>(elements.size()) - 1;
                selectionChanged = true;
            } else if (selectedIndex >= 0) {
                // Wrap to last column, same row
                int row = selectedIndex / static_cast<int>(columns);
                int rowStart = row * static_cast<int>(columns);
                int rowEnd = std::min(rowStart + static_cast<int>(columns) - 1, 
                                    static_cast<int>(elements.size()) - 1);
                selectedIndex = rowEnd;
                selectionChanged = true;
            }
        }
        wasLeftPressed = isLeft;
        
        // Handle RIGHT navigation with rising edge detection
        bool isRight = input.isButtonDown(navRightButton);
        if (isRight && !wasRightPressed) { // RIGHT Rising Edge
            if (selectedIndex >= 0 && 
                selectedIndex < static_cast<int>(elements.size()) - 1 &&
                selectedIndex % static_cast<int>(columns) != static_cast<int>(columns) - 1) {
                selectedIndex++;
                selectionChanged = true;
            } else if (selectedIndex == -1) {
                selectedIndex = 0;
                selectionChanged = true;
            } else if (selectedIndex >= 0) {
                // Wrap to first column, same row
                int row = selectedIndex / static_cast<int>(columns);
                selectedIndex = row * static_cast<int>(columns);
                selectionChanged = true;
            }
        }
        wasRightPressed = isRight;
        
        if (selectionChanged) {
            setSelectedIndex(selectedIndex);
        }
        
        // Forward input to selected element (for button callbacks and checkbox toggles)
        if (selectedIndex >= 0 && selectedIndex < static_cast<int>(elements.size())) {
            UIElement* selected = elements[selectedIndex];
            if (selected->getType() == UIElement::UIElementType::BUTTON) {
                UIButton* btn = static_cast<UIButton*>(selected);
                if (btn) {
                    btn->handleInput(input);
                }
            } else if (selected->getType() == UIElement::UIElementType::CHECKBOX) {
                UICheckBox* cb = static_cast<UICheckBox*>(selected);
                if (cb) {
                    cb->handleInput(input);
                }
            }
        }
    }

    void UIGridLayout::update(unsigned long deltaTime) {
        // Update child elements
        for (UIElement* elem : elements) {
            if (elem->isEnabled) {
                elem->update(deltaTime);
            }
        }
    }

    void UIGridLayout::draw(pixelroot32::graphics::Renderer& renderer) {
        if (!isVisible) return;
        
        // Save current bypass state and apply fixedPosition if enabled
        bool oldBypass = renderer.isOffsetBypassEnabled();
        if (fixedPosition) {
            renderer.setOffsetBypass(true);
        }
        
        // Draw only visible elements (visibility already calculated in updateLayout)
        for (UIElement* elem : elements) {
            if (elem->isVisible) {
                elem->draw(renderer);
            }
        }
        
        // Restore bypass state
        if (fixedPosition) {
            renderer.setOffsetBypass(oldBypass);
        }
    }

}
