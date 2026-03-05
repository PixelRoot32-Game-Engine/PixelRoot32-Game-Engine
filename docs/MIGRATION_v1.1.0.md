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
