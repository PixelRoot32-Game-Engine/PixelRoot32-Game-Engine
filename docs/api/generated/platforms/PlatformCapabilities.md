# PlatformCapabilities

<Badge type="info" text="Struct" />

**Source:** `PlatformCapabilities.h`

## Description

Represents the hardware capabilities of the current platform.

This structure allows the engine to adapt to different hardware configurations
(e.g., single-core vs dual-core ESP32) without excessive #ifdefs.

## Methods

### `static PlatformCapabilities detect()`

**Description:**

Detects capabilities of the current platform.

**Returns:** A populated PlatformCapabilities struct.
