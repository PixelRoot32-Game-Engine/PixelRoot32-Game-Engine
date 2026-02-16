# Changelog

All notable changes to this project will be documented in this file.

## 0.8.1-dev

- **Engine Optimization & Fixes**:
  - **Critical Fix**: Resolved a double-buffer send issue in the ESP32 render loop where `drawer->present()` was being called redundantly after `renderer.endFrame()`, saving ~23ms per frame.
  - **Performance**: Disabled periodic Serial logs (Heartbeat, DMA Profiling) to eliminate stuttering during gameplay.

## 0.8.0-dev

- **Display Pipeline Optimization & Scaling**:
  - **TFT_eSPI Driver**:
    - Implemented **Parallel DMA Pipeline**: Decoupled SPI transmission from CPU rendering, allowing the next block to be processed while the previous one is sending. This significantly improves FPS.
    - **1:1 Fast Path**: Added a dedicated, optimized rendering path for scenarios where logical resolution matches physical resolution (or uses only offsets), bypassing scaling LUTs and using 32-bit memory writes for speed.
    - **Optimized `scaleLine`**: Rewrote the scaling logic with aggressive loop unrolling (8x) and forced Lookup Tables (LUTs) into internal RAM (`MALLOC_CAP_INTERNAL`) to eliminate flash latency.
    - **Reduced Latency**: Removed artificial `vTaskDelay` in the main loop, replacing it with `yield()`, unlocking potential FPS.
    - **Configurable DMA Buffer**: Added `LINES_PER_BLOCK` constant (tuned to 20 lines) to balance RAM usage vs. interrupt overhead.
  - **U8G2 Driver (OLED)**:
    - **Native XBM Support**: Refactored the internal buffer to be row-aligned and compatible with XBM format.
    - **Zero-Copy Rendering**: Replaced per-pixel drawing with direct `drawXBM` calls, massively reducing CPU overhead for monochrome displays.
    - **Optimized Scaling**: Implemented a fast scaling algorithm that writes directly to a temporary physical buffer in XBM format, enabling single-call updates to the display.

## v0.7.0-dev

- **Unified Platform Configuration & Hardware Decoupling**:
  - Consolidated global configuration files into `include/platforms/` (`PlatformCapabilities.h`, `PlatformDefaults.h`, `EngineConfig.h`).
  - Implemented bridge headers with `#pragma message` warnings for backward compatibility.
  - Added `PlatformDefaults.h` to manage target-dependent feature defaults (e.g., DAC support).
  - Fixed build failures on ESP32-S3 and other modern variants by conditionally compiling the DAC audio backend only for classic ESP32 [PR #49](https://github.com/PixelRoot32-Game-Engine/PixelRoot32-Game-Engine/pull/49).
  - Removed hardcoded CPU core IDs, replacing them with `PR32_DEFAULT_AUDIO_CORE` and `PR32_DEFAULT_MAIN_CORE` macros for configurable task affinity.
  - Added explicit feature guards for audio backends (`PIXELROOT32_USE_I2S_AUDIO`, `PIXELROOT32_NO_DAC_AUDIO`) to support modern ESP32 variants (e.g., ESP32-S3).
- **Graphics Extensibility & Ownership Management**:
  - Introduced `BaseDrawSurface` class with default primitive implementations to simplify custom driver development.
  - Added `U8G2_Drawer` implementation for monochromatic OLED display support via the U8G2 library.
  - Added `PIXELROOT32_CUSTOM_DISPLAY` macro and factory methods for safe custom driver initialization.
  - Implemented `unique_ptr` ownership transfer for `DrawSurface` instances between `DisplayConfig`, `Renderer`, and `Engine`.
  - Refactored existing drivers to inherit from `BaseDrawSurface` and removed deprecated text rendering methods.
- **Decoupled Multi-Core Audio Architecture**:
  - Moved audio generation and sequencing to **Core 0** (ESP32) and dedicated system threads (Native/PC).
  - Implemented **sample-accurate timing**, replacing frame-based `deltaTime` updates for perfect music and SFX synchronization.
  - Introduced `AudioScheduler` (Native, ESP32, Default) to own audio state, timing, and sequencing logic.
  - Added a lock-free **Single Producer / Single Consumer (SPSC)** `AudioCommandQueue` for thread-safe communication between the game loop and the audio core.
  - **Hardware-Specific Mixer Optimizations**:
    - Added a **non-linear mixer** with soft clipping to prevent digital distortion.
    - Implemented a high-performance **Look-Up Table (LUT)** mixer for no-FPU architectures (e.g., ESP32-C3).
    - Added automatic hardware detection (via `SOC_CPU_HAS_FPU`) to select the optimal mixing strategy.
  - **ESP32 DAC Backend Improvements**:
    - Optimized internal DAC driver with a software-based delivery system for maximum stability.
    - Added **0.7x output scaling** specifically for the **PAM8302A** amplifier to prevent analog saturation.
    - Updated documentation with clear wiring diagrams and hardware limitations.
  - Refactored `AudioEngine` and `MusicPlayer` into "thin clients" that act as command producers.
  - Removed obsolete `update(deltaTime)` methods from audio classes, simplifying the game loop.
  - Achieved full SDL2 parity with ESP32 multi-core behavior through background thread isolation.
- **Documentation & QA**:
  - Updated technical references in `API_REFERENCE.md` and `README.md` for the new directory structure.
  - Added documentation for custom display drivers and new build flags.
  - Verified engine integrity with 260+ test cases across Native and ESP32 environments.

## v0.6.0-dev

- **Independent Resolution Scaling**: Introduced logical/physical resolution decoupling to reduce memory usage and improve performance.
  - New `DisplayConfig` with separate logical and physical dimensions.
  - Added `ResolutionPresets` helper and `EngineConfig.h` for centralized configuration.
  - Optimized scaling using LUTs and IRAM-cached functions for ESP32.
  - Updated SDL2 and TFT_eSPI drivers to support scaling.
  - Updated Scene, UI, and Physics systems to operate on logical resolution.
  - Comprehensive documentation added in `README.md` and `docs/RESOLUTION_SCALING.md`.
- **Comprehensive Debug Overlay**: Replaced the basic FPS overlay with a new debug display showing FPS, RAM usage, and estimated CPU load.
  - Metrics update every 16 frames to minimize performance impact.
  - Enabled via the new `PIXELROOT32_ENABLE_DEBUG_OVERLAY` flag (supersedes `PIXELROOT32_ENABLE_FPS_DISPLAY`).
- **Standardized Display Rotation**: Standardized rotation handling and initialization order across all drivers.
  - Standardized rotation input (0-3 index or 90-270 degrees) in `TFT_eSPI_Drawer` and `SDL2_Drawer`.
  - Fixed initialization order in `Renderer` to apply rotation before driver init.
  - Updated `main.cpp` and `main_native.cpp` to use new `PHYSICAL_DISPLAY_*` macros.
  - Removed obsolete `Config.h` dependency from `main.cpp`.
- **ESP32 & Rendering Performance**:
  - Implemented DMA double buffering for block transfers (10-line blocks) to reduce overhead.
  - Added pre-calculated palette LUT to avoid runtime 8bpp to 16bpp conversion.
  - Updated SDL2 driver with proper scaling and VSYNC support.
- **Fixed Position UI Support**: Added support for UI elements that ignore camera scrolling.
  - New `setOffsetBypass()` and `isOffsetBypassEnabled()` methods in `Renderer`.
  - Added `fixedPosition` flag and accessors to `UILayout` base class.
  - `UIVerticalLayout` now bypasses offsets when the `fixedPosition` flag is enabled.

## v0.5.0-dev

- **Generic Tilemap Support (2bpp & 4bpp)**: Refactored `TileMap` into a template `TileMapGeneric` to support different sprite types. Added conditional type aliases (`TileMap2bpp`, `TileMap4bpp`) and corresponding `drawTileMap` overloads. Tilemap rendering now automatically uses the Background palette context. API documentation updated to describe the new generic structure and aliases.
- **Rendering & Scene Performance**:
  - Replaced `ArduinoQueue` with a fixed array for O(1) entity access and sorting.
  - Added viewport culling for entities and tilemaps to skip off-screen elements.
  - Implemented palette caching in tilemap rendering to avoid repeated color resolution.
  - Optimized sprite bit access patterns and added internal sprite drawing methods to reduce code duplication.
- **ESP32 Optimizations**: Applied `IRAM_ATTR` to critical rendering functions (`drawPixel`, `drawSpriteInternal`, `resolveColor`, `drawTileMap`) so they execute from internal RAM on ESP32, bypassing slower flash access for improved performance. Documentation updated to reflect these optimizations.
- **Optional FPS Overlay**: Introduced build flag `PIXELROOT32_ENABLE_FPS_DISPLAY` to enable an on-screen FPS counter in the top-right corner. FPS is calculated by averaging frame times over a defined interval and updates every 8 frames to reduce CPU load. Refined FPS calculation and initialization for more stable readings.
- **Documentation**: Documented how to override `MAX_LAYERS` and `MAX_ENTITIES` defaults via compiler flags in README.md and docs/API_REFERENCE.md. `Scene.h` now provides default definitions only when not already defined, and `Scene.cpp` uses the `MAX_LAYERS` constant so user overrides are respected.

## v0.4.1-dev

- **Palette Readability & Alignment**: Reorganized all predefined palettes (`NES`, `GB`, `GBC`, `PICO8`, `PR32`) to align with the `Color.h` enum sequence.
- **Descriptive Color Names**: Added descriptive color names (e.g., "Black", "White", "Navy") as comments next to each hex value in all palette arrays. This improves code readability and helps developers quickly identify colors when working with these predefined palettes.

## v0.4.0-dev

- **UI CheckBox Support**: Introduced the `UICheckBox` element for toggleable states.
  - Added new `UICheckBox` class with checked state management and callback support (`onCheckChanged`).
  - Extended `UIElementType` enum to include the `CHECKBOX` type.
  - Updated all layout containers (`UIGridLayout`, `UIVerticalLayout`, `UIHorizontalLayout`) to support checkbox elements.
- **Improved UI Text Precision**: Refactored `UILabel` and `UIButton` to use `FontManager` for pixel-perfect text dimensions.
  - Replaced manual width calculations with `FontManager::textWidth`.
  - Optimized `UILabel` by removing the dirty flag and implementing immediate dimension recalculation in `setText` and `centerX`.
  - Added a safety fallback to default calculations when no custom font is loaded.
- **Input & Stability**: Fixed button `stateChanged` reset logic in `InputManager` to prevent stale input states from affecting UI interactions.
- **Documentation**: Updated API reference and user manuals to include `UICheckBox` usage and reflect the latest UI behavior.

## v0.3.0-dev

- **Renderer Fix**: Fixed 2bpp/4bpp sprite clipping when camera offset is applied. Clipping now uses finalX/finalY with xOffset/yOffset, preventing the player from disappearing past the viewport width.
- **Dual Palette Mode**: Introduced dual palette mode allowing separate palettes for backgrounds and sprites. Added new methods: `enableDualPaletteMode`, `setBackgroundPalette`, `setSpritePalette`, `setBackgroundCustomPalette`, `setSpriteCustomPalette`, `setDualPalette`, and `setDualCustomPalette`. Updated `resolveColor` to support context-based color resolution for dual palette mode.
- **Native Bitmap Font System**: Implemented a native bitmap font system using 1bpp sprites for consistent text rendering across platforms. Introduced `Font` and `FontManager` classes to manage bitmap fonts and their usage. Updated `Renderer` to support new text rendering methods, allowing for custom fonts and sizes. Added built-in 5x7 font for immediate use.
- **UI Layout System**: Introduced comprehensive UI layout management system:
  - `UIVerticalLayout` for automatic vertical organization with scrolling support (NES-style instant scroll and smooth scrolling options).
  - `UIHorizontalLayout` with scrolling support.
  - `UIGridLayout` with navigation and automatic selection management.
  - `UIAnchorLayout` for fixed-position HUD elements with optimized performance (no reflow).
  - `UIPaddingContainer` and `UIPanel` for enhanced UI organization.
  - Viewport culling and optimized rendering for embedded systems.
- **Core UI System**: Introduced base UI system with `UIElement`, `UIButton`, `UIElementType`, focusability, and layout components.
- **License**: Transitioned project license from GPL-3.0 to MIT for broader compatibility and ease of use.

## v0.2.0-dev

- **Documentation Overhaul**: Added comprehensive Table of Contents, step-by-step SDL2 installation guide for Windows (MSYS2), and critical PlatformIO installation notes.
- **Architecture**: Moved `DrawSurface` implementation handling to the engine core. This removes the need for manual developer implementation and facilitates the integration of future display drivers.
- **Driver Support**: Clarified driver support status (TFT_eSPI & SDL2) and roadmap.

## v0.1.0-dev

- **Initial Public Preview.**
- **Core Architecture**: Scene, Entity, Actor, and PhysicsActor system.
- **Rendering**: 1bpp Sprites, MultiSprite (layered colors), and Tilemap support.
- **Audio**: NES-style sound engine (Pulse, Triangle, Noise channels).
- **Physics**: AABB Collision detection and basic kinematics.
- **Platform Support**: ESP32 (SPI/DMA) and PC (SDL2) targets.
- **Tools**: Added Sprite Compiler python tool.
- **Experimental Build Flags**:
  - `PIXELROOT32_ENABLE_2BPP_SPRITES`: Enables support for 2bpp (4-color) packed sprites.
  - `PIXELROOT32_ENABLE_4BPP_SPRITES`: Enables support for 4bpp (up to 16-color) packed sprites, intended for high-fidelity UI elements or special effects where more colors per sprite are needed.
  - `PIXELROOT32_ENABLE_SCENE_ARENA`: Enables dedicated memory arena for scene management.
