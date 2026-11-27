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

The engine separates *which platform this build targets* (identity) from *what that build can use* (capability). Include `platforms/PlatformSelect.h` and use these in your own game code when you need platform-specific behavior.

Identity — every macro is always defined, and exactly one is `1`:

- `PIXELROOT32_PLATFORM_ESP32`: firmware on the board.
- `PIXELROOT32_PLATFORM_DESKTOP`: native executable (Windows, macOS, Linux).
- `PIXELROOT32_PLATFORM_WEB`: Emscripten / WebAssembly.

Capability — independent of identity:

- `PIXELROOT32_HAS_SDL`: SDL2 is available (desktop and web).
- `PIXELROOT32_HAS_ARDUINO`: the Arduino API is available (probed from `ARDUINO`, so it is true only where `<Arduino.h>` really exists).
- `PIXELROOT32_HAS_HOST_FS`: a file system with `fopen`/`stat`/`rename` is available (desktop, and the web through Emscripten's virtual FS). It answers "may a file adapter run here?", not "is this a PC".

Because the identity macros are always defined, test their value rather than their existence: `#ifdef PIXELROOT32_PLATFORM_ESP32` is true even on desktop.

**Usage Example:**
```cpp
#include "platforms/PlatformSelect.h"

#if PIXELROOT32_HAS_ARDUINO
    // Setup ESP32-specific hardware pins
    pinMode(2, OUTPUT);
#else
    // Setup PC equivalent or mock
    PR32_LOG_INFO("Running on PC emulator");
#endif
```

### Platform Memory and Flash Access

`PlatformMemory.h` does **not** provide an allocator. It provides the flash-access macros that let the same source read asset tables from ESP32 flash and from ordinary RAM on a host. There is no `PSRAM` or `DMA` allocation macro in the engine today: use `malloc`/`new` and the platform's own API when a target needs PSRAM.

| Macro | ESP32 (and AVR) | Desktop / Web |
|-------|-----------------|---------------|
| `PIXELROOT32_FLASH_ATTR` | `PROGMEM` | empty |
| `PIXELROOT32_READ_BYTE_P(addr)` | `pgm_read_byte` | `*(const uint8_t*)(addr)` |
| `PIXELROOT32_READ_WORD_P` / `_DWORD_P` / `_FLOAT_P` / `_PTR_P` | `pgm_read_*` | plain dereference |
| `PIXELROOT32_MEMCPY_P` / `PIXELROOT32_STRCMP_P` | `memcpy_P` / `strcmp_P` | `memcpy` / `strcmp` |
| `PIXELROOT32_SCENE_FLASH_ATTR` | alias of `PIXELROOT32_FLASH_ATTR` | alias of `PIXELROOT32_FLASH_ATTR` |

**Usage Example:**
```cpp
#include "platforms/PlatformMemory.h"

// A table lives in flash on the board and in plain RAM on a host.
const uint8_t kTilemap[] PIXELROOT32_FLASH_ATTR = { /* ... */ };

const uint8_t first = PIXELROOT32_READ_BYTE_P(&kTilemap[0]);
```

The practical consequence is that assets compiled as headers need no filesystem on any target: a browser build draws a game without preloading a single file.

### Platform Capabilities

Detected hardware capabilities, such as the number of CPU cores and recommended audio task pinning. 

> **Note**: For detailed information on the `PlatformCapabilities` struct, refer to the [Core Module](core.md#platformcapabilities).

## Related Documentation

- [API Reference](index.md) - Main index
- [Core Module](core.md) - PlatformCapabilities
- [Configuration](config.md) - Build flags