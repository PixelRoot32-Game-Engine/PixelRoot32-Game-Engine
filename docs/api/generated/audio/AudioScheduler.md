# AudioScheduler

<Badge type="info" text="Class" />

**Source:** `AudioScheduler.h`

## Description

Abstract interface for the audio execution context.

The scheduler is responsible for owning the audio state, processing commands,
and generating samples. It can run in the same thread as the game loop
or in a dedicated audio thread.

## Methods

### `virtual void init(AudioBackend* backend, int sampleRate, const pixelroot32::platforms::PlatformCapabilities& caps = pixelroot32::platforms::PlatformCapabilities(), int blockSize = 256)`

**Description:**

Initializes the scheduler.

**Parameters:**

- `backend`: The audio backend to use for output.
- `sampleRate`: The output sample rate in Hz.
- `caps`: Platform capabilities to guide core pinning or threading decisions.
- `blockSize`: Audio block size in samples for I2S DMA and ring buffer operations.
       Must be a multiple of 128.

### `virtual void submitCommand(const AudioCommand& cmd)`

**Description:**

Submits a command to the scheduler for execution.

**Parameters:**

- `cmd`: The command to enqueue (PLAY_EVENT, STOP_CHANNEL, etc.).

### `virtual void start()`

**Description:**

Starts the scheduler execution. Enables audio generation.

### `virtual void stop()`

**Description:**

Stops the scheduler execution. Silences all voices.

### `virtual bool isIndependent() const`

**Description:**

Checks if the scheduler runs in an independent thread.

**Returns:** true if the scheduler owns a dedicated audio thread, false if it
       runs synchronously with the backend's callback.

### `virtual void generateSamples(int16_t* stream, int length)`

**Description:**

Generates samples into the provided buffer.

**Parameters:**

- `stream`: Pointer to the output buffer (mono, int16 samples).
- `length`: Number of samples to generate (must match blockSize).

Called by the backend (or scheduler thread) to fill the audio buffer.

### `virtual bool isMusicPlaying() const`

**Description:**

Reports whether a music track is currently being sequenced.

### `virtual bool isMusicPaused() const`

**Description:**

Reports whether the music sequencer is paused.

### `virtual ApuCore& getApuCore()`

**Description:**

Gets reference to the underlying ApuCore for diagnostics/profiling.
