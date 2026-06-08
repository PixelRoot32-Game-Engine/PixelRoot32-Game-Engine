# TouchStateMachine

<Badge type="info" text="Class" />

**Source:** `TouchStateMachine.h`

## Description

State machine for touch gesture detection

State transitions:
Idle → Pressed (on touch down)
Pressed → LongPress (after LONG_PRESS_THRESHOLD without release)
Pressed → Dragging (after DRAG_THRESHOLD movement)
Pressed → Idle (on touch up → generate Click/DoubleClick)
LongPress → Idle (on touch up)
Dragging → Idle (on touch up → generate DragEnd)

O(1) update - deterministic timing

## Methods

### `void update(uint8_t touchId, bool pressed, int16_t x, int16_t y, uint32_t timestamp, TouchEventQueue& outputQueue)`

**Description:**

Process a touch input update

**Parameters:**

- `touchId`: Touch identifier (0-4)
- `pressed`: True if touch is currently pressed
- `x`: Current X position
- `y`: Current Y position
- `timestamp`: Current timestamp in ms
- `outputQueue`: Queue to enqueue generated events

### `void reset(uint8_t touchId)`

**Description:**

Reset state for a specific touch ID

**Parameters:**

- `touchId`: Touch identifier to reset

### `void resetAll()`

**Description:**

Reset all touch states

### `TouchState getState(uint8_t touchId) const`

**Description:**

Get current state for a touch ID

**Parameters:**

- `touchId`: Touch identifier

**Returns:** Current state

### `bool isActive() const`

**Description:**

Check if a touch is in progress

**Returns:** true if any touch is active

### `uint32_t getPressDuration(uint8_t touchId, uint32_t currentTime) const`

**Description:**

Get time since press for a touch

**Parameters:**

- `touchId`: Touch identifier
- `currentTime`: Current timestamp in ms (same source as update())

**Returns:** Milliseconds since press started, 0 if not pressed

### `static int16_t distance(int16_t x1, int16_t y1, int16_t x2, int16_t y2)`

**Description:**

Calculate Manhattan distance between two points

### `void handleTouchDown(uint8_t touchId, int16_t x, int16_t y, uint32_t timestamp, TouchEventQueue& queue)`

**Description:**

Handle touch down event

### `void handleTouchUp(uint8_t touchId, int16_t x, int16_t y, uint32_t timestamp, TouchEventQueue& queue)`

**Description:**

Handle touch up event

### `void handleTouchMove(uint8_t touchId, int16_t x, int16_t y, uint32_t timestamp, TouchEventQueue& queue)`

**Description:**

Handle touch move while pressed

### `void checkLongPress(uint8_t touchId, uint32_t timestamp, TouchEventQueue& queue)`

**Description:**

Check and generate long press if needed
