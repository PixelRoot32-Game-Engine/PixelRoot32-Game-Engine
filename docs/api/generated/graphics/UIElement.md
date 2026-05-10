# UIElement

<Badge type="info" text="Class" />

**Source:** `UIElement.h`

**Inherits from:** [Entity](../core/Entity.md)

## Description

Base class for all user interface elements (buttons, labels, etc.).

Integrates with the scene graph and sets EntityType to UI_ELEMENT.

## Inheritance

[Entity](../core/Entity.md) → `UIElement`

## Methods

### `UIElementType getType() const`

**Description:**

Gets the type of the UI element.

**Returns:** The UIElementType.

### `virtual bool isFocusable() const`

**Description:**

Checks if the element is focusable/selectable.
Use this for navigation logic.

**Returns:** true if focusable, false otherwise.

### `void setFixedPosition(bool fixed)`

**Description:**

Sets whether the element is in a fixed position (HUD/Overlay).

**Parameters:**

- `fixed`: True to enable fixed position.

### `bool isFixedPosition() const`

**Description:**

Checks if the element is in a fixed position.

**Returns:** True if fixed position is enabled.

### `virtual void setPosition(pixelroot32::math::Scalar newX, pixelroot32::math::Scalar newY)`

**Description:**

Sets the position of the element.

**Parameters:**

- `newX`: New X coordinate.
- `newY`: New Y coordinate.

### `virtual void getPreferredSize(pixelroot32::math::Scalar& preferredWidth, pixelroot32::math::Scalar& preferredHeight) const`

**Description:**

Gets the preferred size of the element.
Used by layouts to determine how much space the element needs.

**Parameters:**

- `preferredWidth`: Output parameter for preferred width (or -1 if flexible).
- `preferredHeight`: Output parameter for preferred height (or -1 if flexible).
