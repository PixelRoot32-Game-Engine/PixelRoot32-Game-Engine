# SDL2_Drawer

<Badge type="info" text="Class" />

**Source:** `SDL2_Drawer.h`

**Inherits from:** [BaseDrawSurface](../graphics/BaseDrawSurface.md)

## Description

SDL2-backed draw surface for native desktop builds.

## Inheritance

[BaseDrawSurface](../graphics/BaseDrawSurface.md) → `SDL2_Drawer`

## Methods

### `uint8_t* getSpriteBuffer()`

**Description:**

Get pointer to sprite buffer (not supported in SDL2).

### `void setTouchDispatcher(pixelroot32::input::TouchEventDispatcher* touchDispatcher)`

**Description:**

Set the TouchEventDispatcher to receive mouse events.

**Parameters:**

- `touchDispatcher`: Pointer to the Engine's TouchEventDispatcher.

This is the preferred method when PIXELROOT32_ENABLE_TOUCH is enabled.
Mouse events are mapped directly to touch events.

### `void setInputManager(pixelroot32::input::InputManager* inputManager)`

**Description:**

Set the InputManager for backwards compatibility.

**Parameters:**

- `inputManager`: Pointer to the InputManager instance.

Deprecated: Use setTouchDispatcher() instead.
