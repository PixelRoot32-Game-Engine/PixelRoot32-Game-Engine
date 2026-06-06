# Sprite2bpp

<Badge type="info" text="Struct" />

**Source:** `Renderer.h`

## Description

Sprite descriptor for 2bpp (4-color) multi-color sprites.

## Properties

| Name | Type | Description |
|------|------|-------------|
| `uint8_t` | `const` | Pointer to 2bpp bitmap data. |
| `Color` | `const` | Pointer to color palette (max 4 colors). |
| `width` | `uint8_t` | Sprite width in pixels. |
| `height` | `uint8_t` | Sprite height in pixels. |
| `paletteSize` | `uint8_t` | Number of colors in the palette. |
