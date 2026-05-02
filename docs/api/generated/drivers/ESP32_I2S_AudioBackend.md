# ESP32_I2S_AudioBackend

<Badge type="info" text="Class" />

**Source:** `ESP32_I2S_AudioBackend.h`

**Inherits from:** [AudioBackend](../audio/AudioBackend.md)

## Description

Audio backend implementation for ESP32 using I2S.

Uses a FreeRTOS task to continuously feed the I2S DMA buffer
to ensure smooth playback independent of the game loop frame rate.

## Inheritance

[AudioBackend](../audio/AudioBackend.md) → `ESP32_I2S_AudioBackend`

## Methods

### `void audioTaskLoop()`
