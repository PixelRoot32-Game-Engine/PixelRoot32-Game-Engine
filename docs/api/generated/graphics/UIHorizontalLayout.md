# UIHorizontalLayout

<Badge type="info" text="Class" />

**Source:** `UIHorizontalLayout.h`

**Inherits from:** [UILayout](./UILayout.md)

## Description

Horizontal layout container with scroll support.

Organizes UI elements horizontally, one next to another. Supports scrolling
when content exceeds the visible viewport. Handles keyboard/D-pad
navigation automatically.

## Inheritance

[UILayout](./UILayout.md) → `UIHorizontalLayout`

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

Handles input for navigation and scrolling.

**Parameters:**

- `input`: Reference to the InputManager.

### `void update(unsigned long deltaTime)`

**Description:**

Updates the layout (handles smooth scrolling).

**Parameters:**

- `deltaTime`: Time elapsed since last frame in milliseconds.

### `void draw(pixelroot32::graphics::Renderer& renderer)`

**Description:**

Draws the layout and its visible elements.

**Parameters:**

- `renderer`: Reference to the renderer.

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

### `void setViewportWidth(pixelroot32::math::Scalar w)`

**Description:**

Sets the viewport width (visible area).

**Parameters:**

- `w`: Viewport width in pixels.

### `pixelroot32::math::Scalar getScrollOffset() const`

**Description:**

Gets the current scroll offset.

**Returns:** Scroll offset in pixels.

### `void setScrollOffset(pixelroot32::math::Scalar offset)`

**Description:**

Sets the scroll offset directly.

**Parameters:**

- `offset`: Scroll offset in pixels.

### `pixelroot32::math::Scalar getContentWidth() const`

**Description:**

Gets the total content width.

**Returns:** Content width in pixels.

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

### `void setNavigationButtons(uint8_t leftButton, uint8_t rightButton)`

**Description:**

Sets the navigation button indices.

**Parameters:**

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

### `void calculateContentWidth()`

**Description:**

Calculates the total content width.

### `void updateElementVisibility()`

**Description:**

Updates element visibility based on scroll position.

### `void ensureSelectedVisible()`

**Description:**

Ensures the selected element is visible by adjusting scroll.

### `void clampScrollOffset()`

**Description:**

Clamps scroll offset to valid range.
