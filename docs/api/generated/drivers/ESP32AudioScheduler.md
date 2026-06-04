# ESP32AudioScheduler

<Badge type="info" text="Class" />

**Source:** `ESP32AudioScheduler.h`

**Inherits from:** [AudioScheduler](../audio/AudioScheduler.md)

## Description

Audio scheduler for ESP32 targets.

The I2S/DAC backend creates the FreeRTOS task and calls generateSamples();
this class does not spawn its own task. All synthesis is delegated to
ApuCore so ESP32-classic / S3 / C3 share the exact same logic.
Constructor arguments are reserved for API stability.

## Inheritance

[AudioScheduler](../audio/AudioScheduler.md) → `ESP32AudioScheduler`

## Methods

### `void init(AudioBackend* backend, int sampleRate, const pixelroot32::platforms::PlatformCapabilities& caps, int blockSize = 256)`

### `void submitCommand(const AudioCommand& cmd)`

### `void start()`

### `void stop()`

### `bool isIndependent() const`

### `void generateSamples(int16_t* stream, int length)`

### `bool isMusicPlaying() const`

### `bool isMusicPaused() const`

### `ApuCore& getApuCore()`

### `const ApuCore& core() const`

### `ApuCore& core()`
