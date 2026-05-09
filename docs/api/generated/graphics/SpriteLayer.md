# SpriteLayer

<Badge type="info" text="Struct" />

**Source:** `Renderer.h`

## Description

Single monochrome layer used by layered sprites.

Each layer uses the same width/height as its owning MultiSprite but can
provide its own bitmap and color.

## Properties

| Name | Type | Description |
|------|------|-------------|
| `uint16_t` | `const` | Pointer to packed row data for this layer. |
| `color` | `Color` | Color used for "on" pixels in this layer. |
