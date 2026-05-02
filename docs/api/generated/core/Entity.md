# Entity

<Badge type="info" text="Class" />

**Source:** `Entity.h`

## Description

Abstract base class for all game objects.

Entities are the fundamental building blocks of the scene. They have a position,
size, and lifecycle methods (update, draw).

Uses adaptable Scalar type for position to ensure consistent physics across platforms.

## Properties

| Name | Type | Description |
|------|------|-------------|
| `position` | `pixelroot32::math::Vector2` | Position in world space. |
| `width` | `int` | Width and Height of the entity. |
| `type` | `EntityType` | The specific type of this entity. |
| `isVisible` | `bool` | If false, the entity's draw method will not be called. |
| `isEnabled` | `bool` | If false, the entity's update method will not be called. |

## Methods

### `virtual void setVisible(bool v)`

**Description:**

Sets the visibility of the entity.

**Parameters:**

- `v`: true to show, false to hide.

### `virtual void setEnabled(bool e)`

**Description:**

Sets the enabled state of the entity.

**Parameters:**

- `e`: true to enable, false to disable.

### `unsigned char getRenderLayer() const`

**Description:**

Gets the current render layer.

**Returns:** The layer index (0-255).

### `virtual void setRenderLayer(unsigned char layer)`

**Description:**

Sets the render layer.

**Parameters:**

- `layer`: The layer index (0 to MaxLayers-1). Clamped if exceeded.

### `virtual void update(unsigned long deltaTime)`

**Description:**

Updates the entity's logic.

**Parameters:**

- `deltaTime`: Time elapsed since the last frame in milliseconds.
