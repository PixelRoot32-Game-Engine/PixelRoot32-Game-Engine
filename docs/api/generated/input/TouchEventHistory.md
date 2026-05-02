# TouchEventHistory

<Badge type="info" text="Struct" />

**Source:** `TouchTypes.h`

## Description

Ring buffer for touch events (for gesture detection)

Use TouchEventQueue from TouchEventQueue.h instead

## Methods

### `void push(const TouchEvent& event)`

**Description:**

Add event to history

### `void clear()`

**Description:**

Clear history

### `const TouchEvent* mostRecent() const`

**Description:**

Get most recent event
