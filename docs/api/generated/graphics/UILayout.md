# UILayout

<Badge type="info" text="Class" />

**Source:** `UILayout.h`

**Inherits from:** [UIElement](./UIElement.md)

## Description

Base class for UI layout containers.

Layouts organize UI elements automatically, handling positioning,
spacing, and optional scrolling. Layouts are themselves UI elements
that can be added to scenes.

## Inheritance

[UIElement](./UIElement.md) → `UILayout`

## Methods

### `virtual void addElement(UIElement* element)`

**Description:**

Adds a UI element to the layout.

**Parameters:**

- `element`: Pointer to the element to add.

### `virtual void removeElement(UIElement* element)`

**Description:**

Removes a UI element from the layout.

**Parameters:**

- `element`: Pointer to the element to remove.

### `virtual void updateLayout()`

**Description:**

Recalculates positions of all elements in the layout.
Should be called automatically when elements are added/removed.

### `size_t getElementCount() const`

**Description:**

Gets the number of elements in the layout.

**Returns:** Element count.

### `UIElement* getElement(size_t index) const`

**Description:**

Gets the element at a specific index.

**Parameters:**

- `index`: Element index.

**Returns:** Pointer to the element, or nullptr if index is invalid.

### `void clearElements()`

**Description:**

Clears all elements from the layout.

### `void setScrollingEnabled(bool enabled)`

**Description:**

Enables or disables scrolling for this layout.

**Parameters:**

- `enabled`: True to enable scrolling, false to disable.

### `bool isScrollingEnabled() const`

**Description:**

Checks if scrolling is enabled.

**Returns:** True if scrolling is enabled.
