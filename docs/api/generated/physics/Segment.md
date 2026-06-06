# Segment

<Badge type="info" text="Struct" />

**Source:** `CollisionTypes.h`

## Description

Represents a 2D line segment for collision detection.

## Properties

| Name | Type | Description |
|------|------|-------------|
| `x1` | `pixelroot32::math::Scalar` | Start X coordinate. |
| `y1` | `pixelroot32::math::Scalar` | Start Y coordinate. |
| `x2` | `pixelroot32::math::Scalar` | End X coordinate. |
| `y2` | `pixelroot32::math::Scalar` | End Y coordinate. |

## Methods

### `bool intersects(const Circle& a, const Circle& b)`

**Description:**

Checks intersection between two circles.

**Parameters:**

- `a`: First circle.
- `b`: Second circle.

**Returns:** True if circles intersect.

### `bool intersects(const Circle& c, const pixelroot32::core::Rect& r)`

**Description:**

Checks intersection between a circle and a rectangle.

**Parameters:**

- `c`: The circle.
- `r`: The rectangle.

**Returns:** True if they intersect.

### `bool intersects(const Segment& s, const pixelroot32::core::Rect& r)`

**Description:**

Checks intersection between a line segment and a rectangle.

**Parameters:**

- `s`: The line segment.
- `r`: The rectangle.

**Returns:** True if they intersect.

### `bool sweepCircleVsRect(const Circle& start, const Circle& end, const pixelroot32::core::Rect& rect, pixelroot32::math::Scalar& tHit)`

**Description:**

Sweeps a circle against a rectangle to find time of impact.

**Parameters:**

- `start`: Circle start position.
- `end`: Circle end position.
- `rect`: The static rectangle.
- `tHit`: Reference to store the time of impact.

**Returns:** True if a hit occurs along the sweep path.
