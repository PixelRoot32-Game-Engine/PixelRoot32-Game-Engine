# RoomGraph

<Badge type="info" text="Class" />

**Source:** `RoomGraph.h`

**Inherits from:** [RoomGraphBase](./RoomGraphBase.md)

## Description

Fixed-capacity graph of rooms with camera rects and connections.

N Max number of rooms (compile-time constant, must be >= 1).

Zero-heap, zero-allocation. All arrays are fixed-size. The graph is built
once in Scene::init() and is read-only thereafter.

Entering a room updates the camera bounds via Camera2D::setBounds /
Camera2D::setVerticalBounds and fires an optional onEnter callback.
Entity movement across rooms is the game's responsibility — RoomGraph
only provides data and camera management.

## Inheritance

[RoomGraphBase](./RoomGraphBase.md) → `RoomGraph`

## Methods

### `uint16_t addRoom(math::Scalar cameraMinX, math::Scalar cameraMinY, math::Scalar cameraMaxX, math::Scalar cameraMaxY)`

**Description:**

Add a room to the graph.

**Parameters:**

- `cameraMinX`: Left camera bound (world units)
- `cameraMinY`: Top camera bound (world units)
- `cameraMaxX`: Right camera bound (world units)
- `cameraMaxY`: Bottom camera bound (world units)

**Returns:** Room index (0 .. N-1), or 0xFFFF if full.

### `void setTileWindow(uint16_t roomIdx, int16_t originCol, int16_t originRow, int16_t cols, int16_t rows)`

**Description:**

Set the tile window for a room.

**Parameters:**

- `roomIdx`: Room index
- `originCol`: Tile window origin column
- `originRow`: Tile window origin row
- `cols`: Tile window width in tiles
- `rows`: Tile window height in tiles

No-op when roomIdx is out of range.

### `void connect(uint16_t fromIdx, uint16_t toIdx, RoomDir dir)`

**Description:**

Connect two rooms in a cardinal direction.

**Parameters:**

- `fromIdx`: Source room index
- `toIdx`: Target room index
- `dir`: Direction from source to target

No-op when either index is out of range. Re-connecting an already-
occupied direction slot does not double-count the connection.

### `void enterRoom(uint16_t idx, graphics::Camera2D* camera)`

**Description:**

Enter a room by index.

**Parameters:**

- `idx`: Room index
- `camera`: Pointer to Camera2D (may be nullptr)

No-op when idx is out of range.

### `void setOnEnter(void (*fn)(int fromIdx, int toIdx, void* userData), void* userData = nullptr)`

**Description:**

Register an onEnter callback.

**Parameters:**

- `fn`: Plain function pointer (not std::function — zero heap)
- `userData`: Context pointer passed through to the callback

### `uint16_t currentRoomIndex() const`

**Description:**

Get the current room index.

**Returns:** 0xFFFF if enterRoom() has never been called.

### `const Room& getRoom(uint16_t idx) const`

**Description:**

Get a read-only reference to a room by index.

### `uint16_t roomCount() const`

**Description:**

Number of rooms currently in the graph.

### `bool isValidIdx(uint16_t idx) const`

**Description:**

Check if a room index is valid (idx < roomCount()).

### `uint8_t getConnections(uint16_t idx, int out[], uint8_t maxOut) const`

**Description:**

Copy up to maxOut connection target indices into a
       caller-provided buffer.

**Parameters:**

- `idx`: Room index (no-op when out of range)
- `out`: Output buffer — must be non-null when maxOut > 0
- `maxOut`: Maximum number of connections to copy

**Returns:** Number of connections actually copied (≤ maxOut).

Truncation is documented, not an error: when the room has more
connections than maxOut, only maxOut entries are written.
