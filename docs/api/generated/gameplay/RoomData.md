# RoomData

<Badge type="info" text="Struct" />

**Source:** `RoomLayout.h`

## Description

One exported room: its tile-space rect plus four connection slots.

Coordinates are in TILES, not world units — that is the space the editor
authors in. `buildRoomGraph()` multiplies by the layer's tile size to get
the world-space camera rect.

Connection slots are indexed by `RoomDir` (0 = Up, 1 = Down, 2 = Left,
3 = Right) and hold the target room's index within the same layer, or
`kNoRoomConnection` when that direction is a wall.

Trivially copyable by design: the editor emits these as a `static const`
array so the linker parks them in flash, costing zero SRAM.

## Properties

| Name | Type | Description |
|------|------|-------------|
| `originCol` | `uint16_t` | Tile column of the room's left edge. |
| `originRow` | `uint16_t` | Tile row of the room's top edge. |
| `cols` | `uint16_t` | Room width in tiles. |
| `rows` | `uint16_t` | Room height in tiles. |
| `connections` | `uint16_t` | Target room index by direction, or kNoRoomConnection. |
