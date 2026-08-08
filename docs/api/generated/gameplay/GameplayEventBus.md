# GameplayEventBus

<Badge type="info" text="Class" />

**Source:** `GameplayEventBus.h`

## Description

Fixed-capacity FIFO ring buffer for GameplayEvent, single-instance and Engine-owned.

Sized by `GAMEPLAY_EVENT_QUEUE_CAPACITY` (mirror:
`pixelroot32::platforms::config::GameplayEventQueueCapacity`). Overflow
policy is drop-newest: when the buffer is full, `publish()` discards the
incoming event and increments a monotonic drop counter rather than evicting
an already-buffered event — this preserves paired TriggerEnter/TriggerExit
ordering (see design.md D2).

**Non-atomic by design.** Unlike AudioCommandQueue (which bridges the game
thread and the audio task with std::atomic), this bus is produced and
consumed entirely inside the single-threaded Scene::update()/Scene::draw()
loop driven from SceneManager::update(). Plain, non-atomic `uint16_t`
head/tail/count indices. Publishing from an ISR or the audio task is
explicitly unsupported.

The bus is drained (cleared) by SceneManager on every SceneSwap. It is
NOT drained on pushScene()/popScene().

## Methods

### `bool publish(const GameplayEvent& event)`

**Description:**

Publishes an event to the bus.

**Parameters:**

- `event`: The event to enqueue (copied into the ring).

**Returns:** true if the event was stored, false if the buffer was full
        (the event is dropped and the drop counter is incremented).

Never blocks, never allocates.

### `bool consume(GameplayEvent& outEvent)`

**Description:**

Consumes (pops) the oldest pending event, FIFO order.

**Parameters:**

- `outEvent`: Reference to receive the dequeued event.

**Returns:** true if an event was retrieved, false if the buffer was empty.

### `void clear()`

**Description:**

Clears all pending events without changing the drop counter.

### `uint32_t getDroppedCount() const`

**Description:**

Returns the number of events dropped due to a full buffer.

### `uint16_t size() const`

**Description:**

Number of events currently pending.

### `bool isEmpty() const`

**Description:**

True if no events are pending.

### `bool isFull() const`

**Description:**

True if the buffer is at capacity.
