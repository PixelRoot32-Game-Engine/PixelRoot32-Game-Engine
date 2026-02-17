/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#pragma once
#include "UILayout.h"
#include "input/InputManager.h"
#include "graphics/Color.h"

namespace pixelroot32::graphics::ui {

/**
 * @class UIGridLayout
 * @brief Grid layout container for organizing elements in a matrix.
 *
 * Organizes UI elements in a fixed grid of rows and columns. Supports
 * navigation in 4 directions (UP/DOWN/LEFT/RIGHT) and automatic
 * positioning based on grid coordinates.
 */
class UIGridLayout : public UILayout {
public:
    /**
     * @brief Constructs a new UIGridLayout.
     * @param x X position of the layout container.
     * @param y Y position of the layout container.
     * @param w Width of the layout container.
     * @param h Height of the layout container.
     */
    UIGridLayout(pixelroot32::math::Scalar x, pixelroot32::math::Scalar y, pixelroot32::math::Scalar w, pixelroot32::math::Scalar h);

    virtual ~UIGridLayout() = default;

    /**
     * @brief Adds a UI element to the layout.
     * @param element Pointer to the element to add.
     */
    void addElement(UIElement* element) override;

    /**
     * @brief Removes a UI element from the layout.
     * @param element Pointer to the element to remove.
     */
    void removeElement(UIElement* element) override;

    /**
     * @brief Recalculates positions of all elements.
     */
    void updateLayout() override;

    /**
     * @brief Handles input for navigation and selection.
     * @param input Reference to the InputManager.
     */
    void handleInput(const pixelroot32::input::InputManager& input) override;

    /**
     * @brief Updates the layout (for child elements).
     * @param deltaTime Time elapsed since last frame in milliseconds.
     */
    void update(unsigned long deltaTime) override;

    /**
     * @brief Draws the layout and its visible elements.
     * @param renderer Reference to the renderer.
     */
    void draw(pixelroot32::graphics::Renderer& renderer) override;

    /**
     * @brief Sets the number of columns in the grid.
     * @param cols Number of columns (must be > 0).
     */
    void setColumns(uint8_t cols);

    /**
     * @brief Gets the number of columns.
     * @return Number of columns.
     */
    uint8_t getColumns() const { return columns; }

    /**
     * @brief Gets the number of rows (calculated).
     * @return Number of rows.
     */
    uint8_t getRows() const { return rows; }

    /**
     * @brief Gets the currently selected element index.
     * @return Selected index, or -1 if none selected.
     */
    int getSelectedIndex() const { return selectedIndex; }

    /**
     * @brief Sets the selected element index.
     * @param index Index to select (-1 to deselect).
     */
    void setSelectedIndex(int index);

    /**
     * @brief Gets the selected element.
     * @return Pointer to selected element, or nullptr if none selected.
     */
    UIElement* getSelectedElement() const;

    /**
     * @brief Sets the navigation button indices.
     * @param upButton Button index for UP navigation.
     * @param downButton Button index for DOWN navigation.
     * @param leftButton Button index for LEFT navigation.
     * @param rightButton Button index for RIGHT navigation.
     */
    void setNavigationButtons(uint8_t upButton, uint8_t downButton, 
                             uint8_t leftButton, uint8_t rightButton) {
        navUpButton = upButton;
        navDownButton = downButton;
        navLeftButton = leftButton;
        navRightButton = rightButton;
    }

    /**
     * @brief Sets the style colors for selected and unselected buttons.
     * @param selectedTextCol Text color when selected.
     * @param selectedBgCol Background color when selected.
     * @param unselectedTextCol Text color when not selected.
     * @param unselectedBgCol Background color when not selected.
     */
    void setButtonStyle(pixelroot32::graphics::Color selectedTextCol,
                       pixelroot32::graphics::Color selectedBgCol,
                       pixelroot32::graphics::Color unselectedTextCol,
                       pixelroot32::graphics::Color unselectedBgCol);

private:
    uint8_t columns = 1;                                                            ///< Number of columns in the grid
    uint8_t rows = 0;                                                               ///< Number of rows (calculated)
    pixelroot32::math::Scalar cellWidth = pixelroot32::math::toScalar(0);           ///< Width of each cell
    pixelroot32::math::Scalar cellHeight = pixelroot32::math::toScalar(0);          ///< Height of each cell
    int selectedIndex = -1;                                                         ///< Currently selected element index
    uint8_t navUpButton = 0;                                                        ///< Button index for UP navigation
    uint8_t navDownButton = 1;                                                      ///< Button index for DOWN navigation
    uint8_t navLeftButton = 2;                                                      ///< Button index for LEFT navigation
    uint8_t navRightButton = 3;                                                      ///< Button index for RIGHT navigation
    bool wasUpPressed = false;                                                      ///< Previous state of UP button (for rising edge detection)
    bool wasDownPressed = false;                                                    ///< Previous state of DOWN button (for rising edge detection)
    bool wasLeftPressed = false;                                                    ///< Previous state of LEFT button (for rising edge detection)
    bool wasRightPressed = false;                                                   ///< Previous state of RIGHT button (for rising edge detection)
    
    // Style colors for buttons
    pixelroot32::graphics::Color selectedTextColor = pixelroot32::graphics::Color::White;
    pixelroot32::graphics::Color selectedBgColor = pixelroot32::graphics::Color::Cyan;
    pixelroot32::graphics::Color unselectedTextColor = pixelroot32::graphics::Color::White;
    pixelroot32::graphics::Color unselectedBgColor = pixelroot32::graphics::Color::Black;

    /**
     * @brief Calculates the number of rows based on element count and columns.
     */
    void calculateRows();

    /**
     * @brief Calculates cell dimensions based on layout size and grid configuration.
     */
    void calculateCellDimensions();
};

}
