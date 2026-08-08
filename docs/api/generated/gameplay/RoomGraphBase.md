# RoomGraphBase

<Badge type="info" text="Class" />

**Source:** `RoomGraph.h`

## Description

Abstract base class for RoomGraph&lt;N> used by Scene via type erasure.

Exposes only the polymorphic API Scene actually needs (enterRoom,
currentRoomIndex, isValidIdx) so Scene does not need to know the template
parameter N. RoomGraph&lt;N> publicly inherits from this base and satisfies
the contract with inline implementations.

Pure-virtual: subclasses MUST implement every method. Bodies live inline
in the RoomGraph&lt;N> template — no separate .cpp file.

## Methods

### `virtual void enterRoom(uint16_t idx, graphics::Camera2D* camera)`

**Description:**

Enter a room by index.

**Parameters:**

- `idx`: Room index (no-op when out of range)
- `camera`: Camera2D pointer (may be nullptr)

### `virtual uint16_t currentRoomIndex() const`

**Description:**

Get the current room index (0xFFFF if none entered yet).

### `virtual bool isValidIdx(uint16_t idx) const`

**Description:**

Check if a room index is valid (idx < roomCount).
