# AudioCommandQueue

<Badge type="info" text="Class" />

**Source:** `AudioCommandQueue.h`

## Description

Single-Producer Single-Consumer (SPSC) lock-free ring buffer for AudioCommands.

Fixed-size, zero-allocation queue designed for real-time audio thread communication.
Supports one producer (game/logic thread) and a single consumer (the audio thread).
Concurrent multi-producer use is not supported by this algorithm.

Memory: one slot holds a whole AudioCommand (sizeof(AudioCommand) bytes,
144 B on the host ABI). The ring keeps one slot free to distinguish full
from empty, so a CAPACITY of 64 accepts 63 commands. The accepted range is
32-64, and its ceiling is set by RAM, not by throughput: the real command
rate is tens per second, so the former 128 doubled the dominant cost of
ApuCore without adding headroom anyone used. Raising the ceiling means
raising the 9 KB budget below in the same commit, after measuring the new
slot size: a capacity the budget cannot pay for is not a capacity.

Drop policy: When the queue is full, the newest command is silently dropped and
the droppedCommands counter is incremented. Callers can monitor this via
getDroppedCommands() for diagnostics.

Thread-safety: Atomic head/tail loads and stores for SPSC handoff.
Safe for one producer and one consumer; not wait-free under contention from
multiple producers.

## Properties

| Name | Type | Description |
|------|------|-------------|
| `CAPACITY` | `static constexpr size_t` | Default queue capacity in commands. |

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
