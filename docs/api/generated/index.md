# API Reference (Generated)

Auto-generated API documentation from C++ header files.

The `apu` module documents PixelRoot32-APU `2.0.0`.

## Apu

- [ApuCore](./apu/ApuCore.md) — Shared NES-style APU core used by every AudioScheduler.
- [AudioChannel](./apu/AudioChannel.md) — Represents the internal state of a single audio channel.
- [AudioCommand](./apu/AudioCommand.md) — Internal command to communicate between game and audio threads.
- [AudioCommandQueue](./apu/AudioCommandQueue.md) — Single-Producer Single-Consumer (SPSC) lock-free ring buffer for AudioCommands.
- [AudioEvent](./apu/AudioEvent.md) — A fire-and-forget sound event triggered by the game.
- [EnvelopeState](./apu/EnvelopeState.md) — Holds ADSR envelope state for a single voice.
- [InstrumentPreset](./apu/InstrumentPreset.md) — Defines instrument characteristics for playback.
- [LfoState](./apu/LfoState.md) — Holds LFO (Low-Frequency Oscillator) state for pitch or volume modulation.
- [MusicNote](./apu/MusicNote.md) — Represents a single note in a melody.
- [NesEnvelope](./apu/NesEnvelope.md) — NES APU envelope unit state for PULSE 1, PULSE 2, and NOISE.
- [NesFrameCounter](./apu/NesFrameCounter.md) — NES APU frame counter (sequencer) state.
- [NesLengthCounter](./apu/NesLengthCounter.md) — NES APU length counter state for a single voice.
- [NesLinearCounter](./apu/NesLinearCounter.md) — NES APU linear counter state for the TRIANGLE channel.
- [NesSweepUnit](./apu/NesSweepUnit.md) — NES APU sweep unit state for the PULSE channels.
- [SfxBreakpoint](./apu/SfxBreakpoint.md) — Timed automation point for SFX duty steps or pitch envelope.
- [VoiceNesOptions](./apu/VoiceNesOptions.md) — Every per-voice NES opt-in in one place (Hito 4 M14).

## Audio

- [AudioBackend](./audio/AudioBackend.md) — Abstract interface for platform-specific audio drivers.
- [AudioConfig](./audio/AudioConfig.md) — Configuration for the Audio subsystem.
- [AudioEngine](./audio/AudioEngine.md) — Facade class for the NES-style audio subsystem.
- [AudioScheduler](./audio/AudioScheduler.md) — Abstract interface for the audio execution context.
- [DefaultAudioScheduler](./audio/DefaultAudioScheduler.md) — Backend-driven scheduler used on platforms without a dedicated audio task.
- [ImmediateSfxDelayScheduler](./audio/ImmediateSfxDelayScheduler.md) — Plays every scheduled event immediately (ignores delay). Test / stub only.
- [MusicPlayer](./audio/MusicPlayer.md) — Simple sequencer to play MusicTracks using the AudioEngine.
- [NullSfxDelayScheduler](./audio/NullSfxDelayScheduler.md) — No-op scheduler for banks that only use simultaneous layers (no delayed steps).
- [SfxDelayScheduler](./audio/SfxDelayScheduler.md) — Schedules delayed SFX events for `playSfxBank` sequence steps.

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
- [SceneArena](./core/SceneArena.md) — Bump allocator for objects whose lifetime is one scene.
- [SceneManager](./core/SceneManager.md) — Manages the stack of active scenes.
- [TransitionState](./core/TransitionState.md) — State machine for scene transitions.
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

## Gameplay

- [GameplayEvent](./gameplay/GameplayEvent.md) — Fixed-size POD carried by the GameplayEventBus.
- [GameplayEventBus](./gameplay/GameplayEventBus.md) — Fixed-capacity FIFO ring buffer for GameplayEvent, single-instance and Engine-owned.
- [GameplayEventType](./gameplay/GameplayEventType.md) — Tag identifying the meaning of a GameplayEvent.
- [GridMotion](./gameplay/GridMotion.md) — Plain five-`int` aggregate: logical cell, target cell, progress.
- [GridSpec](./gameplay/GridSpec.md) — Plain six-`int` aggregate describing a grid's origin, per-axis cell
       size, and extent (columns/rows). No member functions, no
       per-instance runtime state beyond these fields — a `constexpr
       GridSpec` costs zero SRAM.
- [InteractionComponent](./gameplay/InteractionComponent.md) — Opt-in interaction hooks for a game actor — composition, not inheritance.
- [InteractionTracker](./gameplay/InteractionTracker.md) — Detects enter/exit edges on the per-frame physics contact set and
       dispatches InteractionComponent callbacks (design.md D3).
- [ObjectPool](./gameplay/ObjectPool.md) — Fixed-capacity, zero-heap slot pool over aligned raw storage.
- [Room](./gameplay/Room.md) — POD describing a single room: camera rect + optional tile window
       + fixed-size connection list.
- [RoomData](./gameplay/RoomData.md) — One exported room: its tile-space rect plus four connection slots.
- [RoomDir](./gameplay/RoomDir.md) — Cardinal direction enum for room connections.
- [RoomGraph](./gameplay/RoomGraph.md) — Fixed-capacity graph of rooms with camera rects and connections.
- [RoomGraphBase](./gameplay/RoomGraphBase.md) — Abstract base class for RoomGraph&lt;N> used by Scene via type erasure.
- [RoomLayer](./gameplay/RoomLayer.md) — Runtime view over an exported room array — the room-graph analogue
       of `physics::TileBehaviorLayer`.
- [StateMachine](./gameplay/StateMachine.md) — Non-template finite state machine over a caller-owned, `const` state table.

## Graphics

- [Anchor](./graphics/Anchor.md) — Defines anchor points for positioning UI elements.
- [BaseDrawSurface](./graphics/BaseDrawSurface.md) — Optional base class for DrawSurface implementations providing default primitive rendering.
- [Camera2D](./graphics/Camera2D.md) — 2D camera for viewport management and smooth scrolling.
- [CameraBounds](./graphics/CameraBounds.md) — Closed-interval camera-position range produced by `cameraRangeFor`.
- [CameraEffectsSystem](./graphics/CameraEffectsSystem.md) — Manages up to 4 simultaneous camera effects with round-robin insertion.
- [CameraTween](./graphics/CameraTween.md) — Fixed-capacity camera tween pool with enum-based easing.
- [Color](./graphics/Color.md) — Named color indices into the active 16-entry palette.
- [DirtyGrid](./graphics/DirtyGrid.md) — Two-buffer dirty cell grid (8×8 px cells) for selective framebuffer clears.
- [DisplayConfig](./graphics/DisplayConfig.md) — Configuration settings for initializing displays with optional resolution scaling.
- [DisplayType](./graphics/DisplayType.md) — Identifies the type of display driver to use.
- [DrawSurface](./graphics/DrawSurface.md) — Abstract interface for platform-specific drawing operations.
- [EffectSlot](./graphics/EffectSlot.md) — Per-slot state for a single camera effect (20 bytes).
- [Font](./graphics/Font.md) — Descriptor for a bitmap font using 1bpp sprites.
- [FontManager](./graphics/FontManager.md) — Static utility class for managing bitmap fonts.
- [LayerAttributes](./graphics/LayerAttributes.md) — All tiles with attributes in a single tilemap layer.
- [LayerType](./graphics/LayerType.md) — Classifies draw layers for dirty-region marking (static backgrounds vs dynamic content).
- [MultiSprite](./graphics/MultiSprite.md) — Multi-layer, multi-color sprite built from 1bpp layers.
- [PaletteContext](./graphics/PaletteContext.md) — Context for palette selection in dual palette mode.
- [PaletteType](./graphics/PaletteType.md) — Selects which built-in 16-color palette the renderer resolves against.
- [Particle](./graphics/Particle.md) — Represents a single particle in the particle system.
- [ParticleConfig](./graphics/ParticleConfig.md) — Configuration parameters for a particle emitter.
- [ParticleEmitter](./graphics/ParticleEmitter.md) — Manages a pool of particles to create visual effects.
- [Renderer](./graphics/Renderer.md) — High-level graphics rendering system.
- [ResolutionPreset](./graphics/ResolutionPreset.md) — Logical resolution choices for memory-constrained targets.
- [ResolutionPresets](./graphics/ResolutionPresets.md) — Factory for creating DisplayConfig from resolution presets.
- [ScreenBounds](./graphics/ScreenBounds.md) — Half-open screen-space bounding box accumulated across one or more
       `expandProjectedMapBounds` calls.
- [ScrollBehavior](./graphics/ScrollBehavior.md) — Defines how scrolling behaves in layouts.
- [Sprite](./graphics/Sprite.md) — Compact sprite descriptor for monochrome bitmapped sprites.
- [Sprite2bpp](./graphics/Sprite2bpp.md) — Sprite descriptor for 2bpp (4-color) multi-color sprites.
- [Sprite4bpp](./graphics/Sprite4bpp.md) — Sprite descriptor for 4bpp (16-color) multi-color sprites.
- [SpriteAnimation](./graphics/SpriteAnimation.md) — Lightweight, step-based sprite animation controller.
- [SpriteAnimationFrame](./graphics/SpriteAnimationFrame.md) — Single animation frame that can reference either a Sprite or a MultiSprite.
- [SpriteLayer](./graphics/SpriteLayer.md) — Single monochrome layer used by layered sprites.
- [StaticLayerSnapshot](./graphics/StaticLayerSnapshot.md) — Framebuffer cache for static layers a game draws ITSELF.
- [StaticTilemapLayerCache](./graphics/StaticTilemapLayerCache.md) — Centralized framebuffer snapshot for static 4bpp tilemap layers.
- [TileAnimation](./graphics/TileAnimation.md) — Single tile animation definition (compile-time constant).
- [TileAnimationManager](./graphics/TileAnimationManager.md) — Manages tile animations for a tilemap.
- [TileAttribute](./graphics/TileAttribute.md) — Single attribute key-value pair for tile metadata.
- [TileAttributeEntry](./graphics/TileAttributeEntry.md) — All attributes for a single tile at a specific position.
- [TileMap4bppDrawSpec](./graphics/TileMap4bppDrawSpec.md) — One drawable 4bpp tilemap layer with an origin in logical coordinates.
- [TileMapGeneric](./graphics/TileMapGeneric.md) — Generic tilemap structure supporting 1bpp, 2bpp, or 4bpp tile graphics.
- [TilemapSpriteDirtyMode](./graphics/TilemapSpriteDirtyMode.md) — Suppress per-sprite dirty marks while drawing tilemaps (static layer or selective animated marking).
- [TouchConfig](./graphics/TouchConfig.md) — Configuration for a touch controller (XPT2046 or GT911).
- [TouchController](./graphics/TouchController.md) — Supported touch controller types.
- [TransitionDirection](./graphics/TransitionDirection.md) — Direction of the transition effect.
- [TransitionEffect](./graphics/TransitionEffect.md) — Manages a single scene transition with zero runtime allocation.
- [TransitionType](./graphics/TransitionType.md) — Types of scene transitions.
- [TweenEasing](./graphics/TweenEasing.md) — Easing curves for camera tween interpolation.
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
- [UISprite](./graphics/UISprite.md) — A UI leaf that draws a single sprite.
- [UISpriteFormat](./graphics/UISpriteFormat.md) — Which member of a UISpriteRef's storage is live.
- [UISpriteRef](./graphics/UISpriteRef.md) — Non-owning, format-tagged pointer to one sprite plus its draw
       parameters.
- [UISpriteRow](./graphics/UISpriteRow.md) — A UI leaf that draws a row of repeated icons whose fill is driven by
       one value — hearts, lives, keys, ammo.
- [UITouchButton](./graphics/UITouchButton.md) — Touch-optimized button widget.
- [UITouchCheckbox](./graphics/UITouchCheckbox.md) — Touch-optimized checkbox widget.
- [UITouchElement](./graphics/UITouchElement.md) — UIElement with embedded UITouchWidget data for touch interaction.
- [UITouchSlider](./graphics/UITouchSlider.md) — Touch-optimized slider widget.
- [UITouchWidget](./graphics/UITouchWidget.md) — Base touch widget structure
- [UIVerticalLayout](./graphics/UIVerticalLayout.md) — Vertical layout container with scroll support.
- [UIWidgetFlags](./graphics/UIWidgetFlags.md) — Flags for widget behavior
- [UIWidgetState](./graphics/UIWidgetState.md) — Current state of a touch widget
- [UIWidgetType](./graphics/UIWidgetType.md) — Types of touch UI widgets
- [WipeDirection](./graphics/WipeDirection.md) — Corner-to-corner directions for DiagonalWipe transitions.

## Input

- [ActorPool](./input/ActorPool.md) — Fixed-size pool for managing draggable actors
- [ActorTouchController](./input/ActorTouchController.md) — Handles touch-based dragging of game actors
- [DisplayPreset](./input/DisplayPreset.md) — Display presets for common displays
- [GT911Adapter](./input/GT911Adapter.md) — GT911 I2C touch controller driver
- [InputConfig](./input/InputConfig.md) — Configuration structure for the InputManager.
- [InputManager](./input/InputManager.md) — Handles input from physical buttons, keyboard (on PC), and touch/mouse.
- [TouchAdapter](./input/TouchAdapter.md) — Base class requirements for touch adapters (conceptual)
- [TouchCalibration](./input/TouchCalibration.md) — Calibration parameters for coordinate transformation
- [TouchController](./input/TouchController.md) — Touch controller types
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

- [CellRange](./math/CellRange.md) — Half-open cell-space window `[startCol, endCol) x [startRow, endRow)`
       covering a screen rectangle, under a given ProjectionSpec.
- [Fixed16](./math/Fixed16.md) — Fixed-point 16.16 number implementation optimized for RISC-V.
- [ProjectionSpec](./math/ProjectionSpec.md) — Plain six-`int` aggregate: the screen anchor of cell (0, 0) plus the
       two screen-space axis vectors of the cell grid.
- [Random](./math/Random.md) — Instance-based random number generator
- [Vector2](./math/Vector2.md) — 2D vector using the configured Scalar type (float or Fixed16).

## Physics

- [Circle](./physics/Circle.md) — Represents a 2D circle for collision detection.
- [CollisionSystem](./physics/CollisionSystem.md) — Manages physics simulation and collision detection for all actors.
- [Contact](./physics/Contact.md) — Represents a contact point between two physics bodies.
- [KinematicActor](./physics/KinematicActor.md) — A physics body moved via script/manual velocity with collision detection.
- [KinematicCollision](./physics/KinematicCollision.md) — Contains information about a collision involving a KinematicActor.
- [PhysicsScheduler](./physics/PhysicsScheduler.md) — Fixed-timestep accumulator that decouples physics from frame rate.
- [RigidActor](./physics/RigidActor.md) — A physics body fully simulated by the engine.
- [Segment](./physics/Segment.md) — Represents a 2D line segment for collision detection.
- [SensorActor](./physics/SensorActor.md) — A static body that acts as a trigger: detects overlap but produces no physical response.
- [SnapPolicy](./physics/SnapPolicy.md) — Controls floor snap behavior after slide resolution.
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
