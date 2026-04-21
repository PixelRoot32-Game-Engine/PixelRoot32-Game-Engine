# Delta Spec: Audio Subsystem Fixes

## Change: audio-subsystem-fixes

This delta spec covers three performance and thread-safety fixes for the PixelRoot32 audio subsystem.

---

## Domain: Audio

### ADDED Requirements

### Requirement: Multi-Producer Safe Command Queue

The audio system SHALL support concurrent command submission from multiple producer threads without external synchronization.

**Rationale**: The existing SPSC queue only guarantees safety for one producer. Games may call `AudioEngine::playEvent()` from multiple threads (e.g., game logic + network callbacks).

#### Scenario: Single Producer Baseline

- GIVEN an AudioCommandQueue with single producer thread
- WHEN the producer enqueues 64 AudioCommands
- THEN all 64 commands MUST be successfully enqueued
- AND subsequent dequeue MUST return those 64 commands in FIFO order

#### Scenario: Multiple Producers Concurrent

- GIVEN an AudioEngine with 3 producer threads calling playEvent() concurrently
- WHEN all 3 producers enqueue 32 commands each (96 total) within 10ms window
- THEN no command results in undefined behavior (no corruption, no crash)
- AND all 96 commands eventually reach the audio consumer in some valid order

#### Scenario: Queue Full Under Contention

- GIVEN an AudioCommandQueue at 95% capacity
- WHEN a fourth producer thread attempts to enqueue when full
- THEN the enqueue call MUST return false (drop policy)
- AND the droppedCommands counter MUST be incremented atomically

#### Scenario: Thread-Safety Verification

- GIVEN concurrent enqueue operations from N threads (N >= 2)
- WHEN running 10000 iterations with thread sanitizer or data-race detector
- THEN zero data races MUST be reported by the tool

---

### Requirement: Music Sequencer Bounded Processing

The music sequencer SHALL process a maximum number of notes per audio frame to prevent unbounded catch-up loops.

**Rationale**: If the audio thread is blocked (e.g., due to system load), the sequencer may attempt to emit thousands of pending notes in one generateSamples() call, causing audio glitching.

#### Scenario: Normal Processing

- GIVEN a MusicTrack with 100 notes queued and sequencer note limit of 32
- WHEN one audio frame processes 32 notes
- THEN exactly 32 notes SHOULD be processed
- AND the remaining 68 notes SHOULD be deferred to subsequent frames

#### Scenario: Limit Hit Mid-Frame

- GIVEN 80 pending notes and a limit of 32 per frame
- WHEN the limit is reached during playback
- THEN processing MUST stop at 32 notes
- AND the sequencer MUST resume from note 33 on the next generateSamples() call

#### Scenario: Catch-Up Prevention Verified

- GIVEN an audio thread starved for 500ms (100+ frames missed)
- WHEN the starved frames are processed sequentially
- THEN audio output MUST remain stable (no burst of notes)
- AND no frame SHOULD exceed the note limit

#### Scenario: Delay Notification

- GIVEN note limit reached during playback
- WHEN the sequencer defers notes
- THEN the system SHOULD provide diagnostic output indicating deferral
- AND the audio timeline SHOULD reflect the actual processed notes, not attempted notes

---

### Requirement: Mixing Loop Performance

The integer mixing path SHALL avoid expensive control flow inside the per-sample inner loop.

**Rationale**: The `switch(ch.type)` inside the inner loop for integer path creates branch misprediction overhead at 22kHz+ per channel.

#### Scenario: Integer Path Branch-Free

- GIVEN ESP32-C3 build with integer path enabled
- WHEN generateSamples() processes 512 samples
- THEN the inner channel loop MUST NOT contain a switch statement per sample
- AND the same oscillator algorithm MUST be applied uniformly

#### Scenario: FPU Path Unchanged

- GIVEN ESP32-S3 build with FPU path enabled
- WHEN generateSamples() processes 512 samples
- THEN the output audio MUST be unchanged from baseline
- AND cross-platform compatibility MUST be maintained

---

### MODIFIED Requirements

### Requirement: Audio Command Queue SPSC to MPSC

The AudioCommandQueue class from the existing spec is MODIFIED to support multiple producers.

(Previously: Single-producer single-consumer lock-free ring buffer)

**Updated Requirement**: The AudioCommandQueue class MUST support multiple concurrent producers and a single consumer. The implementation MUST use lock-free atomic operations. Under queue fullness, the newest command MUST be dropped while preserving progress for the single consumer.

#### Scenario: Producer Thread Count

- GIVEN 1, 2, 4, and 8 concurrent producer threads
- WHEN each thread enqueues 1000 commands
- THEN all threads MUST complete without deadlock
- AND no producer thread should block indefinitely

#### Scenario: Capacity Increase

- GIVEN the default queue capacity of 128
- WHEN extended use cases require higher throughput
- THEN the capacity constant MUST be configurable at compile time
- AND the maximum capacity MUST NOT exceed 1024 (memory constraint)

---

### Requirement: AudioDroppedCommands Atomic

The droppedCommands counter from the existing spec is MODIFIED to track MPSC drops accurately.

(Previously: Single-producer counter)

**Updated Requirement**: The droppedCommands counter MUST be updated atomically under MPSC contention. The counter value MUST reflect the total number of commands dropped since initialization, accounting for all producers.

#### Scenario: Accurate Drop Counting

- GIVEN 4 producer threads each dropping 10 commands due to queue full
- WHEN the counter is queried via getDroppedCommands()
- THEN the returned value MUST be exactly 40 (or higher if additional drops occurred)

---

### Requirement: Music Sequencer Update Behavior

The updateMusicSequencer() function from the existing spec is MODIFIED to include per-frame limits.

(Previously: Unbounded catch-up — processes all pending notes per frame)

**Updated Requirement**: The updateMusicSequencer() function MUST process at most MAX_NOTES_PER_FRAME notes per generateSamples() call. When the limit is reached, pending notes MUST be deferred to subsequent frames with appropriate timing adjustment.

#### Scenario: Frame Budget Enforcement

- GIVEN MAX_NOTES_PER_FRAME = 32
- WHEN 64 notes are due in a single frame
- THEN exactly 32 notes SHOULD trigger events
- AND the sequencer timeline MUST advance by only 32 note durations

---

### REMOVED Requirements

### Requirement: [None]

No requirements are being removed. All existing functionality is preserved; new behaviors are additive.

---

## Interface Changes

### New Constants

| Constant | Type | Default | Description |
|----------|------|---------|-------------|
| `MAX_NOTES_PER_FRAME` | size_t | 32 | Max notes processed per audio frame in sequencer |
| `AUDIO_COMMAND_QUEUE_MPSC` | bool | true | Compile-time flag to enable MPSC mode |
| `AUDIO_SEQUENCER_MAX_NOTES` | size_t | 32 | Alias for MAX_NOTES_PER_FRAME |

### Modified APIs

| API | Change | Description |
|-----|--------|-------------|
| `AudioCommandQueue::enqueue()` | Thread safety | Now safe for multiple producers |
| `AudioCommandQueue::CAPACITY` | May be configurable | From 128 to up to 1024 |
| `ApuCore::updateMusicSequencer()` | Adds limit parameter | Maximum notes per call |
| `ApuCore::getDroppedCommands()` | Now accurate for MPSC | Changed from SPSC counter |

### New APIs

| API | Description |
|-----|-------------|
| `ApuCore::setSequencerNoteLimit(size_t)` | Set max notes per frame at runtime |
| `ApuCore::getSequencerNoteLimit()` | Query current limit |
| `ApuCore::getDeferredNotes()` | Get count of notes deferred to next frame |

---

## Non-Functional Requirements

### Real-Time Constraints

- **Audio callback budget**: 2ms for 512 samples @ 22kHz
- **Sequencer overhead**: Max 0.1ms per frame for note limit checks
- **Command queue enqueue**: < 100ns per operation (lock-free)

### Performance Targets

- **MPSC throughput**: 50,000+ commands/second sustained
- **Mixing loop (integer)**: < 20 cycles/sample/channel
- **Sequencer frame time**: < 0.5ms even with 1000 pending notes (limited)

### Thread Safety

- **Producers**: Up to 8 concurrent producer threads
- **Consumer**: Single audio thread (unchanged)
- **Atomic operations**: C11/C++11 atomics with appropriate memory orderings
- **No locks**: All operations lock-free; no mutex in audio hot path

---

## Edge Cases and Error Conditions

### Command Queue

| Condition | Behavior |
|-----------|----------|
| Queue full on enqueue | Drop newest, return false, increment droppedCommands |
| All producers exit | Consumer continues; queue empties gracefully |
| Producer terminates mid-operation | Other producers unaffected; no resource leaks |
| Queue corruption detected | Reset queue; log error; continue operation |

### Sequencer

| Condition | Behavior |
|-----------|----------|
| Limit set to 0 | Process all pending (unbounded — fallback) |
| Limit set to value > 1000 | Clamp to 32 internally |
| Track ends mid-limit | Stop processing; update track state |
| Note index out of bounds | Clamp to track end; stop |

### Mixing

| Condition | Behavior |
|-----------|----------|
| All channels disabled | Output silence (fast path) |
| FPU path unavailable | Fall back to integer path (automatic) |
| Sample rate change | Recalculate tick durations; continue |

---

## Verification Strategy

### Thread Safety Tests

1. **MPSC Stress Test**: 8 producer threads × 10,000 commands each; verify no data races
2. **Lock-Free Verification**: Use ThreadSanitizer; confirm zero races
3. **Dropo Count Accuracy**: Verify droppedCommands reflects all drops

### Performance Tests

1. **Integer Loop Benchmark**: Profile generateSamples() cycles with switch hoisted
2. **MPSC Throughput**: Measure max commands/second with 4+ producers
3. **Sequencer Frame Time**: Measure time for 1000 notes with limit enabled

### Timing Tests

1. **Catch-Up Loop Prevention**: Simulate 100ms audio starvation; verify stable output
2. **Note Deferral Detection**: Log when notes are deferred; verify correct count
3. **Audio Continuity**: Verify no glitches when limit is hit repeatedly

### Integration Tests

1. **Full Audio Pipeline**: playEvent() from multiple threads + generateSamples() + music playback
2. **Platform Parity**: Compare ESP32-C3 (integer) vs ESP32-S3 (FPU) output
3. **Stress Test**: 1 hour continuous playback with concurrent events and music

---

## Summary

| Fix | Requirement Count | Scenario Count | Interface Changes |
|-----|-------------------|----------------|-------------------|
| MPSC Command Queue | 4 | 4 | AudioCommandQueue, droppedCommands |
| Mixing Loop Optimization | 1 | 2 | None (internal optimization) |
| Sequencer Note Limit | 3 | 4 | updateMusicSequencer(), new constants |
| **Total** | **8** | **10** | **4 new/modified APIs** |