# ActorTouchController

<Badge type="info" text="Class" />

**Source:** `ActorTouchController.h`

## Description

Handles touch-based dragging of game actors

This controller manages a pool of actors that can be dragged via touch input.
It implements:
- Drag threshold (5 pixels) to ignore jitter
- Offset preservation (actor moves relative to initial touch position)
- Single drag (only one actor dragged at a time)
- Fixed pool (no dynamic memory allocation)

Usage:
ActorTouchController controller;
controller.registerActor(&myActor);
// In game loop:
TouchEvent events[16];
uint8_t count = dispatcher.getEvents(events, 16);
for (uint8_t i = 0; i < count; i++) {
    controller.handleTouch(events[i]);
}
@endcode

## Methods

### `void reset()`

**Description:**

Clear registered actors and drag state (e.g. scene reset / arena recycle).

### `void setTouchHitSlop(int16_t expandPixels)`

**Description:**

Expand hit-test rectangles by this many pixels on each side (0 = exact hitbox only).
       Useful when calibrated screen coords lag the visual sprite on resistive panels.

### `int16_t getTouchHitSlop() const`

**Description:**

Current hit slop in pixels (per side).

### `void handleTouch(const TouchEvent& event)`

**Description:**

Handle a touch event

**Parameters:**

- `event`: The touch event to process

Routes events based on type:
- TouchDown: Check for hit, begin drag if threshold exceeded
- DragMove: Update dragged actor position
- TouchUp: End drag

### `bool isDragging() const`

**Description:**

Check if currently dragging an actor

**Returns:** true if a drag operation is in progress

### `void onTouchDown(const TouchEvent& event)`

**Description:**

Handle touch down event

**Parameters:**

- `event`: The touch down event

### `void onTouchMove(const TouchEvent& event)`

**Description:**

Handle drag move event

**Parameters:**

- `event`: The drag move event

### `void onTouchUp(const TouchEvent& event)`

**Description:**

Handle touch up event

**Parameters:**

- `event`: The touch up event

### `void onDragStart(const TouchEvent& event)`

**Description:**

Handle drag start (movement exceeded threshold after TouchDown).
       If TouchDown missed the actor but the finger is now on one, start dragging.
