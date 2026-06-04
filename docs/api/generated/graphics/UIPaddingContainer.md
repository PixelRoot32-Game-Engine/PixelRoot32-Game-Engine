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

### `void setPadding(pixelroot32::math::Scalar p)`

**Description:**

Sets uniform padding on all sides.

**Parameters:**

- `p`: Padding value in pixels.

### `void setPadding(pixelroot32::math::Scalar left, pixelroot32::math::Scalar right, pixelroot32::math::Scalar top, pixelroot32::math::Scalar bottom)`

**Description:**

Sets asymmetric padding.

**Parameters:**

- `left`: Left padding in pixels.
- `right`: Right padding in pixels.
- `top`: Top padding in pixels.
- `bottom`: Bottom padding in pixels.

### `pixelroot32::math::Scalar getPaddingLeft() const`

**Description:**

Gets the left padding.

**Returns:** Left padding in pixels.

### `pixelroot32::math::Scalar getPaddingRight() const`

**Description:**

Gets the right padding.

**Returns:** Right padding in pixels.

### `pixelroot32::math::Scalar getPaddingTop() const`

**Description:**

Gets the top padding.

**Returns:** Top padding in pixels.

### `pixelroot32::math::Scalar getPaddingBottom() const`

**Description:**

Gets the bottom padding.

**Returns:** Bottom padding in pixels.

### `void setPosition(pixelroot32::math::Scalar newX, pixelroot32::math::Scalar newY)`

**Description:**

Sets the position of the container.
Also updates the child element's position.

**Parameters:**

- `newX`: New X coordinate.
- `newY`: New Y coordinate.

### `void update(unsigned long deltaTime)`

**Description:**

Updates the container and child element.

**Parameters:**

- `deltaTime`: Time elapsed since last frame in milliseconds.

### `void draw(pixelroot32::graphics::Renderer& renderer)`

**Description:**

Draws the child element.

**Parameters:**

- `renderer`: Reference to the renderer.

### `void updateChildPosition()`

**Description:**

Updates the child element's position based on padding.
