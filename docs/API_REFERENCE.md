# API Reference

This document provides a complete reference for the PixelRoot32 Game Engine public API.

## Global Configuration

The engine's behavior can be customized using `platforms/PlatformDefaults.h` and `platforms/EngineConfig.h`, or via compile-time build flags. This allows for fine-tuning performance and hardware support without modifying the core engine code.

### Platform Macros (Build Flags)

| Macro | Description | Default (ESP32) |
|-------|-------------|-----------------|
| `PR32_DEFAULT_AUDIO_CORE` | CPU core assigned to audio tasks. | `0` |
| `PR32_DEFAULT_MAIN_CORE` | CPU core assigned to the main game loop. | `1` |
| `PIXELROOT32_NO_DAC_AUDIO` | Disable Internal DAC support on classic ESP32. | Enabled |
| `PIXELROOT32_NO_I2S_AUDIO` | Disable I2S audio support. | Enabled |
| `PIXELROOT32_USE_U8G2` | Enable future U8G2 display driver support. | Disabled |
| `PIXELROOT32_NO_TFT_ESPI` | Disable default TFT_eSPI driver support. | Enabled |

### Constants

- **`DISPLAY_WIDTH`**
    The width of the display in pixels. Default is `240`.

- **`DISPLAY_HEIGHT`**
    The height of the display in pixels. Default is `240`.

## Core Module

The Core module provides the fundamental building blocks of the engine, including the main application loop, entity management, and scene organization.

### Engine

**Inherits:** None

The main engine class that manages the game loop and core subsystems. `Engine` acts as the central hub, initializing and managing the Renderer, InputManager, and SceneManager. It runs the main game loop, handling timing (delta time), updating the current scene, and rendering frames.

#### Public Methods

- **`Engine(const DisplayConfig& displayConfig, const InputConfig& inputConfig, const AudioConfig& audioConfig)`**
    Constructs the engine with custom display, input and audio configurations.

- **`Engine(const DisplayConfig& displayConfig, const InputConfig& inputConfig)`**
    Constructs the engine with custom display and input configurations.

- **`Engine(const DisplayConfig& displayConfig)`**
    Constructs the engine with custom display configuration and default input settings.

- **`void init()`**
    Initializes the engine subsystems. Must be called before `run()`.

- **`void run()`**
    Starts the main game loop. This method contains the infinite loop that calls `update()` and `draw()` repeatedly.

- **`unsigned long getDeltaTime() const`**
    Returns the time elapsed since the last frame in milliseconds.

- **`void setScene(Scene* newScene)`**
    Sets the current active scene.

- **`Scene* getCurrentScene() const`**
    Retrieves the currently active scene.

- **`Renderer& getRenderer()`**
    Provides access to the Renderer subsystem.

- **`InputManager& getInputManager()`**
    Provides access to the InputManager subsystem.

- **`AudioEngine& getAudioEngine()`**
    Provides access to the AudioEngine subsystem.

- **`const PlatformCapabilities& getPlatformCapabilities() const`**
    Returns the detected hardware capabilities for the current platform.

### PlatformCapabilities (Struct)

A structure that holds detected hardware capabilities, used to optimize task pinning and threading.

- **`bool hasDualCore`**: True if the hardware has more than one CPU core.
- **`int coreCount`**: Total number of CPU cores detected.
- **`int audioCoreId`**: Recommended CPU core for audio tasks.
- **`int mainCoreId`**: Recommended CPU core for the main game loop.
- **`int audioPriority`**: Recommended priority for audio tasks.

Static Methods:
- **`static PlatformCapabilities detect()`**: Automatically detects hardware capabilities based on the platform and configuration. It respects the defaults defined in `platforms/PlatformDefaults.h` and any compile-time overrides.

#### Optional: Debug Statistics Overlay (build flag)

When the engine is built with the preprocessor define **`PIXELROOT32_ENABLE_DEBUG_OVERLAY`**, the engine draws a technical overlay with real-time metrics.

- **Metrics Included**:
    - **FPS**: Frames per second (green).
    - **RAM**: Memory used in KB (cyan). ESP32 specific.
    - **CPU**: Estimated processor load percentage based on frame processing time (yellow).
- **Behavior**: The metrics are drawn in the top-right area of the screen, fixed and independent of the camera.
- **Performance**: Values are recalculated and formatted only every **16 frames** (`DEBUG_UPDATE_INTERVAL`); the cached strings are drawn every frame. This ensures minimal overhead while providing useful development data.
- **Usage**: Add to your build flags, e.g. in `platformio.ini`:  
  `build_flags = -D PIXELROOT32_ENABLE_DEBUG_OVERLAY`  
  This flag is also available in `EngineConfig.h`.
- **Internal**: Implemented by the private method `Engine::drawDebugOverlay(Renderer& r)`.

---

## Audio Module

The Audio module provides a NES-like audio system with Pulse, Triangle, and Noise
channels, plus a lightweight melody subsystem for background music.

### AudioEngine

**Inherits:** None

The core class managing audio generation and playback.

#### Public Methods

- **`AudioEngine(const AudioConfig& config)`**
    Constructs the AudioEngine.

- **`void init()`**
    Initializes the audio backend.

- **`void generateSamples(int16_t* stream, int length)`**
    Fills the buffer with audio samples.

- **`void playEvent(const AudioEvent& event)`**
    Plays a one-shot audio event on the first available channel of the requested type.

Typical usage from a scene:

```cpp
auto& audio = engine.getAudioEngine();

AudioEvent evt{};
evt.type = WaveType::PULSE;
evt.frequency = 1500.0f;
evt.duration = 0.12f;
evt.volume = 0.8f;
evt.duty = 0.5f;

audio.playEvent(evt);
```

### Data Structures

#### WaveType (Enum)

- `PULSE`: Square wave with variable duty cycle.
- `TRIANGLE`: Triangle wave (fixed volume/duty).
- `NOISE`: Pseudo-random noise.

#### AudioEvent (Struct)

Structure defining a sound effect to be played.

- **`WaveType type`**: Type of waveform to use.
- **`float frequency`**: Frequency in Hz.
- **`float duration`**: Duration in seconds.
- **`float volume`**: Volume level (0.0 to 1.0).
- **`float duty`**: Duty cycle for Pulse waves (0.0 to 1.0, typically 0.125, 0.25, 0.5, 0.75).

#### Note (Enum)

Defined in `AudioMusicTypes.h`. Represents the 12 semitones plus a rest:

- `C`, `Cs`, `D`, `Ds`, `E`, `F`, `Fs`, `G`, `Gs`, `A`, `As`, `B`, `Rest`

Use `noteToFrequency(Note note, int octave)` to convert a note and octave to Hz.

#### MusicNote (Struct)

Represents a single musical note in a melody.

- **`Note note`**: Musical note (C, D, E, etc. or Rest).
- **`uint8_t octave`**: Octave index (0–8).
- **`float duration`**: Duration in seconds.
- **`float volume`**: Volume level (0.0 to 1.0).

#### MusicTrack (Struct)

Represents a sequence of notes to be played as a track.

- **`const MusicNote* notes`**: Pointer to an array of notes.
- **`size_t count`**: Number of notes in the array.
- **`bool loop`**: If true, the track loops when it reaches the end.
- **`WaveType channelType`**: Which channel type to use (typically `PULSE`).
- **`float duty`**: Duty cycle for Pulse tracks.

#### InstrumentPreset (Struct)

Simple preset describing a reusable “instrument”:

- **`float baseVolume`**: Default volume for notes.
- **`float duty`**: Duty cycle suggestion (for Pulse).
- **`uint8_t defaultOctave`**: Default octave for the instrument.

Predefined presets:

- `INSTR_PULSE_LEAD` – main lead pulse in octave 4.
- `INSTR_PULSE_BASS` – bass pulse in octave 3.
- `INSTR_PULSE_CHIP_HIGH` – high-pitched chiptune pulse in octave 5.
- `INSTR_TRIANGLE_PAD` – soft triangle pad in octave 4.

Helper functions:

- **`MusicNote makeNote(const InstrumentPreset& preset, Note note, float duration)`**
- **`MusicNote makeNote(const InstrumentPreset& preset, Note note, uint8_t octave, float duration)`**
- **`MusicNote makeRest(float duration)`**

These helpers reduce boilerplate when defining melodies and keep instruments consistent.

### MusicPlayer

**Inherits:** None

High-level sequencer for playing `MusicTrack` instances as background music.

#### Public Methods

- **`MusicPlayer(AudioEngine& engine)`**
    Constructs a MusicPlayer bound to an existing `AudioEngine`.

- **`void play(const MusicTrack& track)`**
    Starts playing the given track from the beginning.

- **`void stop()`**
    Stops playback of the current track.

- **`void pause()`**
    Pauses playback (time does not advance).

- **`void resume()`**
    Resumes playback after pause.

- **`void update(unsigned long deltaTime)`**
    Advances the internal timer and moves to the next note when its duration
    expires. Intended to be called once per frame from the main `Engine`.

- **`bool isPlaying() const`**
    Returns true if a track is currently playing and not finished.

Typical usage from a scene:

```cpp
using namespace pixelroot32::audio;

static const MusicNote MELODY[] = {
    makeNote(INSTR_PULSE_LEAD, Note::C, 0.20f),
    makeNote(INSTR_PULSE_LEAD, Note::E, 0.20f),
    makeNote(INSTR_PULSE_LEAD, Note::G, 0.25f),
    makeRest(0.10f),
};

static const MusicTrack GAME_MUSIC = {
    MELODY,
    sizeof(MELODY) / sizeof(MusicNote),
    true,
    WaveType::PULSE,
    0.5f
};

void MyScene::init() {
    engine.getMusicPlayer().play(GAME_MUSIC);
}
```

### AudioConfig

Configuration struct for the audio system.

- **`AudioBackend* backend`**: Pointer to the platform-specific audio backend (e.g., SDL2, I2S).
- **`int sampleRate`**: Audio sample rate in Hz (default: 22050).

---

### Entity

**Inherits:** None

Abstract base class for all game objects. Entities are the fundamental building blocks of the scene. They have a position, size, and lifecycle methods (`update`, `draw`).

#### Properties

- **`float x, y`**: Position in world space.
- **`int width, height`**: Dimensions of the entity.
- **`bool isVisible`**: If false, the entity is skipped during rendering.
- **`bool isEnabled`**: If false, the entity is skipped during updates.
- **`unsigned char renderLayer`**: Logical render layer for this entity (0 = background, 1 = gameplay, 2 = UI).

#### Public Methods

- **`Entity(float x, float y, int w, int h, EntityType t)`**
    Constructs a new Entity.

- **`void setVisible(bool v)`**
    Sets the visibility of the entity.

- **`void setEnabled(bool e)`**
    Sets the enabled state of the entity.

- **`unsigned char getRenderLayer() const`**
    Returns the current render layer.

- **`void setRenderLayer(unsigned char layer)`**
    Sets the logical render layer for this entity.

- **`virtual void update(unsigned long deltaTime)`**
    Updates the entity's logic. Must be overridden by subclasses.

- **`virtual void draw(Renderer& renderer)`**
    Renders the entity. Must be overridden by subclasses.

---

### Actor

**Inherits:** [Entity](#entity)

An Entity capable of physical interaction and collision. Actors extend Entity with collision layers and masks. They are used for dynamic game objects like players, enemies, and projectiles.

#### Public Methods

- **`Actor(float x, float y, int w, int h)`**
    Constructs a new Actor.

- **`void setCollisionLayer(CollisionLayer l)`**
    Sets the collision layer this actor belongs to.

- **`void setCollisionMask(CollisionLayer m)`**
    Sets the collision layers this actor interacts with.

- **`bool isInLayer(uint16_t targetLayer) const`**
    Checks if the Actor belongs to a specific collision layer.

- **`virtual Rect getHitBox()`**
    Gets the hitbox for collision detection. Must be implemented by subclasses.

- **`virtual void onCollision(Actor* other)`**
    Callback invoked when a collision occurs with another Actor.

---

### Scene

**Inherits:** None

Represents a game level or screen containing entities. A Scene manages a collection of Entities and a CollisionSystem. It is responsible for updating and drawing all entities it contains.

#### Public Methods

- **`virtual void init()`**
    Called when the scene is initialized or entered.

- **`virtual void update(unsigned long deltaTime)`**
    Updates all entities in the scene and handles collisions.

- **`virtual void draw(Renderer& renderer)`**
    Draws all visible entities in the scene, iterating them by logical render layers (0 = background, 1 = gameplay, 2 = UI).

- **`void addEntity(Entity* entity)`**
    Adds an entity to the scene.

- **`void removeEntity(Entity* entity)`**
    Removes an entity from the scene.

- **`void clearEntities()`**
    Removes all entities from the scene.

#### Overriding scene limits (MAX_LAYERS / MAX_ENTITIES)

The engine defines default limits in `core/Scene.h`: `MAX_LAYERS` (default 3) and `MAX_ENTITIES` (default 32). These are guarded with `#ifndef`, so you can override them from your project without modifying the engine.

> **Note:** The default of 3 for `MAX_LAYERS` is due to ESP32 platform constraints (memory and draw-loop cost). On native/PC you can safely use a higher value; on ESP32, increasing it may affect performance or memory.

**Compiler flags (recommended)**

In your project (e.g. in `platformio.ini`), add the defines to `build_flags` for the environment you use:

```ini
build_flags =
    -DMAX_LAYERS=5
    -DMAX_ENTITIES=64
```

The compiler defines `MAX_LAYERS` and `MAX_ENTITIES` before processing any `.cpp` file. Because `Scene.h` uses `#ifndef MAX_LAYERS` / `#ifndef MAX_ENTITIES`, it will not redefine them and your values will be used. This affects how many render layers are drawn (see `Scene::draw`) and, on Arduino, the capacity of the scene entity queue when constructed with `MAX_ENTITIES`.

---

### SceneManager

**Inherits:** None

Manages the stack of active scenes. Allows for scene transitions (replacing) and stacking (push/pop), useful for pausing or menus.

#### Public Methods

- **`void setCurrentScene(Scene* newScene)`**
    Replaces the current scene with a new one.

- **`void pushScene(Scene* newScene)`**
    Pushes a new scene onto the stack, pausing the previous one.

- **`void popScene()`**
    Removes the top scene from the stack, resuming the previous one.

- **`Scene* getCurrentScene() const`**
    Gets the currently active scene.

---

### SceneArena (Memory Management)

**Inherits:** None

An optional memory arena for zero-allocation scenes. This feature is enabled via the `PIXELROOT32_ENABLE_SCENE_ARENA` macro. It allows scenes to pre-allocate a fixed memory block for temporary data or entity storage, avoiding heap fragmentation on embedded devices.

On ESP32, the main trade-off is:

- **Benefits**: predictable memory usage, no `new`/`delete` in the scene, reduced fragmentation.
- **Costs**: the buffer size is fixed (over-allocating wastes RAM, under-allocating returns `nullptr`), and all allocations are freed only when the arena is reset or the scene ends.

#### Public Methods

- **`void init(void* memory, std::size_t size)`**
    Initializes the arena with a pre-allocated memory buffer.

- **`void reset()`**
    Resets the allocation offset to zero. This "frees" all memory in the arena instantly.

- **`void* allocate(std::size_t size, std::size_t alignment)`**
    Allocates a block of memory from the arena. Returns `nullptr` if the arena is full.

---

## Graphics Module

The Graphics module handles everything related to drawing to the screen, including text, shapes, bitmaps, and particle effects.

### Renderer

**Inherits:** None

High-level graphics rendering system. Provides a unified API for drawing shapes, text, and images, abstracting the underlying hardware implementation.

#### Public Methods

- **`void beginFrame()`**
    Prepares the buffer for a new frame (clears screen).

- **`void endFrame()`**
    Finalizes the frame and sends the buffer to the display.

- **`void setOffsetBypass(bool bypass)`**
    Enables or disables camera offset bypass. When enabled, subsequent draw calls will ignore global x/y offsets (scrolling). This is typically managed automatically by `UILayout` when `fixedPosition` is enabled.

- **`bool isOffsetBypassEnabled() const`**
    Returns whether the offset bypass is currently active.

- **`void drawText(const char* text, int16_t x, int16_t y, Color color, uint8_t size)`**
    Draws a string of text using the native bitmap font system. Uses the default font set in `FontManager`, or a custom font if provided via the overloaded version.
    - **text**: The string to render (ASCII characters 32-126 are supported).
    - **x, y**: Position where text starts (top-left corner).
    - **color**: Color from the `Color` enum (uses sprite palette context).
    - **size**: Scale multiplier (1 = normal, 2 = double, 3 = triple, etc.).

- **`void drawText(const char* text, int16_t x, int16_t y, Color color, uint8_t size, const Font* font)`**
    Draws text using a specific font. If `font` is `nullptr`, uses the default font from `FontManager`.

- **`void drawTextCentered(const char* text, int16_t y, Color color, uint8_t size)`**
    Draws text centered horizontally at a given Y coordinate using the default font.

- **`void drawTextCentered(const char* text, int16_t y, Color color, uint8_t size, const Font* font)`**
    Draws text centered horizontally using a specific font. If `font` is `nullptr`, uses the default font from `FontManager`.

- **`void drawFilledCircle(int x, int y, int radius, uint16_t color)`**
    Draws a filled circle.

- **`void drawCircle(int x, int y, int radius, uint16_t color)`**
    Draws a circle outline.

- **`void drawRectangle(int x, int y, int width, int height, uint16_t color)`**
    Draws a rectangle outline.

- **`void drawFilledRectangle(int x, int y, int width, int height, uint16_t color)`**
    Draws a filled rectangle.

- **`void drawLine(int x1, int y1, int x2, int y2, uint16_t color)`**
    Draws a line between two points.

- **`void drawBitmap(int x, int y, int width, int height, const uint8_t *bitmap, uint16_t color)`**
    Draws a bitmap image.

- **`void drawPixel(int x, int y, uint16_t color)`**
    Draws a single pixel.

- **`void drawSprite(const Sprite& sprite, int x, int y, Color color, bool flipX = false)`**
    Draws a 1bpp monochrome sprite described by a `Sprite` struct using a palette `Color`. Bit 0 of each row is the leftmost pixel, bit (`width - 1`) the rightmost pixel.

- **`void drawSprite(const Sprite2bpp& sprite, int x, int y, bool flipX = false)`**
    Available when `PIXELROOT32_ENABLE_2BPP_SPRITES` is defined. Draws a packed 2bpp sprite where each pixel stores a 2-bit index into the sprite-local palette. Index `0` is treated as transparent.
    *Optimized:* Uses `uint16_t` native access and supports MSB-first bit ordering for high performance.

- **`void drawSprite(const Sprite4bpp& sprite, int x, int y, bool flipX = false)`**
    Available when `PIXELROOT32_ENABLE_4BPP_SPRITES` is defined. Draws a packed 4bpp sprite where each pixel stores a 4-bit index into the sprite-local palette. Index `0` is treated as transparent.
    *Optimized:* Uses `uint16_t` native access and supports MSB-first bit ordering for high performance.

- **`void drawMultiSprite(const MultiSprite& sprite, int x, int y)`**
    Draws a layered sprite composed of multiple 1bpp `SpriteLayer` entries. Each layer is rendered in order using `drawSprite`, enabling multi-color NES/GameBoy-style sprites.

- **`void drawTileMap(const TileMap& map, int originX, int originY, Color color)`**
    Draws a tile-based background using a compact `TileMap` descriptor built on 1bpp `Sprite` tiles. Includes automatic Viewport Culling.

- **`void drawTileMap(const TileMap2bpp& map, int originX, int originY)`**
    Available when `PIXELROOT32_ENABLE_2BPP_SPRITES` is defined. Draws a 2bpp tilemap. Optimized with Viewport Culling and Palette LUT Caching.

- **`void drawTileMap(const TileMap4bpp& map, int originX, int originY)`**
    Available when `PIXELROOT32_ENABLE_4BPP_SPRITES` is defined. Draws a 4bpp tilemap. Optimized with Viewport Culling and Palette LUT Caching.

---

### Platform Optimizations (ESP32)

The engine includes several low-level optimizations for the ESP32 platform to maximize performance:

- **DMA Support**: Buffer transfers to the display are handled via DMA (`pushImageDMA`), allowing the CPU to process the next frame while the current one is being sent to the hardware.
- **IRAM Execution**: Critical rendering functions (`drawPixel`, `drawSpriteInternal`, `resolveColor`, `drawTileMap`) are decorated with `IRAM_ATTR` to run from internal RAM, bypassing the slow SPI Flash latency.
- **Palette Caching**: Tilemaps cache the resolved RGB565 palette for each tile to avoid redundant color calculations during the draw loop.
- **Viewport Culling**: All tilemap rendering functions automatically skip tiles that are outside the current screen boundaries.

- **`void setDisplaySize(int w, int h)`**
    Sets the logical display size.

- **`void setDisplayOffset(int x, int y)`**
    Sets a global offset for all drawing operations.

- **`void setContrast(uint8_t level)`**
    Sets the display contrast/brightness (0-255).

- **`void setFont(const uint8_t* font)`**
    @deprecated This method is obsolete. The engine now uses the native bitmap font system. Use `FontManager::setDefaultFont()` to set the default font, or pass a `Font*` directly to `drawText()` overloads.

---

### Font System

The engine includes a native bitmap font system that uses 1bpp sprites to render text, ensuring pixel-perfect consistency between PC (SDL2) and ESP32 platforms.

#### Font Structure

**Type:** `struct Font`

Represents a bitmap font containing glyph sprites for ASCII characters.

**Members:**
- **`const Sprite* glyphs`**: Array of sprite structures, one per character.
- **`uint8_t firstChar`**: First character code in the font (e.g., 32 for space).
- **`uint8_t lastChar`**: Last character code in the font (e.g., 126 for tilde).
- **`uint8_t glyphWidth`**: Fixed width of each glyph in pixels.
- **`uint8_t glyphHeight`**: Fixed height of each glyph in pixels.
- **`uint8_t spacing`**: Horizontal spacing between characters in pixels.
- **`uint8_t lineHeight`**: Vertical line height (includes spacing between lines).

**Example:**
```cpp
#include <graphics/Font.h>
#include <graphics/Font5x7.h>

// The built-in 5x7 font is available as FONT_5X7
const Font* myFont = &pixelroot32::graphics::FONT_5X7;
```

#### FontManager

**Type:** `class FontManager`

Static utility class for managing fonts and calculating text dimensions.

**Static Methods:**

- **`static void setDefaultFont(const Font* font)`**
    Sets the default font used by `Renderer::drawText()` when no font is explicitly provided. The default font is automatically set to `FONT_5X7` during `Engine::init()`.

- **`static const Font* getDefaultFont()`**
    Returns the currently active default font, or `nullptr` if no font is set.

- **`static int16_t textWidth(const Font* font, const char* text, uint8_t size = 1)`**
    Calculates the pixel width of a text string when rendered with the specified font and size.
    - **font**: Font to use (or `nullptr` to use default font).
    - **text**: String to measure.
    - **size**: Scale multiplier (1 = normal, 2 = double, etc.).
    - **Returns**: Width in pixels.

- **`static uint8_t getGlyphIndex(char c, const Font* font = nullptr)`**
    Gets the array index of a character's glyph sprite.
    - **c**: Character code.
    - **font**: Font to use (or `nullptr` to use default font).
    - **Returns**: Glyph index (0 to `lastChar - firstChar`), or 255 if character is not supported.

- **`static bool isCharSupported(char c, const Font* font = nullptr)`**
    Checks if a character is supported by the font.
    - **c**: Character code.
    - **font**: Font to use (or `nullptr` to use default font).
    - **Returns**: `true` if the character is in the font's range.

**Example:**
```cpp
#include <graphics/FontManager.h>
#include <graphics/Font5x7.h>

// Set a custom font as default
FontManager::setDefaultFont(&myCustomFont);

// Calculate text width
int16_t width = FontManager::textWidth(nullptr, "Hello", 2); // Uses default font, size 2

// Check if character is supported
if (FontManager::isCharSupported('A')) {
    // Character is available
}
```

#### Built-in Font: FONT_5X7

**Type:** `extern const Font FONT_5X7`

A built-in 5x7 pixel bitmap font containing ASCII characters from space (32) to tilde (126), for a total of 95 characters.

**Characteristics:**
- **Glyph Size**: 5 pixels wide × 7 pixels tall
- **Character Range**: ASCII 32-126 (printable characters)
- **Spacing**: 1 pixel between characters
- **Line Height**: 8 pixels (7 + 1)

**Usage:**
```cpp
#include <graphics/Font5x7.h>

// Use the built-in font
renderer.drawText("Hello", 10, 10, Color::White, 1, &pixelroot32::graphics::FONT_5X7);

// Or set it as default
FontManager::setDefaultFont(&pixelroot32::graphics::FONT_5X7);
```

**Note:** The default font is automatically set to `FONT_5X7` during `Engine::init()`, so you typically don't need to set it manually unless you want to use a different font.

#### Text Rendering Examples

**Basic text rendering:**
```cpp
// Uses default font (FONT_5X7)
renderer.drawText("Score: 100", 10, 10, Color::White, 1);

// With custom size
renderer.drawText("BIG TEXT", 10, 30, Color::Yellow, 3);

// Centered text
renderer.drawTextCentered("GAME OVER", 120, Color::Red, 2);
```

**Using a custom font:**
```cpp
// Define your custom font (must be defined elsewhere)
extern const Font MY_CUSTOM_FONT;

// Use it explicitly
renderer.drawText("Custom", 10, 10, Color::Cyan, 1, &MY_CUSTOM_FONT);

// Or set it as default
FontManager::setDefaultFont(&MY_CUSTOM_FONT);
renderer.drawText("Now default", 10, 20, Color::White, 1);
```

**Calculating text dimensions:**
```cpp
// Calculate width for centering
const char* text = "Hello World";
int16_t textWidth = FontManager::textWidth(nullptr, text, 2);
int16_t x = (DISPLAY_WIDTH - textWidth) / 2;
renderer.drawText(text, x, 50, Color::White, 2);
```

---

### Camera2D

**Inherits:** None

---

## UI Module

The UI module provides elements for building user interfaces, including buttons, checkboxes, and automatic layout containers. All UI elements inherit from `UIElement`, which itself inherits from `Entity`, allowing them to be easily integrated into any `Scene`.

### UIElement

**Inherits:** [Entity](#entity)

Abstract base class for all UI components. UI elements are automatically assigned to render layer `2` (UI layer).

#### Public Methods

- **`UIElement(float x, float y, float w, float h, UIElementType t = UIElementType::GENERIC)`**
    Constructs a new UI element.
- **`UIElementType getType() const`**
    Returns the type of the UI element (e.g., `BUTTON`, `CHECKBOX`, `LAYOUT`).
- **`virtual bool isFocusable() const`**
    Returns whether the element can receive focus/selection for navigation.
- **`void setPosition(float newX, float newY)`**
    Updates the element's position.
- **`void setFixedPosition(bool fixed)`**
    Enables or disables fixed position for the element. When `true`, the element and its children (if it's a layout) will ignore `Camera2D` scroll and stay fixed at their logical screen coordinates.
- **`bool isFixedPosition() const`**
    Returns whether the element is in a fixed position.
- **`virtual void getPreferredSize(float& preferredWidth, float& preferredHeight) const`**
    Returns the size the element prefers to have (used by layouts).

---

### UIButton

**Inherits:** [UIElement](#uielement)

A clickable button that supports both physical button/keyboard input and touch input.

#### Public Methods

- **`UIButton(std::string label, uint8_t index, float x, float y, float w, float h, std::function<void()> callback, TextAlignment textAlign = CENTER, int fontSize = 2)`**
    Constructs a new button with a navigation index and a click callback.
- **`void setStyle(Color textCol, Color bgCol, bool drawBg)`**
    Configures the button's colors and background visibility.
- **`void setSelected(bool selected)`**
    Sets the focus state of the button (typically called by a layout).
- **`bool getSelected() const`**
    Returns whether the button is currently focused.
- **`void handleInput(const InputManager& input)`**
    Processes input events. If focused and the action button is pressed, the callback is triggered.
- **`void press()`**
    Manually triggers the button's click callback.

---

### UICheckBox

**Inherits:** [UIElement](#uielement)

A UI element that can be toggled between checked and unchecked states.

#### Public Methods

- **`UICheckBox(std::string label, uint8_t index, float x, float y, float w, float h, bool checked = false, std::function<void(bool)> callback = nullptr, int fontSize = 2)`**
    Constructs a new checkbox.
    - `label`: Checkbox label text.
    - `index`: Navigation index (for D-pad navigation).
    - `x, y`: Position.
    - `w, h`: Size.
    - `checked`: Initial state.
    - `callback`: Function called when state changes.
    - `fontSize`: Text size multiplier.
- **`void setStyle(Color textCol, Color bgCol, bool drawBg = false)`**
    Configures colors and background visibility.
- **`void setChecked(bool checked)`**
    Sets the current state of the checkbox.
- **`bool isChecked() const`**
    Returns `true` if the checkbox is checked.
- **`void setSelected(bool selected)`**
    Sets the focus state.
- **`bool getSelected() const`**
    Returns whether the checkbox is focused.
- **`void toggle()`**
    Inverts the current state and triggers the callback.
- **`void handleInput(const InputManager& input)`**
    Processes input events. If focused and the action button is pressed, it toggles the state.

---

### UILayout

**Inherits:** [UIElement](#uielement)

Base class for UI containers that automatically organize child elements.

#### Public Methods

- **`virtual void addElement(UIElement* element)`**
    Adds a new element to the layout.
- **`virtual void removeElement(UIElement* element)`**
    Removes an element from the layout.
- **`virtual void updateLayout()`**
    Forces a recalculation of all child element positions.
- **`void setPadding(float p)`**
    Sets internal padding for the container.
- **`void setSpacing(float s)`**
    Sets spacing between elements.
- **`void clearElements()`**
    Removes all elements from the container.

- **`void setFixedPosition(bool fixed)`**
    Enables or disables fixed positioning for the layout. When `true`, the layout and all its children will ignore the `Camera2D` scroll/offset and remain at their specified screen coordinates.

- **`bool isFixedPosition() const`**
    Returns whether the layout is currently in fixed position mode.

---

### UIHorizontalLayout & UIVerticalLayout

**Inherits:** [UILayout](#uilayout)

Specific layout implementations that organize elements in a row or column. Both support automatic scrolling when elements exceed the viewport size.

#### Public Methods (Specific to Horizontal/Vertical)

- **`void enableScroll(bool enable)`**
    Enables or disables scrolling support.
- **`void setViewportWidth(float w)`** (Horizontal) / **`void setViewportHeight(float h)`** (Vertical)
    Sets the visible area dimensions for scrolling calculations.
- **`float getScrollOffset() const`**
    Returns the current scroll position in pixels.
- **`void setNavigationButtons(uint8_t upButton, uint8_t downButton)`**
    Sets the button indices used for layout navigation (e.g., UP/DOWN for vertical, LEFT/RIGHT for horizontal).
- **`void setButtonStyle(Color selectedTextCol, Color selectedBgCol, Color unselectedTextCol, Color unselectedBgCol)`**
    Sets the colors used for child buttons when they are selected or unselected.

---

### Data Structures

#### ScrollBehavior (Enum)

- `NONE`: No scrolling allowed.
- `SCROLL`: Free scrolling within content bounds.
- `CLAMP`: Scrolling that stops at the edges of the content.

#### TextAlignment (Enum)

- `LEFT`
- `CENTER`
- `RIGHT`

#### UIElementType (Enum)

- `GENERIC`, `BUTTON`, `LABEL`, `CHECKBOX`, `LAYOUT`

Dead-zone 2D camera used for side-scrolling and simple platformer levels. It controls the world-to-screen offset by driving `Renderer::setDisplayOffset`.

#### Public Methods

- **`Camera2D(int viewportWidth, int viewportHeight)`**  
    Constructs a camera for a given viewport size in pixels.

- **`void setBounds(float minX, float maxX)`**  
    Sets horizontal limits for the camera position. The internal `x` value is clamped to this range whenever it changes. Use this to keep the camera inside the level width.

- **`void setVerticalBounds(float minY, float maxY)`**  
    Sets vertical limits for the camera position. Pass the same value for `minY` and `maxY` to lock vertical movement (for example, in pure side-scrollers).

- **`void setPosition(float x, float y)`**  
    Sets the camera origin in world coordinates and applies both horizontal and vertical bounds.

- **`void followTarget(float targetX)`**  
    Horizontally follows a target using a dead zone expressed as a fraction of the viewport width (by default, between 30% and 70% of the screen). The camera only moves when the target leaves this region.

- **`void followTarget(float targetX, float targetY)`**  
    2D variant that follows a target in both axes using horizontal and vertical dead zones (also expressed as fractions of the viewport size), respecting the configured bounds on each axis.

- **`float getX() const`**, **`float getY() const`**  
    Returns the current camera position in world space.

- **`void apply(Renderer& renderer) const`**  
    Applies the camera by calling `renderer.setDisplayOffset(-x, -y)`, so subsequent draw calls are automatically shifted by the camera.

---

### Color

**Inherits:** None

The `Color` module manages the engine's color palettes and provides the `Color` enumeration for referencing colors within the active palette.

#### PaletteType (Enum)

Enumeration of available color palettes.

- `PR32` (Default): The standard PixelRoot32 palette (vibrant, general purpose).
- `NES`: Nintendo Entertainment System inspired palette.
- `GB`: Game Boy inspired palette (4 greens).
- `GBC`: Game Boy Color inspired palette.
- `PICO8`: PICO-8 fantasy console palette.

#### Public Methods

- **`static void setPalette(PaletteType type)`**
    Sets the active color palette for the engine (legacy mode).
    *Note: This sets both background and sprite palettes to the same value. Does not enable dual palette mode. This should typically be called once during game initialization (e.g., in the first Scene's `init()` method).*

- **`static void setCustomPalette(const uint16_t* palette)`**
    Sets a custom color palette defined by the user (legacy mode).
  - **palette**: Pointer to an array of 16 `uint16_t` values (RGB565).
  - **Warning**: The array must remain valid for the duration of its use (e.g., use `static const` or global arrays). The engine does not copy the data.
  - *Note: Sets both background and sprite palettes to the same value. Does not enable dual palette mode.*

- **`static void enableDualPaletteMode(bool enable)`**
    Enables or disables dual palette mode.
    - **enable**: `true` to enable dual palette mode (separate palettes for backgrounds and sprites), `false` for legacy mode (single palette).

- **`static void setBackgroundPalette(PaletteType palette)`**
    Sets the background palette (for backgrounds, tilemaps, etc.).
    - **palette**: The palette type to use for backgrounds.

- **`static void setSpritePalette(PaletteType palette)`**
    Sets the sprite palette (for sprites, characters, etc.).
    - **palette**: The palette type to use for sprites.

- **`static void setBackgroundCustomPalette(const uint16_t* palette)`**
    Sets a custom background palette.
    - **palette**: Pointer to an array of 16 `uint16_t` RGB565 color values. Must remain valid.

- **`static void setSpriteCustomPalette(const uint16_t* palette)`**
    Sets a custom sprite palette.
    - **palette**: Pointer to an array of 16 `uint16_t` RGB565 color values. Must remain valid.

- **`static void setDualPalette(PaletteType bgPalette, PaletteType spritePalette)`**
    Convenience function that sets both background and sprite palettes at once and automatically enables dual palette mode.
    - **bgPalette**: The palette type to use for backgrounds.
    - **spritePalette**: The palette type to use for sprites.

- **`static void setDualCustomPalette(const uint16_t* bgPalette, const uint16_t* spritePal)`**
    Convenience function that sets both custom palettes at once and automatically enables dual palette mode.
    - **bgPalette**: Pointer to an array of 16 `uint16_t` RGB565 color values for backgrounds. Must remain valid.
    - **spritePal**: Pointer to an array of 16 `uint16_t` RGB565 color values for sprites. Must remain valid.

- **`static uint16_t resolveColor(Color color)`**
    Converts a `Color` enum value to its corresponding RGB565 `uint16_t` representation based on the currently active palette (legacy mode).

- **`static uint16_t resolveColor(Color color, PaletteContext context)`**
    Converts a `Color` enum value to its corresponding RGB565 `uint16_t` representation based on the context (dual palette mode) or current active palette (legacy mode).
    - **context**: `PaletteContext::Background` for backgrounds/tilemaps, `PaletteContext::Sprite` for sprites.

#### Color (Enum)

Enumeration of color indices available in the engine's palette. The actual RGB value of each color depends on the active `PaletteType`.

- `Black`, `White`, `LightGray`, `DarkGray`
- `Red`, `DarkRed`, `Green`, `DarkGreen`, `Blue`, `DarkBlue`
- `Yellow`, `Orange`, `Brown`
- `Purple`, `Pink`, `Cyan`
- `LightBlue`, `LightGreen`, `LightRed`
- `Navy`, `Teal`, `Olive`
- `Gold`, `Silver`
- `Transparent` (special value, not rendered)
- `DebugRed`, `DebugGreen`, `DebugBlue` (debug colors)

---

### Sprite

**Inherits:** None

Compact descriptor for monochrome bitmapped sprites used by `Renderer::drawSprite`. This is the default 1bpp format used throughout the engine.

#### Properties

- **`const uint16_t* data`**  
  Pointer to an array of 16-bit rows. Each `uint16_t` packs pixels for one row.

- **`uint8_t width`**  
  Sprite width in pixels (typically ≤ 16).

- **`uint8_t height`**  
  Sprite height in pixels.

#### Bit Convention

- Each bit represents one pixel: `0` = transparent, `1` = pixel on.  
- Bit 0 is the leftmost pixel in the row.  
- Bit (`width - 1`) is the rightmost pixel in the row.

---

### Sprite2bpp

**Inherits:** None

Optional descriptor for packed 2bpp sprites, enabled when `PIXELROOT32_ENABLE_2BPP_SPRITES` is defined. This format is intended for higher fidelity assets while keeping 1bpp as the default.

#### Properties

- **`const uint8_t* data`**  
  Pointer to packed 2bpp bitmap data. Pixels are stored row by row, four pixels per byte.

- **`const Color* palette`**  
  Pointer to a small sprite-local palette. Each pixel value selects a `Color` from this array.

- **`uint8_t width`**  
  Sprite width in pixels.

- **`uint8_t height`**  
  Sprite height in pixels.

- **`uint8_t paletteSize`**  
  Number of entries in the palette array (up to 4 are used by the 2bpp format).

#### Bit Convention

- Each pixel uses 2 bits storing an index in the range `[0, 3]`.  
- Index `0` is treated as transparent.  
- Within each row, bit 0 of the first byte corresponds to the leftmost pixel.

---

### Sprite4bpp

**Inherits:** None

Optional descriptor for packed 4bpp sprites, enabled when `PIXELROOT32_ENABLE_4BPP_SPRITES` is defined. Use this format for special cases that need more than four colors per sprite.

#### Properties

- **`const uint8_t* data`**  
  Pointer to packed 4bpp bitmap data. Pixels are stored row by row, two pixels per byte.

- **`const Color* palette`**  
  Pointer to a sprite-local palette. Each pixel value selects a `Color` from this array.

- **`uint8_t width`**  
  Sprite width in pixels.

- **`uint8_t height`**  
  Sprite height in pixels.

- **`uint8_t paletteSize`**  
  Number of entries in the palette array (up to 16 are used by the 4bpp format).

#### Bit Convention

- Each pixel uses 4 bits storing an index in the range `[0, 15]`.  
- Index `0` is treated as transparent.  
- Within each row, bit 0 of the first byte corresponds to the leftmost pixel.

---

### SpriteLayer

**Inherits:** None

Single monochrome layer used by layered sprites (`MultiSprite`).

#### Properties

- **`const uint16_t* data`**  
  Packed 1bpp bitmap data for this layer, using the same layout as `Sprite::data`.

- **`Color color`**  
  Palette color used for active pixels in this layer.

---

### MultiSprite

**Inherits:** None

Multi-layer, multi-color sprite built from one or more `SpriteLayer` entries. All layers share the same width and height and are drawn in array order.

#### Properties

- **`uint8_t width`**  
  Sprite width in pixels.

- **`uint8_t height`**  
  Sprite height in pixels.

- **`const SpriteLayer* layers`**  
  Pointer to the first element of a `SpriteLayer` array.

- **`uint8_t layerCount`**  
  Number of layers in the array.

---

### TileMapGeneric (Template)

**Inherits:** None

Generic descriptor for tile-based backgrounds. It stores level data as an array of indices mapping to a tileset.

#### Template Parameters

- **`T`**: The sprite type used for tiles (e.g., `Sprite`, `Sprite2bpp`, `Sprite4bpp`).

#### Properties

- **`uint8_t* indices`**  
  Array of tile indices (one byte per tile). Array size must be `width * height`.

- **`uint8_t width`**  
  Width of the tilemap in tiles.

- **`uint8_t height`**  
  Height of the tilemap in tiles.

- **`const T* tiles`**  
  Pointer to the first element of a tileset array of type `T`.

- **`uint8_t tileWidth`**  
  Width of each individual tile in pixels.

- **`uint8_t tileHeight`**  
  Height of each individual tile in pixels.

- **`uint16_t tileCount`**  
  Number of unique tiles in the `tiles` array.

---

### TileMap (Alias)

**Type:** `TileMapGeneric<Sprite>`

Standard 1bpp tilemap.

---

### TileMap2bpp (Alias)

**Type:** `TileMapGeneric<Sprite2bpp>`

Optional 2bpp tilemap, available when `PIXELROOT32_ENABLE_2BPP_SPRITES` is defined.

---

### TileMap4bpp (Alias)

**Type:** `TileMapGeneric<Sprite4bpp>`

Optional 4bpp tilemap, available when `PIXELROOT32_ENABLE_4BPP_SPRITES` is defined.

---

### SpriteAnimationFrame

**Inherits:** None

Single animation frame that can reference either a simple `Sprite` or a layered `MultiSprite`.

#### Properties

- **`const Sprite* sprite`**  
  Optional pointer to a `Sprite` used for this frame. May be `nullptr` when using layered sprites only.

- **`const MultiSprite* multiSprite`**  
  Optional pointer to a `MultiSprite` used for this frame. May be `nullptr` when using simple sprites only.

Exactly one of `sprite` or `multiSprite` is expected to be non-null for a valid frame.

---

### SpriteAnimation

**Inherits:** None

Lightweight, step-based animation controller for sprite frames. It owns no memory and only references a compile-time array of `SpriteAnimationFrame` entries.

#### Properties

- **`const SpriteAnimationFrame* frames`**  
  Pointer to the first element of an immutable frame table.

- **`uint8_t frameCount`**  
  Number of frames in the table.

- **`uint8_t current`**  
  Current frame index in the range `[0, frameCount)`.

#### Public Methods

- **`void reset()`**  
  Resets the animation to the first frame (`current = 0`).

- **`void step()`**  
  Advances the animation by one frame. When the index reaches `frameCount`, it wraps back to `0`. Intended for step-based animation (e.g. once per horde movement in a game).

- **`const SpriteAnimationFrame& getCurrentFrame() const`**  
  Returns a reference to the current frame descriptor.

- **`const Sprite* getCurrentSprite() const`**  
  Convenience accessor for the current `Sprite`, or `nullptr` when the frame is layered-only.

- **`const MultiSprite* getCurrentMultiSprite() const`**  
  Convenience accessor for the current `MultiSprite`, or `nullptr` when the frame is simple-only.

#### Example Usage (step-based)

```cpp
using namespace pixelroot32::graphics;

// Two-frame animation using simple sprites
static const Sprite SPRITE_F1 = { F1_BITS, 8, 8 };
static const Sprite SPRITE_F2 = { F2_BITS, 8, 8 };

static const SpriteAnimationFrame WALK_FRAMES[] = {
    { &SPRITE_F1, nullptr },
    { &SPRITE_F2, nullptr }
};

class EnemyActor {
public:
    EnemyActor()
    {
        anim.frames     = WALK_FRAMES;
        anim.frameCount = sizeof(WALK_FRAMES) / sizeof(SpriteAnimationFrame);
        anim.reset();
    }

    void stepLogic()
    {
        // Called by the scene when the enemy takes a logical "step"
        anim.step();
    }

    void draw(Renderer& renderer)
    {
        const Sprite* frame = anim.getCurrentSprite();
        if (!frame) {
            return;
        }
        renderer.drawSprite(*frame,
                            static_cast<int>(x),
                            static_cast<int>(y),
                            Color::White);
    }

private:
    float x = 0;
    float y = 0;
    SpriteAnimation anim;
};
```

#### Example Usage (Actor + Renderer)

```cpp
using namespace pixelroot32::graphics;

// Raw 1bpp data (one row per uint16_t, bit 0 = leftmost pixel)
static const uint16_t BODY_BITS[] = {
    0x0018,
    0x003C,
    0x007E,
    0x00FF,
    0x007E,
    0x003C,
    0x0018,
    0x0000
};

static const uint16_t EYES_BITS[] = {
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0000,
    0x0240,
    0x0000,
    0x0000
};

static const SpriteLayer ENEMY_LAYERS[] = {
    { BODY_BITS, Color::Orange },
    { EYES_BITS, Color::White }
};

static const MultiSprite ENEMY_MULTI = {
    11,            // width
    8,             // height
    ENEMY_LAYERS,  // layers
    2              // layerCount
};

// Inside an Actor::draw(Renderer& renderer) implementation:
void EnemyActor::draw(Renderer& renderer) {
    if (!isAlive) {
        return;
    }

    renderer.drawMultiSprite(ENEMY_MULTI,
                             static_cast<int>(x),
                             static_cast<int>(y));
}
```

---

### DisplayConfig

**Inherits:** None

Configuration settings for initializing displays.

#### Properties

- **`rotation`**: Display rotation (0-3).
- **`width`**: Display width in pixels.
- **`height`**: Display height in pixels.
- **`xOffset, yOffset`**: Offsets for display rendering.

---

### DrawSurface

**Inherits:** None

Abstract interface for platform-specific drawing operations. Implementations of this class handle the low-level communication with the display hardware (or window system).

**Note:** This interface is primarily for internal engine use. Most developers should use the `Renderer` class instead, which provides a higher-level, platform-agnostic API.

#### Public Methods

- **`virtual void init()`**
    Initializes the hardware or window.

- **`virtual void setRotation(uint8_t rotation)`**
    Sets the display rotation (0-3).

- **`virtual void clearBuffer()`**
    Clears the frame buffer (fills with black or background color).

- **`virtual void sendBuffer()`**
    Sends the frame buffer to the physical display.

- **`virtual void drawText(const char* text, int16_t x, int16_t y, uint16_t color, uint8_t size)`**
    @deprecated **This method is obsolete.** Text rendering is now handled by `Renderer` using the native bitmap font system. This method is kept only for interface compatibility and should never be called. All text rendering goes through `Renderer::drawText()` which uses the font system.

- **`virtual void drawTextCentered(const char* text, int16_t y, uint16_t color, uint8_t size)`**
    @deprecated **This method is obsolete.** Text rendering is now handled by `Renderer` using the native bitmap font system. This method is kept only for interface compatibility and should never be called. All text rendering goes through `Renderer::drawTextCentered()` which uses the font system.

- **`virtual void drawPixel(int x, int y, uint16_t color)`**
    Draws a single pixel at the specified coordinates.

- **`virtual void drawLine(int x1, int y1, int x2, int y2, uint16_t color)`**
    Draws a line between two points.

- **`virtual void drawRectangle(int x, int y, int width, int height, uint16_t color)`**
    Draws a rectangle outline.

- **`virtual void drawFilledRectangle(int x, int y, int width, int height, uint16_t color)`**
    Draws a filled rectangle.

- **`virtual void drawCircle(int x, int y, int radius, uint16_t color)`**
    Draws a circle outline.

- **`virtual void drawFilledCircle(int x, int y, int radius, uint16_t color)`**
    Draws a filled circle.

- **`virtual void drawBitmap(int x, int y, int width, int height, const uint8_t* bitmap, uint16_t color)`**
    Draws a bitmap image.

- **`virtual void setContrast(uint8_t level)`**
    Sets the display contrast/brightness (0-255).

- **`virtual uint16_t color565(uint8_t r, uint8_t g, uint8_t b)`**
    Converts RGB888 color to RGB565 format.

- **`virtual void setDisplaySize(int w, int h)`**
    Sets the logical display size.

- **`virtual bool processEvents()`**
    Processes platform events (e.g., SDL window events). Returns `false` if the application should quit.

- **`virtual void present()`**
    Swaps buffers (for double-buffered systems like SDL).

---

### UIElement

**Inherits:** [Entity](#entity)

Base class for all user interface elements.

---

### UIButton

**Inherits:** [UIElement](#uielement)

A clickable button UI element. Supports callback functions and state management (selected/pressed).

#### Public Methods

- **`UIButton(std::string t, uint8_t index, float x, float y, float w, float h, std::function<void()> callback)`**
    Constructs a button.

- **`void setStyle(uint16_t textCol, uint16_t bgCol, bool drawBg)`**
    Configures visual style.

- **`void setSelected(bool selected)`**
    Sets the selection state.

- **`void press()`**
    Manually triggers the button's action.

---

### UILabel

**Inherits:** [UIElement](#uielement)

A simple text label UI element.

#### Public Methods

- **`UILabel(std::string t, float x, float y, Color col, uint8_t sz)`**
    Constructs a label.
    - **t**: Label text.
    - **x, y**: Position.
    - **col**: Text color from the `Color` enum.
    - **sz**: Text size multiplier.

- **`void setText(const std::string& t)`**
    Updates the label's text. Recalculates dimensions immediately using the current font metrics.

- **`void centerX(int screenWidth)`**
    Centers the label horizontally within the specified width. Recalculates dimensions before positioning to ensure precision.

---

### ParticleEmitter

**Inherits:** [Entity](#entity)

Manages a pool of particles to create visual effects (fire, smoke, explosions).

#### Public Methods

- **`ParticleEmitter(float x, float y, const ParticleConfig& cfg)`**
    Constructs a new emitter with specific configuration.

- **`void burst(float x, float y, int count)`**
    Emits a burst of particles from a specific location.

---

### ParticleConfig

**Inherits:** None

Configuration parameters for a particle emitter.

#### Properties

- **`startColor, endColor`**: Life cycle colors.
- **`minSpeed, maxSpeed`**: Speed range.
- **`gravity`**: Y-axis force.
- **`friction`**: Velocity damping.
- **`minLife, maxLife`**: Lifetime range.

---

### ParticlePresets

**Inherits:** None

Namespace containing predefined `ParticleConfig` constants for common effects:

- `Fire`
- `Explosion`
- `Sparks`
- `Smoke`
- `Dust`

---

## Input Module

Handles input from physical buttons or keyboard.

### InputManager

**Inherits:** None

Handles input polling, debouncing, and state tracking.

#### Public Methods

- **`bool isButtonPressed(uint8_t buttonIndex) const`**
    Returns true if button was just pressed this frame.

- **`bool isButtonReleased(uint8_t buttonIndex) const`**
    Returns true if button was just released this frame.

- **`bool isButtonDown(uint8_t buttonIndex) const`**
    Returns true if button is currently held down.

- **`bool isButtonClicked(uint8_t buttonIndex) const`**
    Returns true if button was clicked (pressed and released).

---

### InputConfig

**Inherits:** None

Configuration structure for `InputManager`. Defines the mapping between logical inputs and physical pins.

---

## Physics Module

### CollisionSystem

**Inherits:** None

Manages collision detection between entities using AABB checks and collision layers.

#### Public Methods

- **`void addEntity(Entity* e)`**
    Registers an entity for collision checks.

- **`void removeEntity(Entity* e)`**
    Unregisters an entity.

- **`void update()`**
    Performs collision detection checks and triggers `onCollision` callbacks.

---

### Collision Primitives

**Inherits:** None

Lightweight geometric primitives and helpers used by the physics and collision systems.

#### Types

- **`struct Circle`**
    Represents a circle in 2D space.

  - `float x, y` – center position.
  - `float radius` – circle radius.

- **`struct Segment`**
    Represents a line segment between two points.

  - `float x1, y1` – start point.
  - `float x2, y2` – end point.

#### Helper Functions

- **`bool intersects(const Circle& a, const Circle& b)`**
    Returns true if two circles overlap.

- **`bool intersects(const Circle& c, const Rect& r)`**
    Returns true if a circle overlaps an axis-aligned rectangle.

- **`bool intersects(const Segment& s, const Rect& r)`**
    Returns true if a line segment intersects an axis-aligned rectangle.

- **`bool sweepCircleVsRect(const Circle& start, const Circle& end, const Rect& rect, float& tHit)`**
    Performs a simple sweep test between two circle positions against a rectangle, using a segment cast against an expanded rectangle.  
    Returns true if a collision occurs between `start` and `end`, writing the normalized hit time in `tHit` (`0.0f` = at `start`, `1.0f` = at `end`).

---

### DefaultLayers

**Inherits:** None

Namespace with common collision layer constants:

- `kNone`: 0 (No collision)
- `kAll`: 0xFFFF (Collides with everything)

---

### PhysicsActor

**Inherits:** [Actor](#actor)

An actor with basic 2D physics properties similar to a RigidBody2D. It extends the base Actor class by adding velocity, acceleration, friction, restitution (bounciness), and world boundary collision resolution.

#### Public Methods

- **`PhysicsActor(float x, float y, float w, float h)`**
    Constructs a PhysicsActor.

- **`void update(unsigned long deltaTime)`**
    Updates the actor state, applying physics integration and checking world boundary collisions.

- **`WorldCollisionInfo getWorldCollisionInfo() const`**
    Gets information about collisions with the world boundaries.

- **`virtual void onWorldCollision()`**
    Callback triggered when this actor collides with world boundaries.

- **`void setVelocity(float x, float y)`**
    Sets the linear velocity of the actor.

- **`void setRestitution(float r)`**
    Sets the restitution (bounciness). 1.0 means perfect bounce, < 1.0 means energy loss.

- **`void setFriction(float f)`**
    Sets the friction coefficient (0.0 means no friction).

- **`void setLimits(LimitRect limits)`**
    Sets custom movement limits for the actor.

- **`void setWorldSize(int width, int height)`**
    Defines the world size for boundary checking, used as default limits.

---

### LimitRect

**Inherits:** None

Bounding rectangle for world-collision resolution. Defines the limits of the play area.

#### Properties

- **`left`**: Left boundary (-1 for no limit).
- **`top`**: Top boundary (-1 for no limit).
- **`right`**: Right boundary (-1 for no limit).
- **`bottom`**: Bottom boundary (-1 for no limit).

#### Public Methods

- **`LimitRect(int l, int t, int r, int b)`**
    Constructs a LimitRect with specific bounds.

- **`int width() const`**
    Calculates the width of the limit area.

- **`int height() const`**
    Calculates the height of the limit area.

---

### WorldCollisionInfo

**Inherits:** None

Information about world collisions in the current frame. Holds flags indicating which sides of the play area the actor collided with.

#### Properties

- **`left`**: True if collided with the left boundary.
- **`right`**: True if collided with the right boundary.
- **`top`**: True if collided with the top boundary.
- **`bottom`**: True if collided with the bottom boundary.

---

## Math Module

### MathUtil

**Inherits:** None

Collection of helper functions.

#### Public Methods

- **`float lerp(float a, float b, float t)`**
    Linear interpolation.
- **`float clamp(float v, float min, float max)`**
    Clamps a value between min and max.

---

## UI Module

The UI module provides classes for creating user interfaces.

### UIElement

**Inherits:** [Entity](#entity)

Base class for all user interface elements (buttons, labels, etc.). Sets the `EntityType` to `UI_ELEMENT`.

#### UIElementType (Enum)

Enumeration of UI element types for runtime type identification.

- `GENERIC`
- `BUTTON`
- `LABEL`
- `LAYOUT`

#### Public Methods

- **`UIElement(float x, float y, float w, float h, UIElementType type = UIElementType::GENERIC)`**
    Constructs a new UIElement.
    - `x, y`: Position.
    - `w, h`: Dimensions.
    - `type`: The type of the element (default: `GENERIC`).

- **`UIElementType getType() const`**
    Returns the type of the UI element.

- **`virtual bool isFocusable() const`**
    Checks if the element is focusable/selectable. Useful for navigation logic. Returns `false` by default.

- **`void setPosition(float newX, float newY)`**
    Sets the position of the element. Used by layouts to reposition elements automatically.

- **`void getPreferredSize(float& preferredWidth, float& preferredHeight) const`**
    Gets the preferred size of the element. Used by layouts to determine how much space the element needs. By default, returns the current width and height.

### UIButton

**Inherits:** [UIElement](#uielement)

A clickable button UI element. Supports both physical (keyboard/gamepad) and touch input. Can trigger a callback function when pressed.

#### Public Methods

- **`UIButton(std::string t, uint8_t index, float x, float y, float w, float h, std::function<void()> callback, TextAlignment textAlign = TextAlignment::CENTER, int fontSize = 2)`**
    Constructs a new UIButton.
  - `t`: Button label text.
  - `index`: Navigation index (for D-pad navigation).
  - `x, y`: Position.
  - `w, h`: Size.
  - `callback`: Function to call when clicked/pressed.
  - `textAlign`: Text alignment (LEFT, CENTER, RIGHT).
  - `fontSize`: Text size multiplier.

- **`void setStyle(Color textCol, Color bgCol, bool drawBg)`**
    Configures the button's visual style.

- **`void setSelected(bool selected)`**
    Sets the selection state (e.g., focused via D-pad).

- **`bool getSelected() const`**
    Checks if the button is currently selected.

- **`void handleInput(const InputManager& input)`**
    Handles input events. Checks for touch events within bounds or confirmation buttons if selected.

- **`void press()`**
    Manually triggers the button's action.

- **`bool isFocusable() const override`**
    Returns `true` (Buttons are always focusable).

### UILabel

**Inherits:** [UIElement](#uielement)

A simple text label UI element. Displays a string of text on the screen. Auto-calculates its bounds based on text length and size.

#### Public Methods

- **`UILabel(std::string t, float x, float y, Color col, uint8_t sz)`**
    Constructs a new UILabel.

- **`void setText(const std::string& t)`**
    Updates the label's text. Recalculates dimensions.

- **`void setVisible(bool v)`**
    Sets visibility.

- **`void centerX(int screenWidth)`**
    Centers the label horizontally on the screen.

---

### UILayout

**Inherits:** [UIElement](#uielement)

Base class for UI layout containers. Layouts organize UI elements automatically, handling positioning, spacing, and optional scrolling.

#### Public Methods

- **`UILayout(float x, float y, float w, float h)`**
    Constructs a new UILayout.

- **`void setPadding(float p)`**
    Sets the padding (internal spacing) of the layout.

- **`float getPadding() const`**
    Gets the current padding.

- **`void setSpacing(float s)`**
    Sets the spacing between elements.

- **`float getSpacing() const`**
    Gets the current spacing.

- **`size_t getElementCount() const`**
    Gets the number of elements in the layout.

- **`UIElement* getElement(size_t index) const`**
    Gets the element at a specific index.

- **`void clearElements()`**
    Clears all elements from the layout.

---

### UIVerticalLayout

**Inherits:** [UILayout](#uilayout)

Vertical layout container with scroll support. Organizes UI elements vertically, one below another. Supports scrolling when content exceeds the visible viewport. Handles keyboard/D-pad navigation automatically with NES-style instant scroll.

#### Public Methods

- **`UIVerticalLayout(float x, float y, float w, float h)`**
    Constructs a new UIVerticalLayout.
    - `x, y`: Position of the layout container.
    - `w, h`: Width and height of the layout container (viewport height).

- **`void addElement(UIElement* element)`**
    Adds a UI element to the layout. The element will be positioned automatically.

- **`void removeElement(UIElement* element)`**
    Removes a UI element from the layout.

- **`void setScrollEnabled(bool enable)`**
    Enables or disables scrolling. When disabled, scroll offset is reset to 0.

- **`void enableScroll(bool enable)`**
    Alias for `setScrollEnabled()`.

- **`void setViewportHeight(float h)`**
    Sets the viewport height (visible area).

- **`float getScrollOffset() const`**
    Gets the current scroll offset in pixels.

- **`void setScrollOffset(float offset)`**
    Sets the scroll offset directly.

- **`float getContentHeight() const`**
    Gets the total content height (all elements combined).

- **`int getSelectedIndex() const`**
    Gets the currently selected element index (-1 if none selected).

- **`void setSelectedIndex(int index)`**
    Sets the selected element index. Automatically updates button styles and ensures the element is visible.

- **`UIElement* getSelectedElement() const`**
    Gets the currently selected element (nullptr if none selected).

- **`void setScrollSpeed(float speed)`**
    Sets the scroll speed for smooth scrolling (pixels per millisecond).

- **`void setNavigationButtons(uint8_t upButton, uint8_t downButton)`**
    Sets the navigation button indices for UP and DOWN navigation.

- **`void setButtonStyle(Color selectedTextCol, Color selectedBgCol, Color unselectedTextCol, Color unselectedBgCol)`**
    Sets the style colors for selected and unselected buttons. Automatically updates all button styles in the layout.

#### Example Usage

```cpp
// Create a vertical layout
UIVerticalLayout* layout = new UIVerticalLayout(10, 60, 220, 160);
layout->setPadding(5);
layout->setSpacing(6);
layout->setScrollEnabled(true);
layout->setNavigationButtons(0, 1); // UP=0, DOWN=1
layout->setButtonStyle(Color::White, Color::Cyan, Color::White, Color::Black);
layout->setRenderLayer(2);
addEntity(layout);

// Add buttons to the layout (no manual position calculation needed)
for (int i = 0; i < 20; i++) {
    UIButton* btn = new UIButton("Button " + std::to_string(i), 4, 0, 0, 120, 20, []() {
        // Button callback
    });
    layout->addElement(btn);
}

// Layout automatically handles:
// - Positioning elements vertically
// - Scroll when content exceeds viewport
// - Navigation (UP/DOWN)
// - Selection management
// - Button styling
```

#### Performance Notes

- **Viewport Culling**: Only visible elements are rendered, improving performance on ESP32.
- **Optimized Clearing**: The layout area is only cleared when scroll or selection changes, not every frame.
- **Instant Scroll**: Selection-based scrolling is instant (NES-style) for responsive navigation.

---

### UIHorizontalLayout

**Inherits:** [UILayout](#uilayout)

Horizontal layout container with scroll support. Organizes UI elements horizontally, one next to another. Supports scrolling when content exceeds the visible viewport. Handles keyboard/D-pad navigation automatically with NES-style instant scroll.

#### Public Methods

- **`UIHorizontalLayout(float x, float y, float w, float h)`**
    Constructs a new UIHorizontalLayout.
    - `x, y`: Position of the layout container.
    - `w, h`: Width and height of the layout container (viewport width).

- **`void addElement(UIElement* element)`**
    Adds a UI element to the layout. The element will be positioned automatically.

- **`void removeElement(UIElement* element)`**
    Removes a UI element from the layout.

- **`void setScrollEnabled(bool enable)`**
    Enables or disables scrolling. When disabled, scroll offset is reset to 0.

- **`void enableScroll(bool enable)`**
    Alias for `setScrollEnabled()`.

- **`void setViewportWidth(float w)`**
    Sets the viewport width (visible area).

- **`float getScrollOffset() const`**
    Gets the current scroll offset in pixels.

- **`void setScrollOffset(float offset)`**
    Sets the scroll offset directly.

- **`float getContentWidth() const`**
    Gets the total content width (all elements combined).

- **`int getSelectedIndex() const`**
    Gets the currently selected element index (-1 if none selected).

- **`void setSelectedIndex(int index)`**
    Sets the selected element index. Automatically updates button styles and ensures the element is visible.

- **`UIElement* getSelectedElement() const`**
    Gets the currently selected element (nullptr if none selected).

- **`void setScrollSpeed(float speed)`**
    Sets the scroll speed for smooth scrolling (pixels per millisecond).

- **`void setNavigationButtons(uint8_t leftButton, uint8_t rightButton)`**
    Sets the navigation button indices for LEFT and RIGHT navigation.

- **`void setButtonStyle(Color selectedTextCol, Color selectedBgCol, Color unselectedTextCol, Color unselectedBgCol)`**
    Sets the style colors for selected and unselected buttons. Automatically updates all button styles in the layout.

#### Example Usage

```cpp
// Create a horizontal layout (menu bar)
UIHorizontalLayout* menuBar = new UIHorizontalLayout(0, 0, 320, 30);
menuBar->setPadding(5);
menuBar->setSpacing(4);
menuBar->setScrollEnabled(true);
menuBar->setNavigationButtons(2, 3); // LEFT=2, RIGHT=3
menuBar->setButtonStyle(Color::White, Color::Cyan, Color::White, Color::Black);
menuBar->setRenderLayer(2);
addEntity(menuBar);

// Add buttons to the layout (no manual position calculation needed)
UIButton* fileBtn = new UIButton("File", 4, 0, 0, 60, 20, []() { /* ... */ });
UIButton* editBtn = new UIButton("Edit", 4, 0, 0, 60, 20, []() { /* ... */ });
UIButton* viewBtn = new UIButton("View", 4, 0, 0, 60, 20, []() { /* ... */ });
UIButton* helpBtn = new UIButton("Help", 4, 0, 0, 60, 20, []() { /* ... */ });

menuBar->addElement(fileBtn);
menuBar->addElement(editBtn);
menuBar->addElement(viewBtn);
menuBar->addElement(helpBtn);

// Layout automatically handles:
// - Positioning elements horizontally
// - Scroll when content exceeds viewport
// - Navigation (LEFT/RIGHT)
// - Selection management
// - Button styling
// - Vertical centering of elements
```

#### Performance Notes

- **Viewport Culling**: Only visible elements are rendered, improving performance on ESP32.
- **Optimized Clearing**: The layout area is only cleared when scroll or selection changes, not every frame.
- **Instant Scroll**: Selection-based scrolling is instant (NES-style) for responsive navigation.
- **Vertical Centering**: Elements smaller than the layout height are automatically centered vertically.

---

### UIGridLayout

**Inherits:** [UILayout](#uilayout)

Grid layout container for organizing elements in a matrix. Organizes UI elements in a fixed grid of rows and columns. Supports navigation in 4 directions (UP/DOWN/LEFT/RIGHT) and automatic positioning based on grid coordinates.

#### Public Methods

- **`UIGridLayout(float x, float y, float w, float h)`**
    Constructs a new UIGridLayout.
    - `x, y`: Position of the layout container.
    - `w, h`: Width and height of the layout container.

- **`void addElement(UIElement* element)`**
    Adds a UI element to the layout. The element will be positioned automatically based on its index in the grid.

- **`void removeElement(UIElement* element)`**
    Removes a UI element from the layout.

- **`void setColumns(uint8_t cols)`**
    Sets the number of columns in the grid. Must be > 0. Automatically recalculates the layout.

- **`uint8_t getColumns() const`**
    Gets the number of columns.

- **`uint8_t getRows() const`**
    Gets the number of rows (calculated automatically based on element count and columns).

- **`int getSelectedIndex() const`**
    Gets the currently selected element index (-1 if none selected).

- **`void setSelectedIndex(int index)`**
    Sets the selected element index. Automatically updates button styles.

- **`UIElement* getSelectedElement() const`**
    Gets the currently selected element (nullptr if none selected).

- **`void setNavigationButtons(uint8_t upButton, uint8_t downButton, uint8_t leftButton, uint8_t rightButton)`**
    Sets the navigation button indices for UP, DOWN, LEFT, and RIGHT navigation.

- **`void setButtonStyle(Color selectedTextCol, Color selectedBgCol, Color unselectedTextCol, Color unselectedBgCol)`**
    Sets the style colors for selected and unselected buttons. Automatically updates all button styles in the layout.

#### Example Usage

```cpp
// Create a grid layout for inventory (4 columns)
UIGridLayout* inventory = new UIGridLayout(10, 60, 220, 160);
inventory->setColumns(4);
inventory->setPadding(5);
inventory->setSpacing(4);
inventory->setNavigationButtons(0, 1, 2, 3); // UP=0, DOWN=1, LEFT=2, RIGHT=3
inventory->setButtonStyle(Color::White, Color::Cyan, Color::White, Color::Black);
inventory->setRenderLayer(2);
addEntity(inventory);

// Add items to the layout (automatically organized in 4 columns)
for (int i = 0; i < 16; i++) {
    UIButton* item = new UIButton("Item " + std::to_string(i), 4, 0, 0, 50, 50, []() {
        // Item selected callback
    });
    inventory->addElement(item);
}

// Layout automatically handles:
// - Positioning elements in grid (row = index / columns, col = index % columns)
// - Navigation (UP/DOWN/LEFT/RIGHT with wrapping)
// - Selection management
// - Button styling
// - Centering elements within cells
```

#### Performance Notes

- **Viewport Culling**: Only visible elements are rendered, improving performance on ESP32.
- **Automatic Cell Sizing**: Cell dimensions are calculated based on layout size, padding, and spacing.
- **Element Centering**: Elements smaller than their cell are automatically centered within the cell.
- **Navigation Wrapping**: Navigation wraps around edges (UP from first row goes to last row, etc.) for intuitive grid navigation.

---

### UIPaddingContainer

**Inherits:** [UIElement](#uielement)

Container that wraps a single UI element and applies padding. This container adds padding/margin around a single child element without organizing multiple elements. Useful for adding spacing to individual elements or nesting layouts with custom padding.

#### Public Methods

- **`UIPaddingContainer(float x, float y, float w, float h)`**
    Constructs a new UIPaddingContainer.
    - `x, y`: Position of the container.
    - `w, h`: Width and height of the container.

- **`void setChild(UIElement* element)`**
    Sets the child element to wrap. The child's position will be adjusted based on padding.

- **`UIElement* getChild() const`**
    Gets the child element (nullptr if none set).

- **`void setPadding(float p)`**
    Sets uniform padding on all sides.

- **`void setPadding(float left, float right, float top, float bottom)`**
    Sets asymmetric padding for each side.

- **`float getPaddingLeft() const`**
    Gets the left padding.

- **`float getPaddingRight() const`**
    Gets the right padding.

- **`float getPaddingTop() const`**
    Gets the top padding.

- **`float getPaddingBottom() const`**
    Gets the bottom padding.

#### Example Usage

```cpp
// Create a padding container with uniform padding
UIPaddingContainer* container = new UIPaddingContainer(10, 10, 200, 100);
container->setPadding(10);
container->setChild(button);
container->setRenderLayer(2);
addEntity(container);

// Or with asymmetric padding
UIPaddingContainer* container2 = new UIPaddingContainer(10, 10, 200, 100);
container2->setPadding(5, 15, 10, 10); // left, right, top, bottom
container2->setChild(layout);
addEntity(container2);

// Useful for nesting layouts with custom spacing
UIPaddingContainer* wrapper = new UIPaddingContainer(0, 0, 320, 240);
wrapper->setPadding(20);
UIVerticalLayout* innerLayout = new UIVerticalLayout(0, 0, 280, 200);
// ... add elements to innerLayout ...
wrapper->setChild(innerLayout);
addEntity(wrapper);
```

#### Performance Notes

- **No Reflow**: The container does not reorganize elements, only adjusts the child's position. Very efficient.
- **Automatic Position Updates**: When the container's position changes, the child's position is automatically updated.
- **Low Overhead**: Minimal performance impact, ideal for ESP32.

---

### UIPanel

**Inherits:** [UIElement](#uielement)

Visual container that draws a background and border around a child element. Provides a retro-style window/panel appearance. Typically contains a UILayout or other UI elements. Useful for dialogs, menus, and information panels.

#### Public Methods

- **`UIPanel(float x, float y, float w, float h)`**
    Constructs a new UIPanel.
    - `x, y`: Position of the panel.
    - `w, h`: Width and height of the panel.

- **`void setChild(UIElement* element)`**
    Sets the child element to wrap (typically a UILayout).

- **`UIElement* getChild() const`**
    Gets the child element (nullptr if none set).

- **`void setBackgroundColor(Color color)`**
    Sets the background color. Use `Color::Transparent` to disable background drawing.

- **`Color getBackgroundColor() const`**
    Gets the background color.

- **`void setBorderColor(Color color)`**
    Sets the border color. Use `Color::Transparent` to disable border drawing.

- **`Color getBorderColor() const`**
    Gets the border color.

- **`void setBorderWidth(uint8_t width)`**
    Sets the border width in pixels. Set to 0 to disable border.

- **`uint8_t getBorderWidth() const`**
    Gets the border width.

#### Example Usage

```cpp
// Create a dialog panel
UIPanel* dialog = new UIPanel(50, 50, 220, 140);
dialog->setBackgroundColor(Color::Black);
dialog->setBorderColor(Color::White);
dialog->setBorderWidth(2);
dialog->setRenderLayer(2);
addEntity(dialog);

// Add content layout inside the panel
UIVerticalLayout* content = new UIVerticalLayout(0, 0, 220, 140);
content->setPadding(10);
content->setSpacing(5);

UILabel* title = new UILabel("Dialog Title", 0, 0, Color::White, 2);
UIButton* okBtn = new UIButton("OK", 4, 0, 0, 100, 20, []() {
    // OK button callback
});

content->addElement(title);
content->addElement(okBtn);
dialog->setChild(content);

// Panel automatically draws:
// - Background (filled rectangle)
// - Border (4 filled rectangles for each side)
// - Child element (layout with buttons/labels)
```

#### Performance Notes

- **Efficient Rendering**: Background and border are drawn using simple filled rectangles, very efficient on ESP32.
- **Transparent Support**: Background and border can be disabled by setting colors to `Color::Transparent` or border width to 0.
- **Child Positioning**: Child element position is automatically updated when panel position changes.
- **Low Overhead**: Minimal performance impact, ideal for creating retro-style dialogs and menus.

---

### Anchor

**Enum:** Defines anchor points for positioning UI elements in `UIAnchorLayout`.

#### Values

- **`TOP_LEFT`**: Top-left corner
- **`TOP_RIGHT`**: Top-right corner
- **`BOTTOM_LEFT`**: Bottom-left corner
- **`BOTTOM_RIGHT`**: Bottom-right corner
- **`CENTER`**: Center of screen
- **`TOP_CENTER`**: Top center
- **`BOTTOM_CENTER`**: Bottom center
- **`LEFT_CENTER`**: Left center
- **`RIGHT_CENTER`**: Right center

---

### UIAnchorLayout

**Inherits:** [UILayout](#uilayout)

Layout that positions elements at fixed anchor points on the screen without reflow. Very efficient for HUDs, debug UI, and fixed-position elements. Positions are calculated once or when screen size changes.

#### Public Methods

- **`UIAnchorLayout(float x, float y, float w, float h)`**
    Constructs a new UIAnchorLayout.
    - `x, y`: Position of the layout container (usually 0, 0).
    - `w, h`: Width and height of the layout container (usually screen width and height).

- **`void addElement(UIElement* element, Anchor anchor)`**
    Adds a UI element with a specific anchor point.

- **`void addElement(UIElement* element)`**
    Adds a UI element with default anchor (TOP_LEFT).

- **`void removeElement(UIElement* element)`**
    Removes a UI element from the layout.

- **`void setScreenSize(float screenWidth, float screenHeight)`**
    Sets the screen size for anchor calculations. Automatically updates all element positions.

- **`float getScreenWidth() const`**
    Gets the screen width.

- **`float getScreenHeight() const`**
    Gets the screen height.

#### Example Usage

```cpp
// Create HUD with anchor layout
UIAnchorLayout* hud = new UIAnchorLayout(0, 0, screenWidth, screenHeight);
hud->setScreenSize(screenWidth, screenHeight);
hud->setRenderLayer(2);
addEntity(hud);

// Add HUD elements at different anchor points
UILabel* scoreLabel = new UILabel("Score: 0", 0, 0, Color::White, 1);
UILabel* livesLabel = new UILabel("Lives: 3", 0, 0, Color::White, 1);
UILabel* healthBar = new UILabel("Health: 100%", 0, 0, Color::Green, 1);
UILabel* debugInfo = new UILabel("FPS: 60", 0, 0, Color::Yellow, 1);

hud->addElement(scoreLabel, Anchor::TOP_LEFT);
hud->addElement(livesLabel, Anchor::TOP_RIGHT);
hud->addElement(healthBar, Anchor::BOTTOM_CENTER);
hud->addElement(debugInfo, Anchor::BOTTOM_LEFT);

// Elements are automatically positioned:
// - scoreLabel at (0, 0)
// - livesLabel at (screenWidth - livesLabel->width, 0)
// - healthBar centered at bottom
// - debugInfo at (0, screenHeight - debugInfo->height)
```

#### Performance Notes

- **No Reflow**: Positions are calculated once or when screen size changes. Very efficient on ESP32.
- **Fixed Positioning**: Elements maintain their anchor positions regardless of other elements.
- **HUD-Optimized**: Designed specifically for HUD elements that need fixed screen positions.
- **Low Overhead**: Minimal performance impact, ideal for ESP32.
