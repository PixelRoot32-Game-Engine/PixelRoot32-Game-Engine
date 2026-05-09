# UIPanel

<Badge type="info" text="Class" />

**Source:** `UIPanel.h`

**Inherits from:** [UIElement](./UIElement.md)

## Description

Visual container that draws a background and border around a child element.

This container provides a retro-style window/panel appearance with a background
color and border. Typically contains a UILayout or other UI elements. Useful for
dialogs, menus, and information panels.

## Inheritance

[UIElement](./UIElement.md) → `UIPanel`

## Methods

### `void setChild(UIElement* element)`

**Description:**

Sets the child element.

**Parameters:**

- `element`: Pointer to the UI element to wrap (typically a UILayout).

### `UIElement* getChild() const`

**Description:**

Gets the child element.

**Returns:** Pointer to the child element, or nullptr if none set.

### `void setBackgroundColor(pixelroot32::graphics::Color color)`

**Description:**

Sets the background color.

**Parameters:**

- `color`: Background color.

### `pixelroot32::graphics::Color getBackgroundColor() const`

**Description:**

Gets the background color.

**Returns:** Background color.

### `void setBorderColor(pixelroot32::graphics::Color color)`

**Description:**

Sets the border color.

**Parameters:**

- `color`: Border color.

### `pixelroot32::graphics::Color getBorderColor() const`

**Description:**

Gets the border color.

**Returns:** Border color.

### `void setBorderWidth(uint8_t width)`

**Description:**

Sets the border width.

**Parameters:**

- `width`: Border width in pixels.

### `uint8_t getBorderWidth() const`

**Description:**

Gets the border width.

**Returns:** Border width in pixels.

### `void updateChildPosition()`

**Description:**

Updates the child element's position to match the panel.
