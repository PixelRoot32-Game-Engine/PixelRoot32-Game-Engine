# API Reference: Platform Abstractions

> **Source of truth:**
> - `include/core/Log.h`
> - `include/platforms/PlatformMemory.h`
> - `include/platforms/PlatformCapabilities.h`

## Overview

The Platform module provides a hardware abstraction layer (HAL) for the engine. It ensures that platform-specific features (like memory allocation on the ESP32) can be used optimally, while falling back to standard implementations on other platforms (like Windows/Linux via SDL2).

## Key Concepts

### Logging System

A unified logging system that outputs to the Serial console on ESP32 or standard output on PC. It is enabled by defining `PIXELROOT32_DEBUG_MODE=1` in your build flags. If debug mode is disabled, all log macros are stripped by the preprocessor, resulting in zero overhead.

**Log Levels:**
- `PR32_LOG_ERROR(fmt, ...)`: Critical errors.
- `PR32_LOG_WARN(fmt, ...)`: Warnings.
- `PR32_LOG_INFO(fmt, ...)`: General information.
- `PR32_LOG_DEBUG(fmt, ...)`: Verbose debug info.

**Usage Example:**
```cpp
#include "core/Log.h"

void init() {
    PR32_LOG_INFO("Initializing game... Width: %d", 240);
}
```

### Conditional Compilation

The engine uses macros to detect the current platform. You can use these in your own game code if you need platform-specific behavior.

- `PIXELROOT32_PLATFORM_ESP32`: Defined when compiling for any ESP32 variant.
- `PIXELROOT32_PLATFORM_NATIVE`: Defined when compiling for PC (SDL2).

**Usage Example:**
```cpp
#ifdef PIXELROOT32_PLATFORM_ESP32
    // Setup ESP32-specific hardware pins
    pinMode(2, OUTPUT);
#else
    // Setup PC equivalent or mock
    PR32_LOG_INFO("Running on PC emulator");
#endif
```

### Platform Memory Allocation

On the ESP32, standard `malloc` and `new` allocate from the default internal RAM. However, the ESP32 also has external PSRAM and faster internal IRAM. The `PlatformMemory` macros abstract these platform-specific allocations. On PC, these macros safely fall back to standard `malloc`/`free`.

| Macro | Description |
|-------|-------------|
| `PR32_MALLOC(size)` | Standard allocation (internal RAM). |
| `PR32_MALLOC_PSRAM(size)` | Allocates in external PSRAM (if available). Great for large tilemaps or audio buffers. |
| `PR32_MALLOC_DMA(size)` | Allocates DMA-capable memory. Required for SPI display buffers. |
| `PR32_FREE(ptr)` | Safely frees memory allocated by any of the above macros. |

**Usage Example:**
```cpp
#include "platforms/PlatformMemory.h"

// Allocate a large buffer in PSRAM to save precious internal RAM
uint8_t* largeBuffer = (uint8_t*)PR32_MALLOC_PSRAM(1024 * 1024);

if (largeBuffer != nullptr) {
    // Use buffer...
    PR32_FREE(largeBuffer);
}
```

### Platform Capabilities

Detected hardware capabilities, such as the number of CPU cores and recommended audio task pinning. 

> **Note**: For detailed information on the `PlatformCapabilities` struct, refer to the [Core Module](core.md#platformcapabilities).

## Related Documentation

- [API Reference](index.md) - Main index
- [Core Module](core.md) - PlatformCapabilities
- [Configuration](config.md) - Build flags