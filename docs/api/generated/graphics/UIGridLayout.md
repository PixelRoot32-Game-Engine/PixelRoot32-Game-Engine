# UIGridLayout

<Badge type="info" text="Class" />

**Source:** `UIGridLayout.h`

**Inherits from:** [UILayout](./UILayout.md)

## Description

Grid layout container for organizing elements in a matrix.

Organizes UI elements in a fixed grid of rows and columns. Supports
navigation in 4 directions (UP/DOWN/LEFT/RIGHT) and automatic
positioning based on grid coordinates.

## Inheritance

[UILayout](./UILayout.md) → `UIGridLayout`

## Methods

### `void addElement(UIElement* element)`

**Description:**

Adds a UI element to the layout.

**Parameters:**

- `element`: Pointer to the element to add.

### `void removeElement(UIElement* element)`

**Description:**

Removes a UI element from the layout.

**Parameters:**

- `element`: Pointer to the element to remove.

### `void updateLayout()`

**Description:**

Recalculates positions of all elements.

### `void handleInput(const pixelroot32::input::InputManager& input)`

**Description:**

Handles input for navigation and selection.

**Parameters:**

- `input`: Reference to the InputManager.

### `void update(unsigned long deltaTime)`

**Description:**

Updates the layout (for child elements).

**Parameters:**

- `deltaTime`: Time elapsed since last frame in milliseconds.

### `void draw(pixelroot32::graphics::Renderer& renderer)`

**Description:**

Draws the layout and its visible elements.

**Parameters:**

- `renderer`: Reference to the renderer.

### `void setColumns(uint8_t cols)`

**Description:**

Sets the number of columns in the grid.

**Parameters:**

- `cols`: Number of columns (must be > 0).

### `uint8_t getColumns() const`

**Description:**

Gets the number of columns.

**Returns:** Number of columns.

### `uint8_t getRows() const`

**Description:**

Gets the number of rows (calculated).

**Returns:** Number of rows.

### `int getSelectedIndex() const`

**Description:**

Gets the currently selected element index.

**Returns:** Selected index, or -1 if none selected.

### `void setSelectedIndex(int index)`

**Description:**

Sets the selected element index.

**Parameters:**

- `index`: Index to select (-1 to deselect).

### `UIElement* getSelectedElement() const`

**Description:**

Gets the selected element.

**Returns:** Pointer to selected element, or nullptr if none selected.

### `void setNavigationButtons(uint8_t upButton, uint8_t downButton, uint8_t leftButton, uint8_t rightButton)`

**Description:**

Sets the navigation button indices.

**Parameters:**

- `upButton`: Button index for UP navigation.
- `downButton`: Button index for DOWN navigation.
- `leftButton`: Button index for LEFT navigation.
- `rightButton`: Button index for RIGHT navigation.

### `void setButtonStyle(pixelroot32::graphics::Color selectedTextCol, pixelroot32::graphics::Color selectedBgCol, pixelroot32::graphics::Color unselectedTextCol, pixelroot32::graphics::Color unselectedBgCol)`

**Description:**

Sets the style colors for selected and unselected buttons.

**Parameters:**

- `selectedTextCol`: Text color when selected.
- `selectedBgCol`: Background color when selected.
- `unselectedTextCol`: Text color when not selected.
- `unselectedBgCol`: Background color when not selected.

### `void calculateRows()`

**Description:**

Calculates the number of rows based on element count and columns.

### `void calculateCellDimensions()`

**Description:**

Calculates cell dimensions based on layout size and grid configuration.
