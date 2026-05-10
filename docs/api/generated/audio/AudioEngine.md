# AudioEngine

<Badge type="info" text="Class" />

**Source:** `AudioEngine.h`

## Description

Facade class for the NES-style audio subsystem.

Provides a high-level API for playing sound events, controlling volume,
and managing music playback. Internally delegates to an AudioScheduler
(typically DefaultAudioScheduler) which owns the ApuCore for synthesis.

Usage:
1. Construct with an AudioConfig and PlatformCapabilities.
2. Call init() to set up the scheduler and backend.
3. Call playEvent() to trigger one-shot sounds.
4. Use MusicPlayer for music track sequencing.

## Methods

### `void init()`

**Description:**

Initializes the engine and its internal scheduler.

### `void generateSamples(int16_t* stream, int length)`

**Description:**

Generates audio samples into the output buffer.

**Parameters:**

- `stream`: Output buffer (mono, int16 samples).
- `length`: Number of samples to generate.

### `void playEvent(const AudioEvent& event)`

**Description:**

Triggers a one-shot sound event.

**Parameters:**

- `event`: The event to play (type, frequency, duration, volume).

### `void setMasterVolume(float volume)`

**Description:**

Sets the master volume for all audio output.

**Parameters:**

- `volume`: Volume level [0.0 = silent, 1.0 = full].

### `float getMasterVolume() const`

**Description:**

Gets the current master volume. @return Volume [0.0 - 1.0].

**Returns:** Volume [0.0 - 1.0].

### `void setMasterBitcrush(uint8_t bits)`

**Description:**

Sets the master bitcrusher effect on the final output.

**Parameters:**

- `bits`: Bit depth reduction [0 = off, 1-15 = re-quantize to N bits].

### `uint8_t getMasterBitcrush() const`

**Description:**

Gets the current master bitcrush setting. @return Bit depth (0 = off).

**Returns:** Bit depth (0 = off).

### `void submitCommand(const AudioCommand& cmd)`

**Description:**

Submits a raw audio command to the scheduler.

**Parameters:**

- `cmd`: The command to execute.

### `bool isMusicPlaying() const`

**Description:**

Reports the real-time music transport state from the
       underlying scheduler/ApuCore (not a cached flag).

### `bool isMusicPaused() const`

### `void setScheduler(std::unique_ptr<AudioScheduler> scheduler)`

**Description:**

Replaces the scheduler with a custom implementation.

**Parameters:**

- `scheduler`: Unique pointer to the new scheduler. Takes ownership.

### `AudioScheduler* getScheduler() const`

**Description:**

Gets the current scheduler for diagnostics or profiling.

**Returns:** Pointer to the current AudioScheduler, or nullptr if not set.
