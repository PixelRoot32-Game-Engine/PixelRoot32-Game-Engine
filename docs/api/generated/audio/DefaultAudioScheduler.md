# DefaultAudioScheduler

<Badge type="info" text="Class" />

**Source:** `DefaultAudioScheduler.h`

**Inherits from:** [AudioScheduler](./AudioScheduler.md)

## Description

Backend-driven scheduler used on platforms without a dedicated audio task.

Delegates all synthesis to ApuCore; generateSamples() runs in whichever
context the backend invokes it (tests, simulators without a thread, etc.).
Does not own a thread — audio generation is driven by the backend's callback.

For platforms with a dedicated audio task (e.g., FreeRTOS on ESP32),
      use a scheduler that spawns its own thread instead.

## Inheritance

[AudioScheduler](./AudioScheduler.md) → `DefaultAudioScheduler`

::: tip
For platforms with a dedicated audio task (e.g., FreeRTOS on ESP32),
      use a scheduler that spawns its own thread instead.
:::

## Methods

### `void init(AudioBackend* backend, int sampleRate, const pixelroot32::platforms::PlatformCapabilities& caps, int blockSize = 256)`

### `void submitCommand(const AudioCommand& cmd)`

**Description:**

Enqueues a command to the ApuCore. @param cmd The command to submit.

**Parameters:**

- `cmd`: The command to submit.

### `void start()`

**Description:**

Marks scheduler as running. Starts audio generation context.

### `void stop()`

**Description:**

Marks scheduler as stopped. Silences all voices.

### `bool isIndependent() const`

**Description:**

Returns false (no dedicated audio thread). @return false.

**Returns:** false.

### `void generateSamples(int16_t* stream, int length)`

**Description:**

Generates samples via ApuCore. @param stream Output buffer. @param length Sample count.

**Parameters:**

- `stream`: Output buffer.

### `bool isMusicPlaying() const`

### `bool isMusicPaused() const`

### `ApuCore& getApuCore()`

**Description:**

Returns reference to the ApuCore for diagnostics. @return ApuCore reference.

**Returns:** ApuCore reference.

### `const ApuCore& core() const`

**Description:**

Exposes the underlying core for tests or higher-level queries. @return Const ApuCore reference.

**Returns:** Const ApuCore reference.

### `ApuCore& core()`

**Description:**

Exposes the underlying core for tests or higher-level queries. @return ApuCore reference.

**Returns:** ApuCore reference.
