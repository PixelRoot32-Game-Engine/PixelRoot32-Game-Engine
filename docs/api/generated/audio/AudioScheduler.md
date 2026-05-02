# AudioScheduler

<Badge type="info" text="Class" />

**Source:** `AudioScheduler.h`

## Description

Abstract interface for the audio execution context.

The scheduler is responsible for owning the audio state, processing commands,
and generating samples. It can run in the same thread as the game loop
or in a dedicated audio thread.

## Methods

### `virtual void submitCommand(const AudioCommand& cmd)`

**Description:**

Submits a command to the scheduler.

**Parameters:**

- `cmd`: The command to execute.

### `virtual void start()`

**Description:**

Starts the scheduler execution.

### `virtual void stop()`

**Description:**

Stops the scheduler execution.

### `virtual bool isIndependent() const`

**Description:**

Checks if the scheduler runs in an independent thread.

### `virtual void generateSamples(int16_t* stream, int length)`

**Description:**

Generates samples. Should be called by the backend or scheduler thread.

### `virtual bool isMusicPlaying() const`

**Description:**

Reports whether a music track is currently being sequenced.

### `virtual bool isMusicPaused() const`

**Description:**

Reports whether the music sequencer is paused.

### `virtual ApuCore& getApuCore()`

**Description:**

Gets reference to the underlying ApuCore for diagnostics/profiling.
