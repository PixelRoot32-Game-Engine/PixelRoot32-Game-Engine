# NativeAudioScheduler

<Badge type="info" text="Class" />

**Source:** `NativeAudioScheduler.h`

**Inherits from:** [AudioScheduler](../audio/AudioScheduler.md)

## Description

Audio scheduler for native builds.

Runs ApuCore in its own std::thread and double-buffers samples through
a lock-free ring, mirroring the dual-core ESP32 behaviour. All
synthesis / sequencer logic lives in ApuCore; this class owns only
threading and the ring buffer.

The render block is configuration (S12): it is read once in init() from
the caller and rendered in a single `generateSamples` call into a member
buffer of `PR32_AUDIO_MAX_BLOCK_SAMPLES`, so the output is exactly what
one block-sized call per block produces.

## Inheritance

[AudioScheduler](../audio/AudioScheduler.md) → `NativeAudioScheduler`

## Methods

### `explicit NativeAudioScheduler(size_t ringBufferSize = 4096)`

### `void init(AudioBackend* backend, int sampleRate, const pixelroot32::platforms::PlatformCapabilities& caps, int blockSize = 512)`

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
