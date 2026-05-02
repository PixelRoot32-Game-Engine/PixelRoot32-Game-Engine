# AudioBackend

<Badge type="info" text="Class" />

**Source:** `AudioBackend.h`

## Description

Abstract interface for platform-specific audio drivers.

This class abstracts the underlying audio hardware or API (e.g., SDL2, I2S).
It is responsible for requesting audio samples from the AudioEngine and
pushing them to the output device.

## Methods

### `virtual int getSampleRate() const`

**Description:**

Returns the configured sample rate of the backend.

**Returns:** Sample rate in Hz (e.g., 22050, 44100).
