# AudioConfig

<Badge type="info" text="Struct" />

**Source:** `AudioConfig.h`

## Description

Configuration for the Audio subsystem.

## Properties

| Name | Type | Description |
|------|------|-------------|
| `backend` | `AudioBackend*` | Pointer to the platform-specific audio backend. |
| `sampleRate` | `int` | Desired sample rate in Hz. |
| `blockSize` | `int` | Audio block size (samples). Must be multiple of 128. |

## Methods

### `: backend(backend), sampleRate(sampleRate), blockSize(blockSize)`

**Description:**

Constructs an AudioConfig with platform-adaptive block size.

**Parameters:**

- `backend`: Pointer to the audio backend implementation. May be nullptr for headless configs.
- `sampleRate`: Desired sample rate in Hz (default 22050 for retro feel).
- `blockSize`: Audio block size in samples. Defaults to 256 on FPU platforms, 128 on no-FPU platforms.
       Must be a multiple of 128 for I2S DMA alignment.
