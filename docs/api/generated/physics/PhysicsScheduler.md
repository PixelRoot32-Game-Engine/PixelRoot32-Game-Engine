# PhysicsScheduler

<Badge type="info" text="Class" />

**Source:** `PhysicsScheduler.h`

## Description

Fixed-timestep accumulator that decouples physics from frame rate.

Banks real elapsed time and spends it in fixed 60 Hz slices, so collision
results stay identical regardless of how fast or unevenly frames arrive.

Normally at most one step runs per frame. When the accumulator falls more
than 2.5 frames behind, up to MAX_STEPS_BACKLOG steps run to catch up; that
ceiling is what stops a long stall from cascading into a spiral of death.
Time beyond the ceiling is kept in the accumulator, not discarded.

Zero-heap and zero-allocation: the whole state is an accumulator and a step
counter.

## Methods

### `inline void init()`

### `inline uint8_t update(uint32_t realDeltaMicros, CollisionSystem& collisionSystem)`

### `uint8_t getStepsExecuted() const`

### `uint32_t getAccumulator() const`
