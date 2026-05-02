# UIPaddingContainer

<Badge type="info" text="Class" />

**Source:** `UIPaddingContainer.h`

**Inherits from:** [UIElement](./UIElement.md)

## Description

Container that wraps a single UI element and applies padding.

This container adds padding/margin around a single child element without
organizing multiple elements. Useful for adding spacing to individual
elements or nesting layouts with custom padding.

## Inheritance

[UIElement](./UIElement.md) → `UIPaddingContainer`

## Methods

### `void setChild(UIElement* element)`

**Description:**

Sets the child element.

**Parameters:**

- `element`: Pointer to the UI element to wrap.

### `UIElement* getChild() const`

**Description:**

Gets the child element.

**Returns:** Pointer to the child element, or nullptr if none set.

### `void updateChildPosition()`

**Description:**

Updates the child element's position based on padding.
