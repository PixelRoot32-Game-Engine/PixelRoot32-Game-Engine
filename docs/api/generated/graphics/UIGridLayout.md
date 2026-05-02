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

### `void calculateRows()`

**Description:**

Calculates the number of rows based on element count and columns.

### `void calculateCellDimensions()`

**Description:**

Calculates cell dimensions based on layout size and grid configuration.
