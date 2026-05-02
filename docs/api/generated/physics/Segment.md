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
