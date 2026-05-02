# TouchPointBuffer

<Badge type="info" text="Class" />

**Source:** `TouchPointBuffer.h`

## Description

Ring buffer for storing touch points

Implements a fixed-size circular buffer with:
- O(1) push and pop operations
- No dynamic memory allocation
- Thread-safe (single consumer assumed)

Invariants:
- count <= TOUCH_MAX_POINTS
- head always points to next write position
- oldest point is at (head + 1) % TOUCH_MAX_POINTS when count > 0

## Methods

### `bool push(const TouchPoint& point)`

**Description:**

Push a touch point to the buffer

**Parameters:**

- `point`: Touch point to add

**Returns:** true if added successfully

### `bool pop(TouchPoint& outPoint)`

**Description:**

Pop the oldest touch point

**Parameters:**

- `outPoint`: Output for popped point

**Returns:** true if point was available

### `const TouchPoint* peekOldest() const`

**Description:**

Peek at oldest point without removing

**Returns:** Pointer to oldest point, nullptr if empty

### `const TouchPoint* peekNewest() const`

**Description:**

Peek at newest point without removing

**Returns:** Pointer to newest point, nullptr if empty

### `const TouchPoint* at(uint8_t index) const`

**Description:**

Get point at specific index

**Parameters:**

- `index`: Index (0 = oldest, count-1 = newest)

**Returns:** Pointer to point, nullptr if index out of range

### `void clear()`

**Description:**

Clear all points from buffer

### `uint8_t count() const`

**Description:**

Get current count of points in buffer

**Returns:** Number of points

### `bool isEmpty() const`

**Description:**

Check if buffer is empty

**Returns:** true if no points

### `bool isFull() const`

**Description:**

Check if buffer is full

**Returns:** true if no more points can be added

### `constexpr uint8_t capacity() const`

**Description:**

Get capacity of buffer

**Returns:** Maximum number of points
