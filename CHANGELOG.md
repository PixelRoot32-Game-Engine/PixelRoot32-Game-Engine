# Changelog

All notable changes to this project will be documented in this file.

## 1.3.0

### 🔊 Audio System Overhaul

- **Multi-Track Music Player**: Added support for up to 4 simultaneous music tracks (main + 3 sub-tracks) with NES-style tick-based synchronization for precise timing control.
- **ADSR Envelopes & LFO Modulation**: Full ADSR state machine (Attack/Decay/Sustain/Release) and LFO modulation with pitch (vibrato) and volume (tremolo) targets for expressive instrument sounds.
- **ApuCore Architecture**: Centralized audio synthesis, mixing, and sequencing core for consistent behavior across all platforms (ESP32 and Native).
- **Optimized Instrument Presets**: Refined default presets with improved sonic character — punchier percussion (kick, snare, hi-hat), more expressive melodic leads, and better mix clarity.
- **NES-Style Noise**: Support for short-mode LFSR (93-step metallic noise) and duty cycle sweeps on pulse channels for authentic retro sounds.

### ⚡ Performance & Profiling

- **Audio Peak Profiling**: New profiling API to capture audio peak statistics in a ring buffer for real-time diagnostics.
- **Multi-Producer Command Queue**: Enhanced audio command queue to support multiple producers with improved thread safety.
- **Sequencer Note Limits**: Added per-frame note limits to prevent CPU spikes during dense musical passages.

### 📚 Documentation

- **Audio API Clarifications**: Updated documentation to clarify that `instrumentToFrequency()` returns LFSR clock rates for the NOISE channel, not musical pitch frequencies.

## 1.2.2

### 🔊 Audio

- **NES-Style Deterministic Noise**: Replace legacy random-based noise with 15-bit LFSR for deterministic noise patterns matching authentic NES sound behavior.
- **Performance Limits**: Add `MAX_NOTES_PER_FRAME=8` limit to music sequencer to prevent CPU spikes during dense musical passages.
- **Fixed-Point Volume**: Pre-compute master volume as Q16 fixed-point for faster LUT mixing path.
- **ESP32 I2S Buffer**: Increase buffer size to 1024 samples to match native configuration and improve audio stability.
- **ESP32 DAC Optimization**: Use `dacWrite()` for direct register access instead of higher-level abstractions.
- **Performance Metrics**: Add metrics collection when profiling is enabled for audio system analysis.
- **Noise Timing Control**: Introduce `noisePeriodSamples` and `noiseCountdown` for better control of noise generation timing.
- **Command Queue Overflow Handling**: Update `ESP32AudioScheduler` to handle command queue overflow and log warnings when commands are dropped.
- **Interface Refinement**: Adjust `AudioEngine` and `AudioScheduler` interfaces to streamline audio event processing and sample generation.

### 🎨 Graphics & Animations

- **Tile Animation Wall-Clock Pacing**: Update `TileAnimationManager` to support wall-clock pacing with `step(deltaTimeMs)` for improved animation timing independent of frame rate.
- **Framebuffer Redraw Optimization**: Introduce `shouldRedrawFramebuffer()` in `Scene` to conditionally skip `draw()` and `present()` calls when visual state hasn't changed, reducing unnecessary rendering.
- **Visual Signature Computation**: Add efficient visual signature computation to determine framebuffer changes, avoiding redundant redraws.
- **Frame Duration Documentation**: Enhance documentation for `frameDuration` in `TileAnimation` to clarify its relation to logical ticks and animation pacing.
- **Example Optimization**: Refine `AnimatedTilemapScene` example code to demonstrate new optimizations and caching behavior.

### 🏀 Physics

- **Centralized Integration**: Move velocity integration from `Actor::update()` to `CollisionSystem::update()` ensuring single integration path where velocities are updated before positions.
- **Invisible Entity Optimization**: Skip invisible entities in collision detection to improve performance.
- **MAX_STEPS_NORMAL Reduction**: Reduce `MAX_STEPS_NORMAL` from 2 to 1 for ESP32-C3 stability while maintaining simulation accuracy.
- **Unit Test Updates**: Adjust unit tests to reflect new integration flow and updated physics behavior.

### ⚡ API & Architecture

- **TFT_eSPI Configuration**: Add new configuration options `PIXELROOT32_TFT_ESPI_LINES_PER_BLOCK` and `PIXELROOT32_TFT_ESPI_LINES_PER_BLOCK_FALLBACK` to optimize DMA line transfers.
- **Tilemap Optimization Removal**: Delete `TileCache` and `ChunkManager` classes to streamline graphics rendering, updating documentation to emphasize the `StaticTilemapLayerCache` approach instead.
- **Physics Example Config**: Update physics example configs and fix namespace usage for consistency.

### 📚 Documentation & QA

- **Audio Architecture Updates**: Update documentation to reflect noise implementation changes, remaining limitations, and audio architecture refinements.
- **API Documentation**: Update API references to clarify `StaticTilemapLayerCache` usage and improve examples for dynamic and static layer handling.
- **Refined Comments**: Update comments in `AnimatedTilemapScene` to accurately reflect caching behavior and animation management.

## 1.2.1

### 🏀 Physics

- **Fixed Timestep Scheduler**: Implement `PhysicsScheduler` using a time accumulator pattern to keep physics simulation stable at 60Hz regardless of frame rate fluctuations. Improves consistency on ESP32 where WiFi/BT interrupts can cause uneven frame pacing.
- **Scene Integration Update**: Replace direct `CollisionSystem::update()` calls in `Scene` with the new scheduler-based physics update flow.
- **Physics Optimizations**: Add adaptive step limiting (2 normal steps, 4 catch-up steps), velocity clamping, damping support, and fast reciprocal square root optimizations for more stable and efficient simulation.

### 🎮 Examples

- **Space Invaders Example**: Add a complete Space Invaders clone showcasing discrete grid-based player movement with `KinematicActor`, alien formation movement with step-based animation, projectile object pooling with `RigidActor`, bunker defense using `StaticActor`, swept-circle collision detection, procedural audio, and multi-platform support for native/SDL2 and ESP32/TFT_eSPI.
- **Brick Breaker Example**: Add a full breakout-style sample with paddle, ball, destructible bricks, layer-based collision filtering, audio synthesis with SFX and background music via `MusicPlayer`, particle effects, starfield and explosion visuals, HUD for score and lives, and multi-platform support for native/SDL2 and ESP32 (ST7789 + I2S/DAC).
- **Runtime Allocation Improvements**: Use object pooling in gameplay systems such as projectiles and bricks to avoid runtime heap allocations in sample games.

### ⚡ Architecture & Build

- **Fixed Physics by Default**: Update build flags across all profiles to enable the fixed timestep scheduler by default.

### 📚 Documentation & QA

- **Documentation Expansion**: Extend engine and example documentation to cover the new fixed timestep workflow and sample game setups.
- **Unit Test Coverage**: Add comprehensive unit tests for the fixed timestep scheduler and related physics behavior.

## 1.2.0

### ⚡ Architecture & Build

- **Refactor Physics Conditionals**: Switch from compile-time `constexpr` checks to preprocessor macros for physics module conditionals to allow runtime configuration. Enables optional inclusion of physics functionality at build time.
- **Namespace Cleanup**: Replace `using namespace` directives with namespace aliases and selective `using` declarations. Update style guide with detailed namespace usage rules and examples while maintaining code clarity.

### 🎨 Graphics & Animations

- **ILI9341 Display Support**: Add ILI9341 display type enum values and integrate into display factory. Conditionally compile mock SDL2 events only for native platform.
- **Static Tilemap Layer Cache**: Introduce `StaticTilemapLayerCache` to snapshot logical framebuffer for static 4bpp tilemap layers, enabling fast path rendering on ESP32 with `TFT_eSPI_Drawer`. Reduces per-frame redraw overhead by restoring static layers via `memcpy` and only redrawing dynamic layers when camera is stable. Add `PIXELROOT32_ENABLE_STATIC_TILEMAP_FB_CACHE` build flag (default enabled).
- **Tile Animation Config Fix**: Fix tile animation enable flag to properly respect the `PIXELROOT32_ENABLE_TILE_ANIMATIONS` define, defaulting to false when undefined.

### 🔢 Math

- **Deterministic PRNG System**: Implement global PRNG functions (`rand01`, `rand_int`, `rand_chance`, etc.) with Xorshift32 algorithm. Add thread-safe `Random` struct for independent RNG instances with bias-free integer generation using rejection sampling. Optimize `Fixed16` path to avoid float operations. Include comprehensive test suite covering determinism, distribution, and edge cases.

### 🎮 Input

- **Touch Pipeline & ESP32 CYD Support**: Implement hardware-agnostic touch abstraction (`TouchPoint`, `TouchCalibration`, `XPT2046`/`GT911` adapters) and `TouchManager` polling with fixed-size buffers. Add gesture event system: `TouchStateMachine`, `TouchEventQueue`, `TouchEventDispatcher`, and `TouchEvent` types with consume/propagate semantics (UI-first, then scene). Integrate with `Scene::processTouchEvents` and `onUnconsumedTouchEvent`. Support ESP32-2432S028R (CYD) XPT2046 on GPIO bit-bang SPI (`XPT2046_USE_GPIO_SPI`), optional axis swap/mirror, raw-range mapping, and TFT_eSPI touch bridge path for shared-bus builds. Wire PhysicsDemo and `esp32cyd` platform (calibration before init, debug touch marker under `PIXELROOT32_ENABLE_DEBUG_OVERLAY`).

> [TOUCH_INPUT](docs/architecture/touch-input.md)

### 🎨 UI

- **Function Pointer Callbacks**: Change `UIButton` callback from `std::function<void()>` to `ButtonCallback` (function pointer). Add normalized constructor to `UITouchButton` using `Vector2` parameters. Consolidate `UIElementVoidCallback`, `UIElementBoolCallback` type definitions in `UIElement` to avoid duplication. Remove `<functional>` include to reduce binary size and heap allocations.
- **Touch UI Components**: Add `UITouchButton` with `autoSize` method for dynamic width adjustment, `UITouchCheckbox` for touch-optimized checkbox functionality, and `UITouchSlider` with drag support and value callbacks. Update `UIManager` to manage non-owning pointers for touch widgets.

> **Migration guide v1.1.0 → v1.2.0**: [MIGRATION_v1.2.0](docs/migration/migration-v1-2-0.md)

## 1.1.0

### 🎨 Graphics & Animations

- **Multi-Palette Tilemap Support**: Added functionality for multi-palette tilemaps with new methods to initialize and manage background palette slots. Enables per-cell palette indexing for enhanced visual flexibility.
- **Multi-Palette Sprite Support**: Introduced sprite palette slot bank (similar to background system) for 2bpp/4bpp sprites. New `drawSprite` overloads with `paletteSlot` parameter while maintaining backward compatibility.
- **Tile Animation System**: Implemented `TileAnimation` and `TileAnimationManager` classes for frame-based tile animations (water, lava, etc.) with O(1) frame resolution using PROGMEM animation definitions and fixed-size lookup table. Complies with zero-allocation policy for ESP32.

### 🎮 Physics

- **One-Way Platform Collision**: Implemented spatial crossing detection for one-way platforms using position history tracking and pipeline reordering. Actors can jump through platforms from below.
- **Sensor Support in Kinematics**: Updated `moveAndCollide` to skip sensor actors during kinematic movement while still triggering `onCollision` events.
- **TileCollisionBuilder**: New builder class that creates static or sensor physics actors from tile behavior layers, storing tile coordinates and flags in user data.

### ⚡ Architecture & Build

- **Modular Compilation System**: Added `PIXELROOT32_ENABLE_*` flags for conditional subsystem inclusion (audio, physics, UI, particles), significantly reducing firmware size and RAM usage on embedded platforms.
- **Conditional Logging**: Implemented unified `log()` calls with `PIXELROOT32_DEBUG_MODE` flag. No-op log functions when debug mode is disabled for production builds.
- **PlatformIO Profiles**: Added platformio profiles for different subsystem combinations.

### 📚 Documentation

- **Touch input**: Added [touch-input.md](docs/architecture/touch-input.md) (pipeline, calibration, scene integration). Extended [API Reference](docs/api/index.md) Input module and Scene touch hooks; [Architecture Overview](docs/architecture/overview.md) documents touch as parallel to `InputManager`.

> **Migration guide v1.0.0 → v1.1.0**: [MIGRATION_v1.1.0](docs/migration/migration-v1-1-0.md)

## 1.0.0

First stable release. Complete performance overhaul and API stabilization.

### 🚀 Rendering Performance

- **TFT DMA Pipelining**: Double-buffered pipeline for `TFT_eSPI_Drawer` — CPU processes next block while DMA transmits current one. **~43 FPS** stable on 240×240 displays @ 40MHz (up from ~14 FPS).
- **Fast-Path Kernels**: OLED 2x bit-expansion LUT (U8G2); TFT row duplication with 32-bit native access and `memcpy` for vertical scaling.
- **I2C 1MHz**: Official support in `DisplayConfig` for sustained **60 FPS** on OLED (SSD1306/SH1106).

### 🎮 Physics (Flat Solver 1.0)

- **Broadphase**: Uniform grid (32px cells) with static shared buffers to reduce DRAM usage.
- **KinematicActor**: Rewrote `moveAndSlide` and `moveAndCollide` with binary search, wall sliding, and accurate collision normal detection.
- **Stable stacking**: Baumgarte correction, iterative position relaxation, fixed timestep 1/60s.
- **Godot-style API**: `KinematicCollision`, actor types `Static`/`Kinematic`/`Rigid`. Renamed `PHYSICS_RELAXATION_ITERATIONS` → `VELOCITY_ITERATIONS`.

### 🔢 Math System (Scalar / Fixed-Point)

- Numeric abstraction layer: `Scalar` = `float` on ESP32-S3 (FPU) or `Fixed16` (Q16.16) on C3/S2/C6.
- `Vector2`, `Rect`, and physics unified under `Scalar`. ~30% FPS gain on C3/S2 by eliminating software float emulation.
- `MathUtil`: `fixed_sqrt`, `fixed_sin`, `fixed_cos`, `toScalar()`.

### 🛠️ Other

- **Memory**: Explicit `MALLOC_CAP_DMA` support in drivers; broadphase buffer reuse across frames.
- **C++17**: Migrated from C++11.

> **Migration guide v0.8.1-dev → v1.0.0**: [MIGRATION_v1.0.0](docs/migration/migration-v1-0-0.md)

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
  - Comprehensive documentation added in `README.md` and `docs/architecture/resolution-scaling.md`.
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
- **ESP32 Optimizations**: Applied `IRAM_ATTR` to critical rendering functions (`drawPixel`, `drawSpriteInternal`, `resolveColor`, `drawTileMap`) so they execute from internal RAM on ESP32, bypassing slower flash access for improved performance. Documentation in [architecture/overview.md](docs/architecture/overview.md) and [guide/performance/esp32-performance.md](docs/guide/performance/esp32-performance.md) updated to reflect these optimizations.
- **Optional FPS Overlay**: Introduced build flag `PIXELROOT32_ENABLE_FPS_DISPLAY` to enable an on-screen FPS counter in the top-right corner. FPS is calculated by averaging frame times over a defined interval and updates every 8 frames to reduce CPU load. Refined FPS calculation and initialization for more stable readings.
- **Documentation**: Documented how to override `MAX_LAYERS` and `MAX_ENTITIES` defaults via compiler flags in README.md and [API Reference](docs/api/index.md). `Scene.h` now provides default definitions only when not already defined, and `Scene.cpp` uses the `MAX_LAYERS` constant so user overrides are respected.

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
