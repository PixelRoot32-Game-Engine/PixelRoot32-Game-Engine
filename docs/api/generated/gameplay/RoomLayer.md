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

## Methods

### `uint16_t buildRoomGraph(const RoomLayer& layer, RoomGraph<N>& graph)`

**Description:**

Populate a RoomGraph<N> from an exported room layer.

**Parameters:**

- `layer`: The exported layer.
- `graph`: The graph to fill. Rooms are appended, so a graph that already
              holds rooms will run out of capacity sooner. Connections are
              layer-local: they are remapped into graph space and can never
              reach a room this layer did not declare.

**Returns:** Number of rooms added — `layer.roomCount` when everything fit, fewer
        when the graph's capacity truncated it, 0 when the layer was
        rejected. Truncation is documented behaviour, not an error.
