# RoomLayer

<Badge type="info" text="Struct" />

**Source:** `RoomLayout.h`

## Description

Runtime view over an exported room array — the room-graph analogue
       of `physics::TileBehaviorLayer`.

Holds no ownership: `rooms` points at the editor's flash-resident array.

## Properties

| Name | Type | Description |
|------|------|-------------|
| `RoomData` | `const` | Pointer to the exported room array (may be null). |
| `roomCount` | `uint16_t` | Number of entries in `rooms`. |
| `tileWidth` | `uint8_t` | Tile width in world units. MUST be >= 1. |
| `tileHeight` | `uint8_t` | Tile height in world units. MUST be >= 1. |
