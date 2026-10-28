# ESP32_I2S_AudioBackend

<Badge type="info" text="Class" />

**Source:** `ESP32_I2S_AudioBackend.h`

**Inherits from:** [AudioBackend](../audio/AudioBackend.md)

## Description

Audio backend implementation for ESP32 using I2S.

Uses a FreeRTOS task to continuously feed the I2S DMA buffer
to ensure smooth playback independent of the game loop frame rate.

The render block is configuration, not a backend constant (S12): it is
read once in init() from AudioEngine::getBlockSize() and drives the DMA
depth (`dma_buf_len` / `dma_buf_count`), the per-iteration render length
and the real-time budget of the underrun counter (S13).

## Inheritance

[AudioBackend](../audio/AudioBackend.md) → `ESP32_I2S_AudioBackend`

## Methods

### `void init(pixelroot32::audio::AudioEngine* engine, const pixelroot32::platforms::PlatformCapabilities& caps)`

### `int getSampleRate() const`

### `void audioTaskLoop()`

### `uint32_t getUnderrunCount() const`

**Description:**

Blocks where `i2s_write` had almost nothing to wait for
       (less than a tenth of the block budget, so the DMA ring had
       already drained), or where the write failed or came back short.

### `uint32_t getWorstBlockUs() const`

**Description:**

Worst total iteration (render + write) duration, in µs.

### `uint32_t getWorstWriteUs() const`

**Description:**

Worst `i2s_write` blocking time seen, in µs.

### `uint32_t getWorstRenderUs() const`

**Description:**

Worst synthesis (render) time seen, in µs.

### `void accountBlockTiming(int64_t iterationUs, int64_t renderUs, int64_t writeUs, int64_t budgetUs, bool writeComplete, bool countUnderruns)`

**Description:**

Accounts one iteration: updates the worst cases and, once the
       ring is primed, the underrun count.
