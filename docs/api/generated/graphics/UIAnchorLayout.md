# UIAnchorLayout

<Badge type="info" text="Class" />

**Source:** `UIAnchorLayout.h`

**Inherits from:** [UILayout](./UILayout.md)

## Description

Layout that positions elements at fixed anchor points on the screen.

This layout positions UI elements at fixed anchor points (corners, center, etc.)
without reflow. Very efficient for HUDs, debug UI, and fixed-position elements.
Positions are calculated once or when screen size changes.

## Inheritance

[UILayout](./UILayout.md) → `UIAnchorLayout`

## Methods

### `void addElement(UIElement* element, Anchor anchor)`

**Description:**

Adds a UI element with a specific anchor point.

**Parameters:**

- `element`: Pointer to the element to add.
- `anchor`: Anchor point for positioning.

### `void setScreenSize(int screenWidth, int screenHeight)`

**Description:**

Sets the screen size for anchor calculations.

**Parameters:**

- `screenWidth`: Screen width in pixels.
- `screenHeight`: Screen height in pixels.

### `pixelroot32::math::Scalar getScreenWidth() const`

**Description:**

Gets the screen width.

**Returns:** Screen width in pixels.

### `pixelroot32::math::Scalar getScreenHeight() const`

**Description:**

Gets the screen height.

**Returns:** Screen height in pixels.

### `void calculateAnchorPosition(UIElement* element, Anchor anchor, pixelroot32::math::Scalar& outX, pixelroot32::math::Scalar& outY) const`

**Description:**

Calculates position for an element based on its anchor.

**Parameters:**

- `element`: The element to position.
- `anchor`: The anchor point.
- `outX`: Output parameter for calculated X position.
- `outY`: Output parameter for calculated Y position.
