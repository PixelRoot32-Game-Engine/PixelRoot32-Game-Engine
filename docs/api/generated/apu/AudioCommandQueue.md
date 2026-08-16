# AudioCommandQueue

<Badge type="info" text="Class" />

**Source:** `AudioCommandQueue.h`

## Description

Single-Producer Single-Consumer (SPSC) lock-free ring buffer for AudioCommands.

Fixed-size, zero-allocation queue designed for real-time audio thread communication.
Supports one producer (game/logic thread) and a single consumer (the audio thread).
Concurrent multi-producer use is not supported by this algorithm.

Drop policy: When the queue is full, the newest command is silently dropped and
the droppedCommands counter is incremented. Callers can monitor this via
getDroppedCommands() for diagnostics.

Thread-safety: Atomic head/tail loads and stores for SPSC handoff.
Safe for one producer and one consumer; not wait-free under contention from
multiple producers.

## Methods

### `bool enqueue(const AudioCommand& cmd)`

**Description:**

Enqueues a command. Safe for a single producer thread.

**Parameters:**

- `cmd`: The command to enqueue.

**Returns:** true if successful, false if the queue is full (dropped).

### `bool dequeue(AudioCommand& outCmd)`

**Description:**

Dequeues a command. Called from the consumer (Audio Thread).

**Parameters:**

- `outCmd`: Reference to store the dequeued command.

**Returns:** true if a command was dequeued, false if the queue is empty.

### `bool isEmpty() const`

**Description:**

Checks if the queue is empty.

### `size_t getDroppedCommands() const`

**Description:**

Returns the count of dropped commands due to queue full.
Safe to read from the producer or consumer thread.
