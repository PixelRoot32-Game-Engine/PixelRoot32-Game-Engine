# Layer 0: Hardware Layer

## Responsibility

Underlying physical hardware that the engine runs on.

---

## Components

### Microcontrollers

- **ESP32/ESP32-S3**: Main target microcontrollers
  - Dual-core Xtensa LX6 processors
  - Wi-Fi and Bluetooth connectivity
  - Various memory configurations (520KB SRAM on classic ESP32, 512KB on S3)
  
- **ESP32-C3**: RISC-V variant
  - Single-core RISC-V processor
  - No FPU (uses Fixed16 math)
  - 400KB SRAM

### Displays

| Display | Type | Resolution | Interface | Use Case |
|---------|------|------------|-----------|----------|
| ST7789 | TFT LCD | 240x240, 320x240 | SPI | Color games, high resolution |
| ST7735 | TFT LCD | 128x128, 160x128 | SPI | Smaller color displays |
| SSD1306 | OLED | 128x64, 128x32 | I2C/SPI | Monochrome, low power |
| SH1106 | OLED | 128x64 | I2C/SPI | Alternative monochrome |

### Audio Hardware

| Component | Type | Description |
|-----------|------|-------------|
| Internal DAC | 8-bit | ESP32 GPIO 25/26, PAM8302A amplifier |
| I2S + MAX98357A | Digital | High-quality audio, class D amp |
| I2S + PCM5102 | Digital | DAC for headphones/line out |

### Input

- **Physical Buttons**: Connected to GPIOs
  - Typical configurations: 4-directional + 2 action buttons
  - Direct GPIO polling with debouncing
  
- **Touch Controllers**:
  - XPT2046: Resistive touch (SPI)
  - GT911: Capacitive touch (I2C)

### PC/Native Platform

- **Simulation**: SDL2 on Windows/Linux/macOS
- **Purpose**: Rapid development without hardware
- **Features**: Full API compatibility, faster iteration

---

## Hardware Capabilities Detection

The engine uses `PlatformCapabilities` structure to detect hardware at runtime:

```cpp
struct PlatformCapabilities {
    bool hasDualCore;      // Multi-core support
    int audioCoreId;       // Recommended core for audio
    int mainCoreId;        // Recommended core for game loop
    bool hasFPU;           // Floating-point unit available
    size_t totalSRAM;      // Total SRAM available
};
```

---

## Memory Layout (ESP32)

```
┌─────────────────────────────────────┐
│           DRAM (520KB)              │
│  ├─ .dram0.bss (static data)        │
│  ├─ Heap (dynamic allocations)      │
│  └─ Stack                           │
├─────────────────────────────────────┤
│           IRAM (128KB)              │
│  └─ Instruction RAM (cached code)   │
├─────────────────────────────────────┤
│           Flash (4MB+)              │
│  └─ Program code and PROGMEM data   │
└─────────────────────────────────────┘
```

---

## Related Documentation

- [Driver Layer](ARCH_LAYER_DRIVERS.md) - Hardware abstraction drivers
- [Abstraction Layer](ARCH_LAYER_ABSTRACTION.md) - PlatformMemory and cross-platform abstractions
- [Memory System](ARCH_MEMORY_SYSTEM.md) - Memory management strategies
- [Platform Compatibility](../PLATFORM_COMPATIBILITY.md) - Supported hardware matrix
