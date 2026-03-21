# Migration Guide: v1.0.0 → v1.1.0

## 🧠 Platform Memory Abstraction

Version 1.1.0 introduces a centralized platform abstraction layer for memory operations, specifically targeting Flash/PROGMEM access on embedded systems vs. standard RAM access on native platforms.

### 1. Unified Memory Macros

Include **`platforms/PlatformMemory.h`** to use the new unified API. This replaces manual `#ifdef ESP32` blocks and direct `pgm_read_*` calls.

**Before:**

```cpp
#if defined(ESP32)
#include <pgmspace.h>
#endif

const char MY_STRING[] PROGMEM = "Hello";
char buffer[10];
strcpy_P(buffer, MY_STRING);
uint8_t val = pgm_read_byte(&my_array[i]);
```

**After:**

```cpp
#include "platforms/PlatformMemory.h"

const char MY_STRING[] PIXELROOT32_FLASH_ATTR = "Hello";
char buffer[10];
PIXELROOT32_STRCMP_P(buffer, MY_STRING);
uint8_t val = PIXELROOT32_READ_BYTE_P(&my_array[i]);
```

### 2. Macro Mapping Table

| Legacy Macro/Attribute | New Unified Macro | Description |
|------------------------|-------------------|-------------|
| `PROGMEM` | `PIXELROOT32_FLASH_ATTR` | Data attribute for Flash storage |
| `strcmp_P` | `PIXELROOT32_STRCMP_P` | Compare string with Flash string |
| `memcpy_P` | `PIXELROOT32_MEMCPY_P` | Copy from Flash memory |
| `pgm_read_byte(addr)` | `PIXELROOT32_READ_BYTE_P(addr)` | Read 8-bit value |
| `pgm_read_word(addr)` | `PIXELROOT32_READ_WORD_P(addr)` | Read 16-bit value |
| `pgm_read_dword(addr)` | `PIXELROOT32_READ_DWORD_P(addr)` | Read 32-bit value |
| `pgm_read_float(addr)` | `PIXELROOT32_READ_FLOAT_P(addr)` | Read float value |
| `pgm_read_ptr(addr)` | `PIXELROOT32_READ_PTR_P(addr)` | Read pointer |

---

> [!TIP]
> Using these macros ensures your code remains compatible with the Desktop simulator (Native) without extra `#ifdef` logic, as they automatically resolve to standard C memory operations on non-embedded platforms.

---

## 📝 Unified Logging System

Version 1.1.0 introduces a centralized logging abstraction layer that works consistently across ESP32 and native platforms, eliminating the need for platform-specific `#ifdef` blocks in your logging code.

### 1. Logging API

Include **`core/Log.h`** for platform-agnostic logging with different log levels.

**Before:**

```cpp
#ifdef ESP32
    Serial.print("[INFO] Player position: ");
    Serial.println(playerX);
#else
    printf("[INFO] Player position: %d\n", playerX);
#endif
```

**After:**

```cpp
#include "core/Log.h"

using namespace pixelroot32::core::logging;

// Log with explicit level
log(LogLevel::Info, "Player position: %d", playerX);

// Log with default Info level (shorthand)
log("Player position: %d", playerX);
```

### 2. Log Levels Mapping

| LogLevel Enum | Output Prefix | Use Case |
|--------------|---------------|----------|
| `LogLevel::Info` | `[INFO]` | General information, debug messages |
| `LogLevel::Warning` | `[WARN]` | Warnings, non-critical issues |
| `LogLevel::Error` | `[ERROR]` | Errors, critical failures |

### 4. Migration Checklist

- [ ] Replace `#ifdef ESP32 Serial.print/println` blocks with `log()` calls
- [ ] Replace `printf()` calls in native code with `log()` calls
- [ ] Remove manual `#ifdef` blocks around logging statements
- [ ] Use explicit `LogLevel` for warnings and errors
- [ ] Use the shorthand `log(fmt, ...)` for Info-level messages

> [!TIP]
> The logging system automatically routes to the appropriate output (Serial for ESP32, stdout for native) without polluting your code with platform checks.
