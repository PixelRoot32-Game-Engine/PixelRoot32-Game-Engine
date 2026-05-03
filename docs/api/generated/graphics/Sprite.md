# Sprite

<Badge type="info" text="Struct" />

**Source:** `Renderer.h`

## Description

Compact sprite descriptor for monochrome bitmapped sprites.

Sprites are stored as an array of 16-bit rows. Each row packs horizontal
pixels into bits, using the following convention:

- Bit 0 represents the leftmost pixel of the row.
- Bit (width - 1) represents the rightmost pixel of the row.

Only the lowest (width) bits of each row are used. A bit value of 1 means
"pixel on", 0 means "pixel off".

This format is optimized for small microcontroller displays (NES/GameBoy
style assets) and keeps data in flash-friendly, constexpr-friendly form.

## Properties

| Name | Type | Description |
|------|------|-------------|
| `uint16_t` | `const` | Pointer to packed row data (size = height). |
| `width` | `uint8_t` | Sprite width in pixels (<= 16). |
| `height` | `uint8_t` | Sprite height in pixels. |
