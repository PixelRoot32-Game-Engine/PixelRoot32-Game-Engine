# API Reference: Platform Abstractions

This document covers platform-specific abstractions including logging, memory management, and hardware capabilities in PixelRoot32.

> **Note:** This is part of the [API Reference](../API_REFERENCE.md). See the main index for complete documentation.

---

## Platform Abstractions Overview

Version 1.1.0 introduces unified abstractions for cross-platform operations, eliminating the need for manual `#ifdef` blocks in user code.

---

## Logging System

**Namespace:** `pixelroot32::core::logging`

The unified logging system provides platform-agnostic logging with different log levels, automatically routing to the appropriate output (Serial for ESP32, stdout for native). Enable with `-DPIXELROOT32_DEBUG_MODE` in build flags.

### Log Levels

| LogLevel Enum | Output Prefix | Use Case |
|--------------|---------------|----------|
| `LogLevel::Info` | `[INFO]` | General information, debug messages |
| `LogLevel::Profiling` | `[PROF]` | Performance timing markers |
| `LogLevel::Warning` | `[WARN]` | Warnings, non-critical issues |
| `LogLevel::Error` | `[ERROR]` | Errors, critical failures |

### Functions

- **`void log(LogLevel level, const char* format, ...)`**
    Logs a message with the specified level and printf-style formatting.

- **`void log(const char* format, ...)`**
    Logs a message with Info level (shorthand).

### Conditional Compilation

When `PIXELROOT32_DEBUG_MODE` is **not defined**, all `log()` calls become no-ops at compile time. The engine uses a double-layer conditional:

1. **`#ifdef PIXELROOT32_DEBUG_MODE`** in the header makes `log()` calls emit formatting code
2. **`if constexpr (EnableLogging)`** in the implementation skips runtime formatting

This means zero runtime cost in production builds (no string formatting, no branching).

### Usage Example

```cpp
// Enable in platformio.ini:
// build_flags = -D PIXELROOT32_DEBUG_MODE

#include "core/Log.h"

using namespace pixelroot32::core::logging;

// Log with explicit level
log(LogLevel::Info, "Player position: %d", playerX);

// Log warning
log(LogLevel::Warning, "Low memory: %d bytes free", freeRAM);

// Log error
log(LogLevel::Error, "Failed to load sprite: %s", filename);

// Log with default Info level
log("Player position: %d", playerX);
```

---

## Platform Memory Abstraction

**Include:** `platforms/PlatformMemory.h`

Provides a unified API for memory operations that differ between ESP32 (Flash/PROGMEM) and Native (RAM) platforms.

### Macros

- **`PIXELROOT32_FLASH_ATTR`**
    Attribute for data stored in Flash memory.

- **`PIXELROOT32_STRCMP_P(dest, src)`**
    Compare a RAM string with a Flash string.

- **`PIXELROOT32_MEMCPY_P(dest, src, size)`**
    Copy data from Flash to RAM.

- **`PIXELROOT32_READ_BYTE_P(addr)`**
    Read an 8-bit value from Flash.

- **`PIXELROOT32_READ_WORD_P(addr)`**
    Read a 16-bit value from Flash.

- **`PIXELROOT32_READ_DWORD_P(addr)`**
    Read a 32-bit value from Flash.

- **`PIXELROOT32_READ_FLOAT_P(addr)`**
    Read a float value from Flash.

- **`PIXELROOT32_READ_PTR_P(addr)`**
    Read a pointer from Flash.

### Usage Example

```cpp
#include "platforms/PlatformMemory.h"

const char MY_STRING[] PIXELROOT32_FLASH_ATTR = "Hello";
char buffer[10];
PIXELROOT32_STRCMP_P(buffer, MY_STRING);
uint8_t val = PIXELROOT32_READ_BYTE_P(&my_array[i]);
```

---

## PlatformCapabilities

**Namespace:** `pixelroot32::platforms`

A structure that holds detected hardware capabilities, used to optimize task pinning and threading.

### Properties

- **`bool hasDualCore`**: True if the hardware has more than one CPU core.
- **`int coreCount`**: Total number of CPU cores detected.
- **`int audioCoreId`**: Recommended CPU core for audio tasks.
- **`int mainCoreId`**: Recommended CPU core for the main game loop.
- **`int audioPriority`**: Recommended priority for audio tasks.

### Static Methods

- **`static PlatformCapabilities detect()`**: Automatically detects hardware capabilities based on the platform and configuration. It respects the defaults defined in `platforms/PlatformDefaults.h` and any compile-time overrides.

---

## Related Documentation

- [API Reference](../API_REFERENCE.md) - Main index
- [API Config](API_CONFIG.md) - Build flags and configuration
- [Platform Compatibility Guide](../PLATFORM_COMPATIBILITY.md)