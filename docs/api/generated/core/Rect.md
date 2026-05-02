# Rect

<Badge type="info" text="Struct" />

**Source:** `Entity.h`

## Description

Represents a 2D rectangle, typically used for hitboxes or bounds.

Uses adaptable Scalar type for coordinates to support both float and fixed-point math.

## Properties

| Name | Type | Description |
|------|------|-------------|
| `position` | `pixelroot32::math::Vector2` | Top-left corner coordinates. |
| `width` | `int` | Dimensions of the rectangle. |

## Methods

### `bool intersects(const Rect& other) const`

**Description:**

Checks if this rectangle intersects with another.

**Parameters:**

- `other`: The other rectangle to check against.

**Returns:** true if the rectangles overlap, false otherwise.
