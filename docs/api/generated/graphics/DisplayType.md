# DisplayType

<Badge type="info" text="Enum" />

**Source:** `DisplayConfig.h`

## Description

Identifies the type of display driver to use.

## Methods

### `static DisplayConfig createCustom(DrawSurface* surface, uint16_t w, uint16_t h, int rot = 0)`

**Description:**

Static factory to create a DisplayConfig with a custom DrawSurface.

**Parameters:**

- `surface`: Pointer to the custom DrawSurface implementation (ownership is transferred).
- `w`: Physical and logical width.
- `h`: Physical and logical height.
- `rot`: Rotation.

### `bool needsScaling() const`

**Description:**

Checks if scaling is needed (logical != physical).

**Returns:** true if the engine needs to scale output.

### `float getScaleX() const`

**Description:**

Gets horizontal scaling factor.

**Returns:** Scale factor (physical / logical).

### `float getScaleY() const`

**Description:**

Gets vertical scaling factor.

**Returns:** Scale factor (physical / logical).

### `uint16_t width() const`

**Description:**

Deprecated, gets logical width.

**Returns:** The logical width.

### `uint16_t height() const`

**Description:**

Deprecated, gets logical height.

**Returns:** The logical height.

### `DrawSurface& getDrawSurface() const`

**Description:**

Gets the underlying DrawSurface implementation.

**Returns:** Reference to the DrawSurface.

### `void initDrawSurface()`

**Description:**

Initializes the underlying draw surface.
