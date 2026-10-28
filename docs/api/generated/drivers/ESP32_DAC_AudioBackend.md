# ESP32_DAC_AudioBackend

<Badge type="info" text="Class" />

**Source:** `ESP32_DAC_AudioBackend.h`

**Inherits from:** [AudioBackend](../audio/AudioBackend.md)

## Description

Audio backend for ESP32 classic / S2 internal 8-bit DAC.

Uses **I2S in DAC-built-in mode** so samples are pushed to the DAC via
DMA instead of the previous per-sample `dacWrite()` spin loop. This
frees ~15-25% of CPU on Core 0 at 22050 Hz and removes the per-sample
pinmux/mutex overhead that `dacWrite()` incurs on every call.

Target: ESP32 (original), ESP32-S2. The DAC does not exist on S3/C3.

Like the I2S backend, the render block is configuration (S12): it is read
once in init() from AudioEngine::getBlockSize() and drives the DMA depth,
the per-iteration render length and the underrun budget (S13).

## Inheritance

[AudioBackend](../audio/AudioBackend.md) → `ESP32_DAC_AudioBackend`

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

Worst total iteration (render + DAC conversion + write)
       duration, in µs.

### `uint32_t getWorstWriteUs() const`

**Description:**

Worst `i2s_write` blocking time seen, in µs.

### `void accountBlockTiming(int64_t iterationUs, int64_t writeUs, int64_t budgetUs, bool writeComplete, bool countUnderruns)`

**Description:**

Accounts one iteration: updates both worst cases and, once the
       ring is primed, the underrun count.
