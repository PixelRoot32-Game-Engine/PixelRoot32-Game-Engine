# AudioEngine

<Badge type="info" text="Class" />

**Source:** `AudioEngine.h`

## Description

Core class for the NES-like audio subsystem.

## Methods

### `void init()`

### `void generateSamples(int16_t* stream, int length)`

### `void playEvent(const AudioEvent& event)`

### `void setMasterVolume(float volume)`

### `float getMasterVolume() const`

### `void setMasterBitcrush(uint8_t bits)`

### `uint8_t getMasterBitcrush() const`

### `void submitCommand(const AudioCommand& cmd)`

### `bool isMusicPlaying() const`

**Description:**

Reports the real-time music transport state from the
       underlying scheduler/ApuCore (not a cached flag).

### `bool isMusicPaused() const`

### `AudioScheduler* getScheduler() const`

**Description:**

Gets the underlying scheduler for diagnostics/profiling.
