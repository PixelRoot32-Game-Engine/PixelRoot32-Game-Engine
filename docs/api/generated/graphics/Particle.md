# Particle

<Badge type="info" text="Struct" />

**Source:** `Particle.h`

## Description

Represents a single particle in the particle system.

Designed to be lightweight to fit many instances in memory (RAM optimization).

## Properties

| Name | Type | Description |
|------|------|-------------|
| `position` | `pixelroot32::math::Vector2` | Current position. |
| `velocity` | `pixelroot32::math::Vector2` | Velocity vector. |
| `color` | `uint16_t` | Current color (RGB565). |
| `startColor` | `Color` | Initial color for interpolation. |
| `endColor` | `Color` | Final color for interpolation. |
| `life` | `uint8_t` | Current remaining life (frames or ticks). |
| `maxLife` | `uint8_t` | Total life duration. |
| `active` | `bool` | Whether the particle is currently in use. |
