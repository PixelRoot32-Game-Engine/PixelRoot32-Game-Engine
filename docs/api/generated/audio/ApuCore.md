# ApuCore

<Badge type="info" text="Class" />

**Source:** `ApuCore.h`

## Description

Shared NES-style APU core used by every AudioScheduler.

Owns the 4 channels (2x PULSE, 1x TRIANGLE, 1x NOISE), the SPSC command
queue and the music sequencer. Platform-specific schedulers
(DefaultAudioScheduler, ESP32AudioScheduler, NativeAudioScheduler) are
thin orchestrators that decide *when* generateSamples() runs; all
synthesis, mixing and sequencing lives here to eliminate the three-way
duplication that existed before.

Mixing curve:
  per channel: s_c = wave_c(phase) * volume_c * MIXER_SCALE
  sum:         S   = Σ s_c                         (bounded to [-1.6, 1.6])
  compressor:  y   = S / (1 + |S| * MIXER_K)
  output:      y * masterVolume * 32767, passed through a single-pole
               DC-blocker to remove offset + transient clicks.

On cores without an FPU (ESP32-C3) the integer-optimised path uses
`audio_mixer_lut` which is pre-fitted to the same curve.

## Properties

| Name | Type | Description |
|------|------|-------------|
| `audioTimeSamples` | `uint64_t` | Global sample counter at capture. |
| `peak` | `float` | Peak sample magnitude [0.0 - 1.0]. |
| `clipped` | `bool` | Whether any sample exceeded ±32767. |

## Methods

### `void init(int sampleRate)`

**Description:**

Configures the output sample rate.

**Parameters:**

- `sampleRate`: Sample rate in Hz (e.g., 22050, 44100).

Safe to call before start(). Pre-calculates internal timing values.

### `bool submitCommand(const AudioCommand& cmd)`

**Description:**

Enqueues a command for processing.

**Parameters:**

- `cmd`: The audio command to enqueue.

**Returns:** true if the command was enqueued, false if the queue was full.

### `void generateSamples(int16_t* stream, int length)`

**Description:**

Generates audio samples.

**Parameters:**

- `stream`: Output buffer (mono, int16 samples).
- `length`: Number of samples to generate.

Processes all pending commands, updates music sequencer, and synthesizes
voices into the output buffer. Applies master volume, bitcrush, and DC blocking.

### `uint32_t getDroppedCommands() const`

**Description:**

Returns the total number of commands dropped since construction. @return Monotonic count.

**Returns:** Monotonic count.

### `void setSequencerNoteLimit(size_t limit)`

**Description:**

Sets the maximum notes processed per audio frame.

**Parameters:**

- `limit`: Max notes [1-1000], clamped to safe bounds internally.

Bounded processing prevents audio starvation when many notes queue up.

### `size_t getSequencerNoteLimit() const`

**Description:**

Gets current max notes per frame setting. @return Current limit.

**Returns:** Current limit.

### `size_t getDeferredNotes() const`

**Description:**

Returns notes deferred to next frame due to note limit. @return Deferred count.

**Returns:** Deferred count.

### `bool isMusicPlaying() const`

**Description:**

Reports if music is currently playing. @return true if playing.

**Returns:** true if playing.

### `bool isMusicPaused() const`

**Description:**

Reports if music is paused. @return true if paused.

**Returns:** true if paused.

### `int getSampleRate() const`

**Description:**

Gets the configured sample rate. @return Sample rate in Hz.

**Returns:** Sample rate in Hz.

### `void reset()`

**Description:**

Resets all state to initial values.

### `void setPostMixMono(void (*fn)(int16_t* mono, int length, void* user), void* user)`

**Description:**

Sets an optional post-mix callback.

**Parameters:**

- `fn`: Function pointer: void(int16_t* mono, int length, void* user).
- `user`: User data passed to the callback.

Called after bitcrush on the final mono buffer. Runs in audio thread context.

### `void getAndResetProfileStats(ProfileEntry* out, uint8_t& count)`

**Description:**

Reads and clears all profile entries.

**Parameters:**

- `out`: Array of ProfileEntry to fill.
- `count`: On input: max entries. On output: actual count written.

### `size_t countEnabledVoicesForTesting() const`

### `size_t getSequencerMainNoteIndexForTesting() const`
