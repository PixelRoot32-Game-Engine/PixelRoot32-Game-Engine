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
