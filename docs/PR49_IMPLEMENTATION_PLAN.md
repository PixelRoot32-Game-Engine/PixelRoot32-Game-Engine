# Work Plan: PR-49 Enhancements Implementation

Based on the technical review of PR-49, this plan outlines the phases required to integrate the suggested enhancements into the PixelRoot32 engine core.

## Phase 1: Unified Configuration & Platform Detection

- **Task 1.1: Consolidate PlatformDefaults.h**
  - Define compile-time defaults for Core Affinity (e.g., `PR32_DEFAULT_AUDIO_CORE`).
  - **New**: Implement Display Driver Selection logic (supporting `TFT_eSPI` and future `U8G2`).
  - Integrate with `PlatformCapabilities::detect()` so it respects these overrides.
- **Task 1.2: Standardize Driver Guards**
  - Refactor [TFT_eSPI_Drawer.h](file:///c:/Users/gperez88/Documents/Proyects/Games/pixelroot32%20workspace/PixelRoot32-Game-Samples/lib/PixelRoot32-Game-Engine/include/drivers/esp32/TFT_eSPI_Drawer.h) to use `PIXELROOT32_USE_TFT_ESPI_DRIVER`.
  - Prepare structure for future `U8G2_Drawer.h` integration.

## Phase 2: Backend Refinement and Consistency

This phase ensures all audio backends follow the same architectural patterns and configuration capabilities.

- **Task 2.1: Implement I2S Feature Guards**
  - Add `PIXELROOT32_USE_I2S_AUDIO` to [PlatformDefaults.h](file:///c:/Users/gperez88/Documents/Proyects/Games/pixelroot32%20workspace/PixelRoot32-Game-Samples/lib/PixelRoot32-Game-Engine/include/PlatformDefaults.h).
  - Update [ESP32_I2S_AudioBackend.h](file:///c:/Users/gperez88/Documents/Proyects/Games/pixelroot32%20workspace/PixelRoot32-Game-Samples/lib/PixelRoot32-Game-Engine/include/drivers/esp32/ESP32_I2S_AudioBackend.h) and [ESP32_I2S_AudioBackend.cpp](file:///c:/Users/gperez88/Documents/Proyects/Games/pixelroot32%20workspace/PixelRoot32-Game-Samples/lib/PixelRoot32-Game-Engine/src/drivers/esp32/ESP32_I2S_AudioBackend.cpp) with the new guards.
- **Task 2.2: Decouple Core Affinity**
  - Replace hardcoded core IDs in [ESP32_DAC_AudioBackend.cpp](file:///c:/Users/gperez88/Documents/Proyects/Games/pixelroot32%20workspace/PixelRoot32-Game-Samples/lib/PixelRoot32-Game-Engine/src/drivers/esp32/ESP32_DAC_AudioBackend.cpp) and [ESP32_I2S_AudioBackend.cpp](file:///c:/Users/gperez88/Documents/Proyects/Games/pixelroot32%20workspace/PixelRoot32-Game-Samples/lib/PixelRoot32-Game-Engine/src/drivers/esp32/ESP32_I2S_AudioBackend.cpp) with the `PIXELROOT32_AUDIO_TASK_CORE` macro.

## Phase 3: Documentation and Quality Assurance

Final phase to ensure the new features are discoverable and the engine remains stable.

- **Task 3.1: Update Technical Documentation**
  - Add a "Platform Configuration" section to [AUDIO_NES_SUBSYSTEM_REFERENCE.md](file:///c:/Users/gperez88/Documents/Proyects/Games/pixelroot32%20workspace/PixelRoot32-Game-Samples/lib/PixelRoot32-Game-Engine/docs/AUDIO_NES_SUBSYSTEM_REFERENCE.md).
  - Document the new build flags: `PIXELROOT32_NO_DAC_AUDIO`, `PIXELROOT32_AUDIO_TASK_CORE`, and `PIXELROOT32_USE_I2S_AUDIO`.
- **Task 3.2: Verification**
  - Perform test builds for:
    - Classic ESP32 (Internal DAC enabled).
    - ESP32-S3 (DAC disabled, I2S enabled).
    - Native/Mock platform (Ensuring no regressions in desktop builds).

---
**Status Tracking**

- Phase 1: [x] Completed
- Phase 2: [x] Completed
- Phase 3: [ ] Pending
