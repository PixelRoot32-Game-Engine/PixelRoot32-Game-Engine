# UIVerticalLayout

<Badge type="info" text="Class" />

**Source:** `UIVerticalLayout.h`

**Inherits from:** [UILayout](./UILayout.md)

## Description

Vertical layout container with scroll support.

Organizes UI elements vertically, one below another. Supports scrolling
when content exceeds the visible viewport. Handles keyboard/D-pad
navigation automatically.

## Inheritance

[UILayout](./UILayout.md) → `UIVerticalLayout`

## Methods

### `void setScrollEnabled(bool enable)`

**Description:**

Enables or disables scrolling.

**Parameters:**

- `enable`: True to enable scrolling.

### `void enableScroll(bool enable)`

**Description:**

Enables or disables scrolling (alias for setScrollEnabled).

**Parameters:**

- `enable`: True to enable scrolling.

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

### `void setNavigationButtons(uint8_t upButton, uint8_t downButton)`

**Description:**

Sets the navigation button indices.

**Parameters:**

- `upButton`: Button index for UP navigation.
- `downButton`: Button index for DOWN navigation.

### `void calculateContentHeight()`

**Description:**

Calculates the total content height.

### `void updateElementVisibility()`

**Description:**

Updates element visibility based on scroll position.

### `void ensureSelectedVisible()`

**Description:**

Ensures the selected element is visible by adjusting scroll.

### `void clampScrollOffset()`

**Description:**

Clamps scroll offset to valid range.
