# API Reference

This document provides a complete reference for the PixelRoot32 Game Engine public API.

## Global Configuration

The `Config.h` file contains global configuration constants for the engine.

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

- **`void update(unsigned long deltaTime)`**
    Updates audio logic (envelopes, sequencers).

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

- **`void drawText(const char* text, int16_t x, int16_t y, uint16_t color, uint8_t size)`**
    Draws a string of text.

- **`void drawTextCentered(const char* text, int16_t y, uint16_t color, uint8_t size)`**
    Draws text centered horizontally at a given Y coordinate.

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

- **`void drawSprite(const Sprite4bpp& sprite, int x, int y, bool flipX = false)`**
    Available when `PIXELROOT32_ENABLE_4BPP_SPRITES` is defined. Draws a packed 4bpp sprite where each pixel stores a 4-bit index into the sprite-local palette. Index `0` is treated as transparent.

- **`void drawMultiSprite(const MultiSprite& sprite, int x, int y)`**
    Draws a layered sprite composed of multiple 1bpp `SpriteLayer` entries. Each layer is rendered in order using `drawSprite`, enabling multi-color NES/GameBoy-style sprites.

- **`void drawTileMap(const TileMap& map, int originX, int originY, Color color)`**
    Draws a tile-based background using a compact `TileMap` descriptor built on 1bpp `Sprite` tiles.

- **`void setDisplaySize(int w, int h)`**
    Sets the logical display size.

- **`void setDisplayOffset(int x, int y)`**
    Sets a global offset for all drawing operations.

- **`void setContrast(uint8_t level)`**
    Sets the display contrast/brightness (0-255).

- **`void setFont(const uint8_t* font)`**
    Sets the font for text rendering.

---

### Camera2D

**Inherits:** None

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

Enumeration of the 32 built-in colors available in the engine's palette.

#### Values

- `Black`, `White`, `LightGray`, `DarkGray`
- `Red`, `DarkRed`, `Green`, `DarkGreen`, `Blue`, `DarkBlue`
- `Yellow`, `Orange`, `Brown`
- `Purple`, `Pink`, `Cyan`
- `LightBlue`, `LightGreen`, `LightRed`
- `Navy`, `Teal`, `Olive`
- `Gold`, `Silver`
- `Transparent` (special value)
- `DebugRed`, `DebugGreen`, `DebugBlue`

#### Public Methods

- **`uint16_t resolveColor(Color color)`**
    Converts a `Color` enum value to its corresponding RGB565 `uint16_t` representation.

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

### TileMap

**Inherits:** None

Descriptor for compact tile-based backgrounds that reuse 1bpp `Sprite` tiles.

#### Properties

- **`uint8_t* indices`**  
  Pointer to a `width * height` array of tile indices. Each entry selects one tile from the `tiles` array.

- **`uint8_t width`**  
  Number of tiles horizontally.

- **`uint8_t height`**  
  Number of tiles vertically.

- **`const Sprite* tiles`**  
  Pointer to the first element of a `Sprite` tile set.

- **`uint8_t tileWidth`**  
  Tile width in pixels.

- **`uint8_t tileHeight`**  
  Tile height in pixels.

- **`uint16_t tileCount`**  
  Number of entries in the `tiles` array.

Typically used with `Renderer::drawTileMap` to render scroll-free backgrounds or simple starfields in a dedicated background render layer.

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

- **`UILabel(std::string t, float x, float y, uint16_t col, uint8_t sz)`**
    Constructs a label.

- **`void setText(const std::string& t)`**
    Updates the label's text.

- **`void centerX(int screenWidth)`**
    Centers the label horizontally.

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

#### Public Methods

- **`UIElement(float x, float y, float w, float h)`**
    Constructs a new UIElement.

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
