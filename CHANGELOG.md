# Changelog

All notable changes to this project will be documented in this file.

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
