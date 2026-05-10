# MultiSprite

<Badge type="info" text="Struct" />

**Source:** `Renderer.h`

## Description

Multi-layer, multi-color sprite built from 1bpp layers.

A MultiSprite combines several SpriteLayer entries that share the same
width and height. Layers are drawn in array order, allowing more complex
visuals (highlights, outlines) while keeping each layer 1bpp.

This design keeps compatibility with the existing Sprite format while
enabling NES/GameBoy-style layered sprites.

## Properties

| Name | Type | Description |
|------|------|-------------|
| `width` | `uint8_t` | Sprite width in pixels (<= 16). |
| `height` | `uint8_t` | Sprite height in pixels. |
| `SpriteLayer` | `const` | Pointer to array of layers. |
| `layerCount` | `uint8_t` | Number of layers in the array. |
