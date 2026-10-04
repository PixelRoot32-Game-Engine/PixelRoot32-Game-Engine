# PlatformCapabilities

<Badge type="info" text="Struct" />

**Source:** `PlatformCapabilities.h`

## Description

Represents the hardware capabilities of the current platform.

This structure allows the engine to adapt to different hardware configurations
(e.g., single-core vs dual-core ESP32) without excessive #ifdefs.

## Properties

| Name | Type | Description |
|------|------|-------------|
| `audioCoreId` | `int` | Recommended core ID for audio processing. On dual-core ESP32, this is typically 0. On single-core, it's 0. |
| `mainCoreId` | `int` | Recommended core ID for the main game loop. On dual-core ESP32, this is typically 1. On single-core, it's 0. |
| `audioPriority` | `int` | Recommended task priority for audio. |

## Methods

### `static PlatformCapabilities detect()`

**Description:**

Detects capabilities of the current platform.

**Returns:** A populated PlatformCapabilities struct.
