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
