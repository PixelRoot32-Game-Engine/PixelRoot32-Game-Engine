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

## Inheritance

[AudioBackend](../audio/AudioBackend.md) → `ESP32_DAC_AudioBackend`

## Methods

### `void audioTaskLoop()`
