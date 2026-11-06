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
| `rooms` | `const RoomData*` | Pointer to the exported room array (may be null). |
| `roomCount` | `uint16_t` | Number of entries in `rooms`. |
| `tileWidth` | `uint8_t` | Cell stride X in world units (32 for 2:1 iso, not the art width). MUST be >= 1. |
| `tileHeight` | `uint8_t` | Cell stride Y in world units (16 for 2:1 iso, not the art height). MUST be >= 1. |
