# API Reference (Generated)

Auto-generated API documentation from C++ header files.

::: warning Generated Content
These files are automatically generated from header comments.
Do not edit manually. Run `python scripts/generate_api_docs.py` to regenerate.
:::

## Audio

- [ApuCore](./audio/ApuCore.md) — Shared NES-style APU core used by every AudioScheduler.
- [AudioBackend](./audio/AudioBackend.md) — Abstract interface for platform-specific audio drivers.
- [AudioChannel](./audio/AudioChannel.md) — Represents the internal state of a single audio channel.
- [AudioCommand](./audio/AudioCommand.md) — Internal command to communicate between game and audio threads.
- [AudioCommandQueue](./audio/AudioCommandQueue.md) — Multi-Producer Single-Consumer lock-free ring buffer for AudioCommands.
- [AudioConfig](./audio/AudioConfig.md) — Configuration for the Audio subsystem.
- [AudioEngine](./audio/AudioEngine.md) — Core class for the NES-like audio subsystem.
- [AudioEvent](./audio/AudioEvent.md) — A fire-and-forget sound event triggered by the game.
- [AudioScheduler](./audio/AudioScheduler.md) — Abstract interface for the audio execution context.
- [DefaultAudioScheduler](./audio/DefaultAudioScheduler.md) — Backend-driven scheduler used on platforms without a dedicated
       audio task.
- [InstrumentPreset](./audio/InstrumentPreset.md) — Defines instrument characteristics for playback.
- [MusicNote](./audio/MusicNote.md) — Represents a single note in a melody.
- [MusicPlayer](./audio/MusicPlayer.md) — Simple sequencer to play MusicTracks.

## Core

- [Actor](./core/Actor.md) — An Entity capable of physical interaction and collision.
- [CollisionShape](./core/CollisionShape.md) — Defines the geometric shape used for collision detection.
- [Engine](./core/Engine.md) — The main engine class that manages the game loop and core subsystems.
- [Entity](./core/Entity.md) — Abstract base class for all game objects.
- [EntityType](./core/EntityType.md) — Categorizes entities for type-safe casting and logic differentiation.
- [LimitRect](./core/LimitRect.md) — Defines a rectangular boundary for actor movement.
- [LogLevel](./core/LogLevel.md) — Enumeration of log levels.
- [PhysicsActor](./core/PhysicsActor.md) — An actor with basic 2D physics properties using adaptable Scalar type.
- [PhysicsBodyType](./core/PhysicsBodyType.md) — Defines the simulation behavior of a PhysicsActor.
- [Rect](./core/Rect.md) — Represents a 2D rectangle, typically used for hitboxes or bounds.
- [Scene](./core/Scene.md) — Represents a game level or screen containing entities.
- [SceneManager](./core/SceneManager.md) — Manages the stack of active scenes.
- [WorldCollisionInfo](./core/WorldCollisionInfo.md) — Stores flags indicating which world boundaries were hit in the current frame.

## Drivers

- [ESP32AudioScheduler](./drivers/ESP32AudioScheduler.md) — Audio scheduler for ESP32 targets.
- [ESP32_DAC_AudioBackend](./drivers/ESP32_DAC_AudioBackend.md) — Audio backend for ESP32 classic / S2 internal 8-bit DAC.
- [ESP32_I2S_AudioBackend](./drivers/ESP32_I2S_AudioBackend.md) — Audio backend implementation for ESP32 using I2S.
- [NativeAudioScheduler](./drivers/NativeAudioScheduler.md) — Audio scheduler for native builds.
- [SDL2_AudioBackend](./drivers/SDL2_AudioBackend.md) — Audio backend implementation for SDL2 (Windows/Linux/Mac).
- [SDL2_Drawer](./drivers/SDL2_Drawer.md) — SDL2-backed draw surface for native desktop builds.
- [TFT_eSPI_Drawer](./drivers/TFT_eSPI_Drawer.md) — Concrete implementation of DrawSurface for ESP32 using the TFT_eSPI library.
- [U8G2_Drawer](./drivers/U8G2_Drawer.md) — Implementation of DrawSurface using the U8G2 library for monochromatic OLED displays.

## Graphics

- [Anchor](./graphics/Anchor.md) — Defines anchor points for positioning UI elements.
- [BaseDrawSurface](./graphics/BaseDrawSurface.md) — Optional base class for DrawSurface implementations that provides default primitive rendering.
- [Camera2D](./graphics/Camera2D.md) — 2D camera system for managing viewports and scrolling.
- [DirtyGrid](./graphics/DirtyGrid.md) — Two-buffer dirty cell grid (8×8 px cells) for selective framebuffer clears.
- [DisplayType](./graphics/DisplayType.md) — Identifies the type of display driver to use.
- [DrawSurface](./graphics/DrawSurface.md) — Abstract interface for platform-specific drawing operations.
- [Font](./graphics/Font.md) — Descriptor for a bitmap font using 1bpp sprites.
- [FontManager](./graphics/FontManager.md) — Static utility class for managing bitmap fonts.
- [LayerAttributes](./graphics/LayerAttributes.md) — All tiles with attributes in a single tilemap layer.
- [LayerType](./graphics/LayerType.md) — Classifies draw layers for dirty-region marking (static backgrounds vs dynamic content).
- [MultiSprite](./graphics/MultiSprite.md) — Multi-layer, multi-color sprite built from 1bpp layers.
- [Particle](./graphics/Particle.md) — Represents a single particle in the particle system.
- [ParticleConfig](./graphics/ParticleConfig.md) — Configuration parameters for a particle emitter.
- [ParticleEmitter](./graphics/ParticleEmitter.md) — Manages a pool of particles to create visual effects.
- [Renderer](./graphics/Renderer.md) — High-level graphics rendering system.
- [ScrollBehavior](./graphics/ScrollBehavior.md) — Defines how scrolling behaves in layouts.
- [Sprite](./graphics/Sprite.md) — Compact sprite descriptor for monochrome bitmapped sprites.
- [Sprite2bpp](./graphics/Sprite2bpp.md) — Sprite descriptor for 2bpp (4-color) multi-color sprites.
- [Sprite4bpp](./graphics/Sprite4bpp.md) — Sprite descriptor for 4bpp (16-color) multi-color sprites.
- [SpriteAnimation](./graphics/SpriteAnimation.md) — Lightweight, step-based sprite animation controller.
- [SpriteAnimationFrame](./graphics/SpriteAnimationFrame.md) — Single animation frame that can reference either a Sprite or a MultiSprite.
- [SpriteLayer](./graphics/SpriteLayer.md) — Single monochrome layer used by layered sprites.
- [TileAnimation](./graphics/TileAnimation.md) — Single tile animation definition (compile-time constant).
- [TileAnimationManager](./graphics/TileAnimationManager.md) — Manages tile animations for a tilemap.
- [TileAttribute](./graphics/TileAttribute.md) — Single attribute key-value pair for tile metadata.
- [TileAttributeEntry](./graphics/TileAttributeEntry.md) — All attributes for a single tile at a specific position.
- [TileMapGeneric](./graphics/TileMapGeneric.md) — Generic tilemap structure supporting 1bpp, 2bpp, or 4bpp tile graphics.
- [TilemapSpriteDirtyMode](./graphics/TilemapSpriteDirtyMode.md) — Suppress per-sprite dirty marks while drawing tilemaps (static layer or selective animated marking).
- [TouchConfig](./graphics/TouchConfig.md) — Configuration for touch controller
- [TouchController](./graphics/TouchController.md) — Supported touch controller types
- [UIAnchorLayout](./graphics/UIAnchorLayout.md) — Layout that positions elements at fixed anchor points on the screen.
- [UIButton](./graphics/UIButton.md) — A clickable button UI element.
- [UICheckBox](./graphics/UICheckBox.md) — A clickable checkbox UI element.
- [UIElement](./graphics/UIElement.md) — Base class for all user interface elements (buttons, labels, etc.).
- [UIGridLayout](./graphics/UIGridLayout.md) — Grid layout container for organizing elements in a matrix.
- [UIHitTest](./graphics/UIHitTest.md) — AABB hit testing for touch UI widgets
- [UIHorizontalLayout](./graphics/UIHorizontalLayout.md) — Horizontal layout container with scroll support.
- [UILabel](./graphics/UILabel.md) — A simple text label UI element.
- [UILayout](./graphics/UILayout.md) — Base class for UI layout containers.
- [UIManager](./graphics/UIManager.md) — Registry of touch UI elements for event routing (non-owning pointers).
- [UIPaddingContainer](./graphics/UIPaddingContainer.md) — Container that wraps a single UI element and applies padding.
- [UIPanel](./graphics/UIPanel.md) — Visual container that draws a background and border around a child element.
- [UITouchButton](./graphics/UITouchButton.md) — Touch-optimized button widget.
- [UITouchCheckbox](./graphics/UITouchCheckbox.md) — Touch-optimized checkbox widget.
- [UITouchElement](./graphics/UITouchElement.md) — UIElement with embedded UITouchWidget data for touch interaction.
- [UITouchSlider](./graphics/UITouchSlider.md) — Touch-optimized slider widget.
- [UITouchWidget](./graphics/UITouchWidget.md) — Base touch widget structure
- [UIVerticalLayout](./graphics/UIVerticalLayout.md) — Vertical layout container with scroll support.
- [UIWidgetFlags](./graphics/UIWidgetFlags.md) — Flags for widget behavior
- [UIWidgetState](./graphics/UIWidgetState.md) — Current state of a touch widget
- [UIWidgetType](./graphics/UIWidgetType.md) — Types of touch UI widgets

## Input

- [ActorPool](./input/ActorPool.md) — Fixed-size pool for managing draggable actors
- [ActorTouchController](./input/ActorTouchController.md) — Handles touch-based dragging of game actors
- [DisplayPreset](./input/DisplayPreset.md) — Display presets for common displays
- [GT911Adapter](./input/GT911Adapter.md) — GT911 I2C touch controller driver
- [InputConfig](./input/InputConfig.md) — Configuration structure for the InputManager.
- [InputManager](./input/InputManager.md) — Handles input from physical buttons, keyboard (on PC), and touch/mouse.
- [TouchAdapterBase](./input/TouchAdapterBase.md) — Base class requirements for touch adapters (conceptual)
- [TouchCalibration](./input/TouchCalibration.md) — Calibration parameters for coordinate transformation
- [TouchEvent](./input/TouchEvent.md) — Compact touch event structure (12 bytes total, naturally aligned)
- [TouchEventDispatcher](./input/TouchEventDispatcher.md) — Pull-based touch event dispatcher
- [TouchEventFlags](./input/TouchEventFlags.md) — Flags for touch events
- [TouchEventHistory](./input/TouchEventHistory.md) — Ring buffer for touch events (for gesture detection)
- [TouchEventQueue](./input/TouchEventQueue.md) — Ring buffer for touch events (192 bytes total)
- [TouchEventType](./input/TouchEventType.md) — High-level touch event types for gesture detection
- [TouchFactory](./input/TouchFactory.md) — Factory for creating touch system configurations
- [TouchManager](./input/TouchManager.md) — Touch event aggregation layer
- [TouchPoint](./input/TouchPoint.md) — Normalized touch data structure.
- [TouchPointBuffer](./input/TouchPointBuffer.md) — Ring buffer for storing touch points
- [TouchRotation](./input/TouchRotation.md) — Display rotation modes for calibration
- [TouchState](./input/TouchState.md) — Internal states for touch gesture detection
- [TouchStateData](./input/TouchStateData.md) — Per-touch-ID state tracking
- [TouchStateMachine](./input/TouchStateMachine.md) — State machine for touch gesture detection
- [XPT2046Adapter](./input/XPT2046Adapter.md) — XPT2046 SPI touch controller driver

## Math

- [Fixed16](./math/Fixed16.md) — Fixed-point 16.16 number implementation optimized for RISC-V.
- [Vector2](./math/Vector2.md) — 2D vector using the configured Scalar type (float or Fixed16).

## Physics

- [Circle](./physics/Circle.md) — Represents a 2D circle for collision detection.
- [CollisionSystem](./physics/CollisionSystem.md) — Manages physics simulation and collision detection for all actors.
- [Contact](./physics/Contact.md) — Represents a contact point between two physics bodies.
- [KinematicActor](./physics/KinematicActor.md) — A physics body moved via script/manual velocity with collision detection.
- [KinematicCollision](./physics/KinematicCollision.md) — Contains information about a collision involving a KinematicActor.
- [RigidActor](./physics/RigidActor.md) — A physics body fully simulated by the engine.
- [Segment](./physics/Segment.md) — Represents a 2D line segment for collision detection.
- [SensorActor](./physics/SensorActor.md) — A static body that acts as a trigger: detects overlap but produces no physical response.
- [SpatialGrid](./physics/SpatialGrid.md) — Optimized spatial partitioning with separate static/dynamic layers.
- [StaticActor](./physics/StaticActor.md) — A physics body that does not move.
- [TileBehaviorLayer](./physics/TileBehaviorLayer.md) — Runtime representation of exported behavior layer for O(1) flag lookup.
- [TileCollisionBehavior](./physics/TileCollisionBehavior.md) — Defines how a tile collider behaves in the physics system.
- [TileCollisionBuilder](./physics/TileCollisionBuilder.md) — Helper class for creating physics bodies from exported behavior layers.
- [TileCollisionBuilderConfig](./physics/TileCollisionBuilderConfig.md) — Configuration for tile collision building.
- [TileConsumptionConfig](./physics/TileConsumptionConfig.md) — Configuration for tile consumption operations.
- [TileConsumptionHelper](./physics/TileConsumptionHelper.md) — Helper class for consuming tiles (removing bodies and updating visuals).
- [TileFlags](./physics/TileFlags.md) — Bit flags for tile behavior attributes (8-bit, 1 byte per tile).
Optimized for ESP32 runtime with bit operations only.

## Platforms

- [MockAudioBackend](./platforms/MockAudioBackend.md) — Mock implementation of AudioBackend for unit testing.
- [PlatformCapabilities](./platforms/PlatformCapabilities.md) — Represents the hardware capabilities of the current platform.
- [SPIClass](./platforms/SPIClass.md) — Mocks the Arduino SPI class for native platform.

## Test

- [PhysicsSnapshot](./test/PhysicsSnapshot.md) — Captures physics state for determinism validation
- [PhysicsTestSuite](./test/PhysicsTestSuite.md) — Comprehensive testing for Flat Solver
- [StressTestScene](./test/StressTestScene.md) — Scene for stress testing physics performance.
