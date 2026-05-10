# TileAnimation

<Badge type="info" text="Struct" />

**Source:** `TileAnimation.h`

## Description

Single tile animation definition (compile-time constant).

Defines a sequence of tile indices that form an animation loop.
All data stored in PROGMEM/flash to minimize RAM usage.

Memory Layout: 4 bytes total (POD structure)
- baseTileIndex: First tile in animation sequence (0-255)
- frameCount: Number of frames in animation (1-255) 
- frameDuration: Hold each animation cell for this many **60 Hz ticks** (1-255).
  Logical tick rate is capped at 60/s (wall clock), independent of Engine loop speed.
- reserved: Padding for alignment (future use)

Example: Water animation with 4 frames, 8 ticks per frame (~133 ms per cell at 60 Hz)
{ baseTileIndex: 42, frameCount: 4, frameDuration: 8, reserved: 0 }

This structure is stored in PROGMEM (flash memory)
sizeof(TileAnimation) == 4 bytes

::: tip
This structure is stored in PROGMEM (flash memory)
:::

::: tip
sizeof(TileAnimation) == 4 bytes
:::

## Properties

| Name | Type | Description |
|------|------|-------------|
| `baseTileIndex` | `uint8_t` | First tile in sequence (e.g., 42) |
| `frameCount` | `uint8_t` | Number of frames (e.g., 4) |
| `frameDuration` | `uint8_t` | Ticks per frame (e.g., 8) |
| `reserved` | `uint8_t` | Padding for alignment/future use |
