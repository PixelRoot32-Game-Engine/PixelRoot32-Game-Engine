# TileBehaviorLayer

<Badge type="info" text="Struct" />

**Source:** `TileAttributes.h`

## Description

Runtime representation of exported behavior layer for O(1) flag lookup.

This structure matches the format exported by the Tilemap Editor
and provides efficient access to tile behavior flags without runtime strings.

## Properties

| Name | Type | Description |
|------|------|-------------|
| `uint8_t` | `const` | Pointer to dense uint8_t array (1 byte per tile) |
| `width` | `uint16_t` | Layer width in tiles |
| `height` | `uint16_t` | Layer height in tiles |
