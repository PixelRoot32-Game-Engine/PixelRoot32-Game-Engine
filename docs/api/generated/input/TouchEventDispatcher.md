# TouchEventDispatcher

<Badge type="info" text="Class" />

**Source:** `TouchEventDispatcher.h`

## Description

Pull-based touch event dispatcher

This is the main API for consumers to receive touch events.
It combines the state machine and event queue into a unified interface.

Usage pattern (pull-based):
TouchEvent events[16];
uint8_t count = dispatcher.getEvents(events, 16);
for (uint8_t i = 0; i < count; i++) {
    handleEvent(events[i]);
}
Or for checking without consuming:
if (dispatcher.hasEvents()) {
    TouchEvent event;
    dispatcher.peek(event);
    // inspect without removing
}
@endcode

## Methods

### `uint8_t getEvents(TouchEvent* events, uint8_t maxCount)`

**Description:**

Get events using caller-provided buffer (pull-based)

**Parameters:**

- `events`: Caller-provided buffer for events
- `maxCount`: Maximum events to retrieve

**Returns:** Number of events retrieved (removed from queue)

This is the primary API for consumers.
Events are removed from the internal queue.

### `uint8_t peekEvents(TouchEvent* events, uint8_t maxCount) const`

**Description:**

Peek at events without removing them

**Parameters:**

- `events`: Caller-provided buffer
- `maxCount`: Maximum events to peek

**Returns:** Number of events peeked

### `bool hasEvents() const`

**Description:**

Check if events are available

**Returns:** true if queue has events

### `uint8_t getEventCount() const`

**Description:**

Get number of pending events

**Returns:** Number of events in queue

### `void clearEvents()`

**Description:**

Clear all pending events

### `void reset()`

**Description:**

Reset state machine (force all touches to idle)

### `TouchState getTouchState(uint8_t touchId) const`

**Description:**

Get current state for a touch ID

**Parameters:**

- `touchId`: Touch identifier

**Returns:** Current state

### `bool isTouchActive() const`

**Description:**

Check if any touch is active

**Returns:** true if any touch is in progress
