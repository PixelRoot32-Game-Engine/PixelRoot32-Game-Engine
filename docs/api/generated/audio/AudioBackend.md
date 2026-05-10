# AudioBackend

<Badge type="info" text="Class" />

**Source:** `AudioBackend.h`

## Description

Abstract interface for platform-specific audio drivers.

This class abstracts the underlying audio hardware or API (e.g., SDL2 on native,
I2S on ESP32). It is responsible for requesting audio samples from the AudioEngine
and pushing them to the output device. Platforms implement this interface to
bridge between the engine's sample generation and the actual hardware.

Typical lifecycle:
1. init() is called with an AudioEngine pointer.
2. The backend sets up the audio output (I2S, SDL audio, etc.).
3. In its output callback, the backend calls engine->generateSamples().
4. On destruction, resources are cleaned up.

## Methods

### `virtual void init(AudioEngine* engine, const pixelroot32::platforms::PlatformCapabilities& caps = pixelroot32::platforms::PlatformCapabilities())`

**Description:**

Initializes the audio backend.

**Parameters:**

- `engine`: Pointer to the AudioEngine instance to request samples from.
       Must not be nullptr.
- `caps`: Platform capabilities to guide backend initialization
       (e.g., core pinning on ESP32, thread priorities).

### `virtual int getSampleRate() const`

**Description:**

Returns the configured sample rate of the backend.

**Returns:** Sample rate in Hz (e.g., 22050, 44100, 48000).
