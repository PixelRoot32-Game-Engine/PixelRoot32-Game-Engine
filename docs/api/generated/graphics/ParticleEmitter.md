# ParticleEmitter

<Badge type="info" text="Class" />

**Source:** `ParticleEmitter.h`

**Inherits from:** [Entity](../core/Entity.md)

## Description

Manages a pool of particles to create visual effects.

Participates in the scene update/draw loop.
Uses a fixed-size array for particles to avoid dynamic allocation during runtime.

## Inheritance

[Entity](../core/Entity.md) → `ParticleEmitter`

## Methods

### `void burst(pixelroot32::math::Vector2 position, int count)`

**Description:**

Emits a burst of particles from a specific location.

**Parameters:**

- `position`: Emission origin.
- `count`: Number of particles to spawn.

### `inline uint16_t lerpColor(uint16_t c1, uint16_t c2, pixelroot32::math::Scalar t)`

**Description:**

Linear interpolation between two 16-bit RGB565 colors.

**Parameters:**

- `c1`: Start color.
- `c2`: End color.
- `t`: Interpolation factor (0.0 - 1.0).

**Returns:** Interpolated RGB565 color.
