# SpriteAnimation

<Badge type="info" text="Struct" />

**Source:** `Renderer.h`

## Description

Lightweight, step-based sprite animation controller.

SpriteAnimation owns no memory. It references a compile-time array of
SpriteAnimationFrame entries and exposes simple integer-based control:

- step(): advance to the next frame (wrapping at frameCount)
- reset(): go back to frame 0
- getCurrentSprite()/getCurrentMultiSprite(): query current frame data

The animation object never draws anything; Actors remain responsible for
asking which frame to render and calling Renderer accordingly.

Initially this struct is used for "step-based" animation (advance once per
logical event, such as a horde movement). The design can be extended later
with time-based advancement without changing Renderer.

## Properties

| Name | Type | Description |
|------|------|-------------|
| `SpriteAnimationFrame` | `const` | Pointer to immutable frame table. |
| `frameCount` | `uint8_t` | Number of frames in the table. |
| `current` | `uint8_t` | Current frame index [0, frameCount). |

## Methods

### `void reset()`

**Description:**

Reset the animation to the first frame.

### `void step()`

**Description:**

Advance to the next frame in a loop (step-based advancement).

### `const SpriteAnimationFrame& getCurrentFrame() const`

**Description:**

Get the current frame descriptor (may contain either type of sprite).

**Returns:** Reference to the current frame descriptor.

### `const Sprite* getCurrentSprite() const`

**Description:**

Convenience helper: returns the current simple Sprite, if any.

**Returns:** Pointer to the current simple Sprite, or nullptr if none.

### `const MultiSprite* getCurrentMultiSprite() const`

**Description:**

Convenience helper: returns the current MultiSprite, if any.

**Returns:** Pointer to the current MultiSprite, or nullptr if none.
