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

### `bool registerActor(pixelroot32::core::Actor* actor)`

**Description:**

Register an actor to the drag pool

**Parameters:**

- `actor`: Pointer to the actor to register

**Returns:** true if registration succeeded, false if pool is full

Note: Does not check for duplicates - caller should ensure
the actor is not already registered.

### `void setTouchHitSlop(int16_t expandPixels)`

**Description:**

Expand hit-test rectangles by this many pixels on each side (0 = exact hitbox only).
       Useful when calibrated screen coords lag the visual sprite on resistive panels.

### `int16_t getTouchHitSlop() const`

**Description:**

Current hit slop in pixels (per side).

### `bool unregisterActor(pixelroot32::core::Actor* actor)`

**Description:**

Unregister an actor from the drag pool

**Parameters:**

- `actor`: Pointer to the actor to unregister

**Returns:** true if actor was found and removed, false if not found

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

### `pixelroot32::core::Actor* getDraggedActor() const`

**Description:**

Get the currently dragged actor

**Returns:** Pointer to the dragged actor, nullptr if not dragging

### `pixelroot32::core::Actor* hitTest(int16_t x, int16_t y)`

**Description:**

Perform hit test to find actor at touch position

**Parameters:**

- `x`: X coordinate
- `y`: Y coordinate

**Returns:** Pointer to hit actor, nullptr if none hit

### `bool pointInRect(int16_t px, int16_t py, const pixelroot32::core::Rect& rect, int16_t slop = 0)`

**Description:**

Check if a point is inside a rectangle, optionally expanded by @a slop pixels per side.

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
