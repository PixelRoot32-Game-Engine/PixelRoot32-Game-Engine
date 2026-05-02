# Platform configuration

This page is the **Guide** entry point for **PlatformIO**, **`build_flags`**, and engine **feature toggles**. Detailed hardware matrices, per-chip audio/display constraints, example `[env:…]` blocks, and troubleshooting are kept in a single canonical reference (previously duplicated here):

- **[Platform compatibility & build configuration](../reference/platform-compatibility.md)** — ESP32 variants, FPU vs `Fixed16`, DAC/I2S, native SDL2, memory classes, and copy-paste `platformio.ini` examples.

## Feature macros

Subsystem compilation uses `PIXELROOT32_ENABLE_*` and related defines (see [API — Configuration](../api/config.md) and `include/platforms/PlatformDefaults.h`).

Typical flags in `platformio.ini`:

```ini
build_flags =
    -std=gnu++17
    -fno-exceptions
    -D PIXELROOT32_ENABLE_AUDIO=1
    -D PIXELROOT32_ENABLE_PHYSICS=1
    -D PIXELROOT32_ENABLE_UI_SYSTEM=1
```

Disable unused subsystems on RAM-constrained boards; the reference guide explains interactions (e.g. **C3** without FPU, **classic ESP32** DAC vs I2S).

## Related

- [Resolution scaling](./resolution-scaling.md) — logical vs physical display
- [Memory](./memory.md) — pools and embedded budgets
- [Testing](./testing.md) — `native_test` and coverage
