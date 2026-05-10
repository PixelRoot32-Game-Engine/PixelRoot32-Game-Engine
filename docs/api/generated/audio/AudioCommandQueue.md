# AudioCommandQueue

<Badge type="info" text="Class" />

**Source:** `AudioCommandQueue.h`

## Description

Multi-Producer Single-Consumer (MPSC) lock-free ring buffer for AudioCommands.

Fixed-size, zero-allocation queue designed for real-time audio thread communication.
Supports multiple concurrent producer threads (e.g., game logic, music sequencer)
and a single consumer thread (the audio thread).

Drop policy: When the queue is full, the newest command is silently dropped and
the droppedCommands counter is incremented. Callers can monitor this via
getDroppedCommands() for diagnostics.

Thread-safety: Uses compare-and-swap (CAS) for atomic ring index advancement.
The producer path is wait-free; the consumer path is lock-free.

## Methods

### `bool enqueue(const AudioCommand& cmd)`

**Description:**

Enqueues a command. Thread-safe for multiple producers.

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
Thread-safe for concurrent reads from multiple producers.
