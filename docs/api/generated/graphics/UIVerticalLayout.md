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

### `void setViewportHeight(pixelroot32::math::Scalar h)`

**Description:**

Sets the viewport height (visible area).

**Parameters:**

- `h`: Viewport height in pixels.

### `pixelroot32::math::Scalar getScrollOffset() const`

**Description:**

Gets the current scroll offset.

**Returns:** Scroll offset in pixels.

### `void setScrollOffset(pixelroot32::math::Scalar offset)`

**Description:**

Sets the scroll offset directly.

**Parameters:**

- `offset`: Scroll offset in pixels.

### `pixelroot32::math::Scalar getContentHeight() const`

**Description:**

Gets the total content height.

**Returns:** Content height in pixels.

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

### `void setScrollSpeed(pixelroot32::math::Scalar speed)`

**Description:**

Sets the scroll speed for smooth scrolling.

**Parameters:**

- `speed`: Pixels per millisecond.

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
