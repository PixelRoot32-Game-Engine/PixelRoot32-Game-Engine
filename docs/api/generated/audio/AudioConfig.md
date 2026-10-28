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
| `blockSize` | `int` | Audio block size (samples): the render unit, multiple of 128. The I2S backend derives its DMA depth from it. |
