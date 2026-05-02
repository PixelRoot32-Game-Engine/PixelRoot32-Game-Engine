# TouchEventQueue

<Badge type="info" text="Class" />

**Source:** `TouchEventQueue.h`

## Description

Ring buffer for touch events (192 bytes total)

Fixed-size circular buffer with O(1) enqueue/dequeue operations.
Uses a static array - no dynamic memory allocation.

Memory layout:
- events[16]: 16 * 12 = 192 bytes
- head: 1 byte
- tail: 1 byte
- count: 1 byte
Total: ~195 bytes (with padding)

## Methods

### `bool isEmpty() const`

**Description:**

Check if queue is empty

**Returns:** true if no events in queue

### `bool isFull() const`

**Description:**

Check if queue is full

**Returns:** true if queue cannot accept more events

### `uint8_t getCount() const`

**Description:**

Get number of events in queue

**Returns:** Number of events currently queued

### `constexpr uint8_t getCapacity() const`

**Description:**

Get capacity of queue

**Returns:** Maximum number of events (16)

### `bool enqueue(const TouchEvent& event)`

**Description:**

Enqueue an event (add to tail)

**Parameters:**

- `event`: Event to add

**Returns:** true if event was enqueued, false if queue was full

### `bool dequeue(TouchEvent& event)`

**Description:**

Dequeue an event (remove from head)

**Parameters:**

- `event`: Output parameter for dequeued event

**Returns:** true if event was dequeued, false if queue was empty

### `bool peek(TouchEvent& event) const`

**Description:**

Peek at head event without removing

**Parameters:**

- `event`: Output parameter for peeked event

**Returns:** true if event exists, false if queue empty

### `uint8_t peekMultiple(TouchEvent* events, uint8_t maxCount) const`

**Description:**

Peek at multiple events from head

**Parameters:**

- `events`: Output buffer for events
- `maxCount`: Maximum number of events to peek

**Returns:** Number of events peeked

### `void clear()`

**Description:**

Clear all events from queue

### `uint8_t drop(uint8_t count)`

**Description:**

Remove and discard n events from head

**Parameters:**

- `count`: Number of events to drop

**Returns:** Number of events actually dropped

### `uint8_t getEvents(TouchEvent* events, uint8_t maxCount)`

**Description:**

Get events by providing caller-owned buffer

**Parameters:**

- `events`: Caller-provided buffer for events
- `maxCount`: Maximum events to retrieve

**Returns:** Number of events retrieved

This is the pull-based API: consumer provides the buffer.

### `bool hasEvents() const`

**Description:**

Check if events are available (hasEvents)

**Returns:** true if queue has events
