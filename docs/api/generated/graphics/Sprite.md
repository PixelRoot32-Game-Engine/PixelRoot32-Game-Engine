# Sprite

<Badge type="info" text="Struct" />

**Source:** `Renderer.h`

## Description

Compact sprite descriptor for monochrome bitmapped sprites.

Sprites are stored as an array of 16-bit rows. Each row packs horizontal
pixels into bits, using the following convention:

- Bit (width - 1) represents the leftmost pixel of the row.
- Bit 0 represents the rightmost pixel of the row.

A binary literal therefore reads left to right as the row is drawn when
the sprite is 16 pixels wide (`0b1000000000000000` lights only the leftmost
pixel). Only the lowest (width) bits of each row are used. A bit value of 1 means
"pixel on", 0 means "pixel off".

This format is optimized for small microcontroller displays (NES/GameBoy
style assets) and keeps data in flash-friendly, constexpr-friendly form.

## Properties

| Name | Type | Description |
|------|------|-------------|
| `data` | `const uint16_t*` | Pointer to packed row data (size = height). |
| `width` | `uint8_t` | Sprite width in pixels (<= 16). |
| `height` | `uint8_t` | Sprite height in pixels. |
