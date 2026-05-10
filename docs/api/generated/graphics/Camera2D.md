# Camera2D

<Badge type="info" text="Class" />

**Source:** `Camera2D.h`

## Description

2D camera for viewport management and smooth scrolling.

Manages the viewable area of a scene, providing bounds clamping and
smooth follow-target behavior. The camera sits at a fixed position
until instructed to follow a target; its X/Y coordinates are negated
to become the Renderer's X/Y offset (so content scrolls "under" the camera).

Bounds are enforced per-axis; if no bounds are set the camera is unclamped
on that axis. A target X or Y position outside bounds is clamped before
the camera moves.

Camera2D does not own a Renderer. Call apply(renderer) to push the
      camera transform after updating.

::: tip
Camera2D does not own a Renderer. Call apply(renderer) to push the
      camera transform after updating.
:::

## Methods

### `void setBounds(pixelroot32::math::Scalar minX, pixelroot32::math::Scalar maxX)`

### `void setVerticalBounds(pixelroot32::math::Scalar minY, pixelroot32::math::Scalar maxY)`

### `void setPosition(pixelroot32::math::Vector2 position)`

### `void followTarget(pixelroot32::math::Scalar targetX)`

### `void followTarget(pixelroot32::math::Vector2 target)`

### `pixelroot32::math::Scalar getX() const`

### `pixelroot32::math::Scalar getY() const`

### `pixelroot32::math::Vector2 getPosition() const`

### `void apply(Renderer& renderer) const`

**Description:**

Pushes the camera transform into the renderer (sets display offset).

### `void setViewportSize(int width, int height)`
