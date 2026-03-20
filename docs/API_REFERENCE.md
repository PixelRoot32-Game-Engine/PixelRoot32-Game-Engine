# API Reference

This document provides a complete reference for the PixelRoot32 Game Engine public API.

> **Note:** For the most up-to-date and comprehensive API documentation with examples and cross-references, visit the [official documentation](https://docs.pixelroot32.org/api_reference/).

## Global Configuration

The engine's behavior can be customized using `platforms/PlatformDefaults.h` and `platforms/EngineConfig.h`, or via compile-time build flags. This allows for fine-tuning performance and hardware support without modifying the core engine code.

For detailed platform-specific capabilities and limitations, see [Platform Compatibility Guide](PLATFORM_COMPATIBILITY.md).

### Platform Macros (Build Flags)

| Macro | Description | Default (ESP32) |
|-------|-------------|-----------------|
| `PR32_DEFAULT_AUDIO_CORE` | CPU core assigned to audio tasks. | `0` |
| `PR32_DEFAULT_MAIN_CORE` | CPU core assigned to the main game loop. | `1` |
| `PIXELROOT32_NO_DAC_AUDIO` | Disable Internal DAC support on classic ESP32. | Enabled |
| `PIXELROOT32_NO_I2S_AUDIO` | Disable I2S audio support. | Enabled |
| `PIXELROOT32_USE_U8G2_DRIVER` | Enable U8G2 display driver support for monochromatic OLEDs. | Disabled |
| `PIXELROOT32_NO_TFT_ESPI` | Disable default TFT_eSPI driver support. | Enabled |

### Modular Compilation Flags

| Macro | Description | Default |
|-------|-------------|---------|
| `PIXELROOT32_ENABLE_AUDIO` | Enable audio subsystem (AudioEngine + MusicPlayer). | `1` |
| `PIXELROOT32_ENABLE_PHYSICS` | Enable physics system (CollisionSystem). | `1` |
| `PIXELROOT32_ENABLE_UI_SYSTEM` | Enable UI system (UIButton, UILabel, etc.). | `1` |
| `PIXELROOT32_ENABLE_PARTICLES` | Enable particle system. | `1` |
| `PIXELROOT32_ENABLE_DEBUG_OVERLAY` | Enable FPS/RAM/CPU debug overlay. | Disabled |
| `PIXELROOT32_ENABLE_TILE_ANIMATIONS` | Enable tile animation system. | `1` |
| `PIXELROOT32_ENABLE_2BPP_SPRITES` | Enable 2bpp sprite support. | Disabled |
| `PIXELROOT32_ENABLE_4BPP_SPRITES` | Enable 4bpp sprite support. | Disabled |
| `PIXELROOT32_ENABLE_SCENE_ARENA` | Enable scene memory arena. | Disabled |
| `PIXELROOT32_ENABLE_PROFILING` | Enable profiling hooks in physics pipeline. | Disabled |
| `PIXELROOT32_DEBUG_MODE` | Enable unified logging system. | Disabled |

**Memory savings by subsystem (approximate):**

| Subsystem Disabled | RAM Savings | Flash Savings |
|-------------------|-------------|--------------|
| `PIXELROOT32_ENABLE_AUDIO=0` | ~8 KB | ~15 KB |
| `PIXELROOT32_ENABLE_PHYSICS=0` | ~12 KB | ~25 KB |
| `PIXELROOT32_ENABLE_UI_SYSTEM=0` | ~4 KB | ~20 KB |
| `PIXELROOT32_ENABLE_PARTICLES=0` | ~6 KB | ~10 KB |

**Build profiles (platformio.ini):**

```ini
[profile_full]         ; All features enabled
build_flags =
    -D PIXELROOT32_ENABLE_AUDIO=1
    -D PIXELROOT32_ENABLE_PHYSICS=1
    -D PIXELROOT32_ENABLE_PARTICLES=1
    -D PIXELROOT32_ENABLE_UI_SYSTEM=1

[profile_arcade]       ; Audio + Physics + Particles, no UI
build_flags =
    -D PIXELROOT32_ENABLE_AUDIO=1
    -D PIXELROOT32_ENABLE_PHYSICS=1
    -D PIXELROOT32_ENABLE_PARTICLES=1
    -D PIXELROOT32_ENABLE_UI_SYSTEM=0

[profile_puzzle]       ; Audio + UI only, no physics/particles
build_flags =
    -D PIXELROOT32_ENABLE_AUDIO=1
    -D PIXELROOT32_ENABLE_PHYSICS=0
    -D PIXELROOT32_ENABLE_PARTICLES=0
    -D PIXELROOT32_ENABLE_UI_SYSTEM=1

[profile_retro]        ; Minimal: no subsystems
build_flags =
    -D PIXELROOT32_ENABLE_AUDIO=0
    -D PIXELROOT32_ENABLE_PHYSICS=0
    -D PIXELROOT32_ENABLE_PARTICLES=0
    -D PIXELROOT32_ENABLE_UI_SYSTEM=0
```

**Recommended profiles by game type:**

| Game Type | Recommended Profile | Rationale |
|-----------|-------------------|-----------|
| Arcade (shooters, platformers) | `arcade` or `full` | Physics + particles + optional UI |
| Puzzle / Casual | `puzzle` | UI for menus, simple collision logic |
| Retro / Minimal | `retro` | Minimal footprint, custom collision |
| Educational / Tool | `puzzle` or custom | UI for menus |

### Constants

- **`DISPLAY_WIDTH`**
    The width of the display in pixels. Default is `240`.

- **`DISPLAY_HEIGHT`**
    The height of the display in pixels. Default is `240`.

- **`int xOffset`**
    The horizontal offset for the display alignment. Default is `0`.

- **`int yOffset`**
    The vertical offset for the display alignment. Default is `0`.

- **`PHYSICS_MAX_PAIRS`**
    Maximum number of collision pairs considered in broadphase. Default is `128`.

- **`PHYSICS_MAX_CONTACTS`**
    Maximum number of simultaneous contacts in the solver (fixed pool, no heap per frame). Default is `128`. When exceeded, additional contacts are dropped.

- **`VELOCITY_ITERATIONS`**
    Number of impulse solver passes per frame. Higher values improve stacking stability but increase CPU load. Default is `2`.

- **`SPATIAL_GRID_CELL_SIZE`**
    Size of each cell in the broadphase grid (in pixels). Default is `32`.

- **`SPATIAL_GRID_MAX_ENTITIES_PER_CELL`**
    Legacy: maximum entities per cell when using a single grid. Default is `24`.

- **`SPATIAL_GRID_MAX_STATIC_PER_CELL`**
    Maximum static (immovable) actors per grid cell. Default is `12`. Used by the static layer of the spatial grid.

- **`SPATIAL_GRID_MAX_DYNAMIC_PER_CELL`**
    Maximum dynamic (RIGID/KINEMATIC) actors per grid cell. Default is `12`. Used by the dynamic layer of the spatial grid.

## Math Module

The Math module provides a platform-agnostic numerical abstraction layer (`Scalar`) that automatically selects the most efficient representation (`float` or `Fixed16`) based on the target hardware's capabilities (FPU presence).

### Scalar

**Namespace:** `pixelroot32::math`

`Scalar` is the fundamental numeric type used throughout the engine for physics, positioning, and logic.

- **On FPU platforms (ESP32, S3):** `Scalar` is an alias for `float`.
- **On non-FPU platforms (C3, S2, C6):** `Scalar` is an alias for `Fixed16`.

#### Fixed16 (16.16 Fixed Point)

On platforms without a Hardware Floating Point Unit (FPU), the engine uses `Fixed16` for all calculations.

- **Storage**: 32-bit signed integer.
- **Precision**: 16 bits for the integer part, 16 bits for the fractional part (approx. 0.000015 resolution).
- **Literal**: Use the `_fp` suffix for literals on non-FPU platforms for compile-time conversion.
  *Example:* `Scalar gravity = 9.8_fp;`

#### Helper Functions

- **`Scalar toScalar(float value)`**
    Converts a floating-point literal or variable to `Scalar`.
    *Usage:* `Scalar speed = toScalar(2.5f);`

- **`Scalar toScalar(int value)`**
    Converts an integer to `Scalar`.

- **`int toInt(Scalar value)`**
    Converts a `Scalar` back to an integer (truncating decimals).

- **`int roundToInt(Scalar value)`**
    Converts a `Scalar` to an integer, rounding to the nearest whole number. Essential for mapping logical positions to pixel coordinates without jitter.

- **`int floorToInt(Scalar value)`**
    Returns the largest integer less than or equal to the scalar value.

- **`int ceilToInt(Scalar value)`**
    Returns the smallest integer greater than or equal to the scalar value.

- **`float toFloat(Scalar value)`**
    Converts a `Scalar` to `float`. **Warning:** Use sparingly on non-FPU platforms.

- **`Scalar abs(Scalar v)`**
    Returns the absolute value.

- **`Scalar sqrt(Scalar v)`**
    Returns the square root. **Warning:** Expensive operation. Prefer squared distances for comparisons.

- **`Scalar min(Scalar a, Scalar b)`**
    Returns the smaller of two values.

- **`Scalar max(Scalar a, Scalar b)`**
    Returns the larger of two values.

- **`Scalar clamp(Scalar v, Scalar minVal, Scalar maxVal)`**
    Clamps a value between a minimum and maximum.

- **`Scalar lerp(Scalar a, Scalar b, Scalar t)`**
    Linearly interpolates between `a` and `b` by `t` (where `t` is 0.0 to 1.0).

- **`Scalar sin(Scalar x)`**
    Returns the sine of the angle `x` (in radians).

- **`Scalar cos(Scalar x)`**
    Returns the cosine of the angle `x` (in radians).

- **`Scalar atan2(Scalar y, Scalar x)`**
    Returns the arc tangent of y/x (in radians).

- **`Scalar sign(Scalar x)`**
    Returns the sign of x (-1, 0, or 1).

- **`bool is_equal_approx(Scalar a, Scalar b)`**
    Returns true if a and b are approximately equal.

- **`bool is_zero_approx(Scalar x)`**
    Returns true if x is approximately zero.

#### Constants

- **`Scalar kPi`**
    Value of PI (3.14159...).

- **`Scalar kDegToRad`**
    Conversion factor from degrees to radians.

- **`Scalar kRadToDeg`**
    Conversion factor from radians to degrees.

### Vector2

**Namespace:** `pixelroot32::math`

A 2D vector structure composed of two `Scalar` components.

#### Members

- **`Scalar x`**
- **`Scalar y`**

#### Methods

- **`Vector2(Scalar x, Scalar y)`**
    Constructor.

- **`Scalar lengthSquared() const`**
    Returns the squared magnitude of the vector. **Preferred over `length()` for comparisons.**

- **`Scalar length() const`**
    Returns the magnitude of the vector.

- **`Vector2 normalized() const`**
    Returns a normalized (unit length) version of the vector.

---


### MathUtil

**Inherits:** None

Collection of helper functions.

#### Public Methods

- **`float lerp(float a, float b, float t)`**
    Linear interpolation.
- **`float clamp(float v, float min, float max)`**
    Clamps a value between min and max.

- **`Scalar dot(const Vector2& other) const`**
    Returns the dot product with another vector.

- **`Scalar cross(const Vector2& other) const`**
    Returns the cross product with another vector (2D analog).

- **`Scalar angle() const`**
    Returns the angle of the vector in radians.

- **`Scalar angle_to(const Vector2& to) const`**
    Returns the angle to another vector in radians.

- **`Scalar angle_to_point(const Vector2& to) const`**
    Returns the angle from this point to another point.

- **`Vector2 direction_to(const Vector2& to) const`**
    Returns the normalized direction vector pointing to the target.

- **`Scalar distance_to(const Vector2& to) const`**
    Returns the distance to another point.

- **`Scalar distance_squared_to(const Vector2& to) const`**
    Returns the squared distance to another point.

- **`Vector2 limit_length(Scalar max_len) const`**
    Returns the vector with its length limited to `max_len`.

- **`Vector2 clamp(Vector2 min, Vector2 max) const`**
    Returns the vector clamped between min and max vectors.

- **`Vector2 lerp(const Vector2& to, Scalar weight) const`**
    Linear interpolation between this vector and `to`.

- **`Vector2 rotated(Scalar phi) const`**
    Returns the vector rotated by `phi` radians.

- **`Vector2 move_toward(const Vector2& to, Scalar delta) const`**
    Moves the vector toward `to` by a maximum of `delta` distance.

- **`Vector2 slide(const Vector2& n) const`**
    Returns the component of the vector along the sliding plane defined by normal `n`.

- **`Vector2 reflect(const Vector2& n) const`**
    Returns the vector reflected across the plane defined by normal `n`.

- **`Vector2 project(const Vector2& b) const`**
    Returns the projection of this vector onto vector `b`.

- **`Vector2 abs() const`**
    Returns a new vector with absolute values of components.

- **`Vector2 sign() const`**
    Returns a new vector with sign of components.

- **`bool is_normalized() const`**
    Returns true if the vector is normalized.

- **`bool is_zero_approx() const`**
    Returns true if the vector is approximately zero.

- **`bool is_equal_approx(const Vector2& other) const`**
    Returns true if the vector is approximately equal to `other`.

## Platform Abstractions

Version 1.1.0 introduces unified abstractions for cross-platform operations, eliminating the need for manual `#ifdef` blocks in user code.

### Logging System

**Namespace:** `pixelroot32::core::logging`

The unified logging system provides platform-agnostic logging with different log levels, automatically routing to the appropriate output (Serial for ESP32, stdout for native). Enable with `-DPIXELROOT32_DEBUG_MODE` in build flags.

#### Log Levels

| LogLevel Enum | Output Prefix | Use Case |
|--------------|---------------|----------|
| `LogLevel::Info` | `[INFO]` | General information, debug messages |
| `LogLevel::Profiling` | `[PROF]` | Performance timing markers |
| `LogLevel::Warning` | `[WARN]` | Warnings, non-critical issues |
| `LogLevel::Error` | `[ERROR]` | Errors, critical failures |

#### Functions

- **`void log(LogLevel level, const char* format, ...)`**
    Logs a message with the specified level and printf-style formatting.

- **`void log(const char* format, ...)`**
    Logs a message with Info level (shorthand).

#### Conditional Compilation

When `PIXELROOT32_DEBUG_MODE` is **not defined**, all `log()` calls become no-ops at compile time. The engine uses a double-layer conditional:

1. **`#ifdef PIXELROOT32_DEBUG_MODE`** in the header makes `log()` calls emit formatting code
2. **`if constexpr (EnableLogging)`** in the implementation skips runtime formatting

This means zero runtime cost in production builds (no string formatting, no branching).

#### Usage Example:

```cpp
// Enable in platformio.ini:
// build_flags = -DPIXELROOT32_DEBUG_MODE

#include "core/Log.h"

using namespace pixelroot32::core::logging;

// Log with explicit level
log(LogLevel::Info, "Player position: %d", playerX);

// Log warning
log(LogLevel::Warning, "Low memory: %d bytes free", freeRAM);

// Log error
log(LogLevel::Error, "Failed to load sprite: %s", filename);

// Log with default Info level
log("Player position: %d", playerX);
```

### Platform Memory Abstraction

**Include:** `platforms/PlatformMemory.h`

Provides a unified API for memory operations that differ between ESP32 (Flash/PROGMEM) and Native (RAM) platforms.

#### Macros

- **`PIXELROOT32_FLASH_ATTR`**
    Attribute for data stored in Flash memory.

- **`PIXELROOT32_STRCMP_P(dest, src)`**
    Compare a RAM string with a Flash string.

- **`PIXELROOT32_MEMCPY_P(dest, src, size)`**
    Copy data from Flash to RAM.

- **`PIXELROOT32_READ_BYTE_P(addr)`**
    Read an 8-bit value from Flash.

- **`PIXELROOT32_READ_WORD_P(addr)`**
    Read a 16-bit value from Flash.

- **`PIXELROOT32_READ_DWORD_P(addr)`**
    Read a 32-bit value from Flash.

- **`PIXELROOT32_READ_FLOAT_P(addr)`**
    Read a float value from Flash.

- **`PIXELROOT32_READ_PTR_P(addr)`**
    Read a pointer from Flash.

**Usage Example:**

```cpp
#include "platforms/PlatformMemory.h"

const char MY_STRING[] PIXELROOT32_FLASH_ATTR = "Hello";
char buffer[10];
PIXELROOT32_STRCMP_P(buffer, MY_STRING);
uint8_t val = PIXELROOT32_READ_BYTE_P(&my_array[i]);
```

## Core Module

The Core module provides the fundamental building blocks of the engine, including the main application loop, entity management, and scene organization.

### Engine

**Inherits:** None

The main engine class that manages the game loop and core subsystems. `Engine` acts as the central hub, initializing and managing the Renderer, InputManager, and SceneManager. It runs the main game loop, handling timing (delta time), updating the current scene, and rendering frames.

#### Public Methods

- **`Engine(DisplayConfig&& displayConfig, const InputConfig& inputConfig, const AudioConfig& audioConfig)`**
    Constructs the engine with custom display, input and audio configurations.

- **`Engine(DisplayConfig&& displayConfig, const InputConfig& inputConfig)`**
    Constructs the engine with custom display and input configurations.

- **`Engine(DisplayConfig&& displayConfig)`**
    Constructs the engine with custom display configuration and default input settings.

- **`void init()`**
    Initializes the engine subsystems. Must be called before `run()`.

- **`void run()`**
    Starts the main game loop. This method contains the infinite loop that calls `update()` and `draw()` repeatedly.

- **`unsigned long getDeltaTime() const`**
    Returns the time elapsed since the last frame in milliseconds.

- **`void setScene(Scene* newScene)`**
    Sets the current active scene.

- **`std::optional<Scene*> getCurrentScene() const`**
    Retrieves the currently active scene, or std::nullopt if no scene is active.

- **`Renderer& getRenderer()`**
    Provides access to the Renderer subsystem.

- **`InputManager& getInputManager()`**
    Provides access to the InputManager subsystem.

- **`AudioEngine& getAudioEngine()`**
    Provides access to the AudioEngine subsystem.
  - **Note**: Only available if `PIXELROOT32_ENABLE_AUDIO=1`

- **`MusicPlayer& getMusicPlayer()`**
    Provides access to the MusicPlayer subsystem.
  - **Note**: Only available if `PIXELROOT32_ENABLE_AUDIO=1`

- **`const PlatformCapabilities& getPlatformCapabilities() const`**
    Returns the detected hardware capabilities for the current platform.

### PlatformCapabilities (Struct)

**Namespace:** `pixelroot32::platforms`

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

### DisplayConfig (Struct)

Configuration settings for display initialization and scaling.

- **`DisplayType type`**: Type of display (ST7789, ST7735, OLED_SSD1306, OLED_SH1106, NONE, CUSTOM).
- **`int rotation`**: Display rotation (0-3 or degrees).
- **`uint16_t physicalWidth`**: Actual hardware width.
- **`uint16_t physicalHeight`**: Actual hardware height.
- **`uint16_t logicalWidth`**: Virtual rendering width.
- **`uint16_t logicalHeight`**: Virtual rendering height.
- **`int xOffset`**: X coordinate offset for hardware alignment.
- **`int yOffset`**: Y coordinate offset for hardware alignment.

#### Pin Configuration (Optional)

- **`uint8_t clockPin`**: SPI SCK / I2C SCL.
- **`uint8_t dataPin`**: SPI MOSI / I2C SDA.
- **`uint8_t csPin`**: SPI CS (Chip Select).
- **`uint8_t dcPin`**: SPI DC (Data/Command).
- **`uint8_t resetPin`**: Reset pin.
- **`bool useHardwareI2C`**: If true, uses hardware I2C peripheral (default true).

---

## Graphics Module

The Graphics module manages rendering, sprites, tilemaps, and associated metadata.

### Tile Attribute System

The tile attribute system allows querying custom metadata attached to tiles at runtime.

#### Data Structures

- **`TileAttribute`**
    Represents a single key-value pair.
    - `const char* key`: Attribute key string (`PIXELROOT32_FLASH_ATTR`).
    - `const char* value`: Attribute value string (`PIXELROOT32_FLASH_ATTR`).

- **`TileAttributeEntry`**
    Represents all attributes for a specific tile position.
    - `uint16_t x, y`: Tile coordinates.
    - `uint8_t num_attributes`: Number of attributes for this tile.
    - `const TileAttribute* attributes`: Pointer to an array of attributes (`PIXELROOT32_FLASH_ATTR`).

- **`LayerAttributes`**
    Represents attribute data for an entire layer.
    - `const char* layer_name`: Name of the layer.
    - `uint16_t num_tiles_with_attributes`: Number of tiles in this layer that have attributes.
    - `const TileAttributeEntry* tiles`: Pointer to an array of tile entries (`PIXELROOT32_FLASH_ATTR`).

#### Query Functions

**Namespace:** `pixelroot32::graphics`

These functions are defined as `inline` in `Renderer.h` for maximum performance and use the platform abstraction layer from `platforms/PlatformMemory.h`.

- **`const char* get_tile_attribute(const LayerAttributes* layers, uint8_t num_layers, uint8_t layer_idx, uint16_t x, uint16_t y, const char* key)`**
    Returns the value of a specific attribute for a tile. Returns `nullptr` if the attribute or tile does not exist.
    - `layers`: Pointer to the `layer_attributes` array.
    - `num_layers`: Total number of layers with attributes.
    - `layer_idx`: Index of the layer to query.
    - `x, y`: Tile coordinates.
    - `key`: The attribute key to search for (supports RAM or PROGMEM/Flash strings).

> [!IMPORTANT]
> Since attributes are stored in Flash memory on ESP32, you must use **`PIXELROOT32_STRCMP_P`** or **`PIXELROOT32_MEMCPY_P`** to compare or copy the returned values to ensure cross-platform compatibility (Native vs ESP32).

- **`bool tile_has_attributes(const LayerAttributes* layers, uint8_t num_layers, uint8_t layer_idx, uint16_t x, uint16_t y)`**
    Returns `true` if the tile at the specified position has any attributes.

- **`const TileAttributeEntry* get_tile_entry(const LayerAttributes* layers, uint8_t num_layers, uint8_t layer_idx, uint16_t x, uint16_t y)`**
    Returns a pointer to the entire `TileAttributeEntry` for a tile. Useful for iterating through all attributes of a tile or performing multiple queries efficiently. Returns `nullptr` if the tile has no attributes.

---

## Audio Module

The Audio module provides a NES-like audio system with Pulse, Triangle, and Noise
channels, plus a lightweight melody subsystem for background music.

### AudioEngine

**Inherits:** None

The core class managing audio generation and playback.

#### Public Methods

- **`AudioEngine(const AudioConfig& config, const PlatformCapabilities& caps)`**
    Constructs the AudioEngine with specific configuration and platform capabilities.

- **`AudioEngine(const AudioConfig& config)`**
    Constructs the AudioEngine with a specific configuration.

- **`void init()`**
    Initializes the audio backend.

- **`void generateSamples(int16_t* stream, int length)`**
    Fills the buffer with audio samples.

- **`void playEvent(const AudioEvent& event)`**
    Plays a one-shot audio event on the first available channel of the requested type.

- **`void setMasterVolume(float volume)`**
    Sets the master volume level (0.0 to 1.0).

- **`void setScheduler(std::shared_ptr<AudioScheduler> scheduler)`**
    Sets a custom audio scheduler. For advanced use.

- **`void submitCommand(const Command& command)`**
    Submits a low-level command to the audio thread. For advanced use.

- **`float getMasterVolume() const`**
    Gets the current master volume level.

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
Music timing is handled internally by the `AudioEngine`.

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

- **`bool isPlaying() const`**
    Returns true if a track is currently playing and not finished.

- **`void setTempoFactor(float factor)`**
    Sets the global tempo scaling factor.
  - `1.0f` is normal speed.
  - `2.0f` is double speed.
  - `0.5f` is half speed.

- **`float getTempoFactor() const`**
    Gets the current tempo scaling factor (default 1.0f).

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

- **`Scalar x, y`**: Position in world space.
- **`int width, height`**: Dimensions of the entity.
- **`bool isVisible`**: If false, the entity is skipped during rendering.
- **`bool isEnabled`**: If false, the entity is skipped during updates.
- **`unsigned char renderLayer`**: Logical render layer for this entity (0 = background, 1 = gameplay, 2 = UI).

#### Public Methods

- **`Entity(Scalar x, Scalar y, int w, int h, EntityType t)`**
    Constructs a new Entity.

- **`void setVisible(bool v)`**
    Sets the visibility of the entity.

- **`void setEnabled(bool e)`**
    Sets the enabled state of the entity.

- **`unsigned char getRenderLayer() const`**
    Returns the current render layer.

- **`void setRenderLayer(unsigned char layer)`**
    Sets the logical render layer for this entity. The value is clamped to the range `[0, MAX_LAYERS - 1]`.

- **`virtual void update(unsigned long deltaTime)`**
    Updates the entity's logic. Must be overridden by subclasses.

- **`virtual void draw(Renderer& renderer)`**
    Renders the entity. Must be overridden by subclasses.

#### Modular Compilation Notes

The Entity class is always available. However, specialized subclasses may be affected by modular compilation flags:

- **UI Elements** (UIButton, UILabel, etc.): Only available if `PIXELROOT32_ENABLE_UI_SYSTEM=1`
- **Physics Actors** (PhysicsActor, RigidActor, etc.): Only available if `PIXELROOT32_ENABLE_PHYSICS=1`
- **Particle Systems**: Only available if `PIXELROOT32_ENABLE_PARTICLES=1`

### Actor

**Inherits:** [Entity](#entity)

The base class for all objects capable of collision. Actors extend Entity with collision layers, masks, and shape definitions. Note: You should typically use a specialized subclass like `RigidActor` or `KinematicActor` instead of this base class.

#### Constants

- **`enum CollisionShape`**
  - `AABB`: Axis-aligned bounding box (Rectangle).
  - `CIRCLE`: Circular collision body.

#### Properties

- **`uint16_t entityId`**: Unique id assigned by `CollisionSystem::addEntity` (used for pair deduplication). `0` = unregistered.
- **`int queryId`**: Used internally by the spatial grid for deduplication in `getPotentialColliders`.
- **`CollisionLayer layer`**: Bitmask representing the layers this actor belongs to.
- **`CollisionLayer mask`**: Bitmask representing the layers this actor scans for collisions.

#### Public Methods

- **`Actor(Scalar x, Scalar y, int w, int h)`**
    Constructs a new Actor.

- **`void setCollisionLayer(CollisionLayer l)`**
    Sets the collision layer this actor belongs to.

- **`void setCollisionMask(CollisionLayer m)`**
    Sets the collision layers this actor interacts with.

- **`bool isInLayer(uint16_t targetLayer) const`**
    Checks if the Actor belongs to a specific collision layer.

- **`virtual Rect getHitBox()`**
    Returns the bounding rectangle for AABB detection or the bounding box of the circle.

- **`virtual void onCollision(Actor* other)`**
    Callback invoked when a collision is detected. **Note:** All collision responses (velocity/position changes) are handled by the `CollisionSystem`. This method is for gameplay notifications only.

---

## Physics Module

The Physics module provides a high-performance "Flat Solver" optimized for microcontrollers. It handles collision detection, position resolution, and physical integration for different types of bodies.

### PhysicsActor

**Inherits:** [Actor](#actor)

Base class for all physics-enabled bodies. It provides the core integration and response logic used by the `CollisionSystem`.

#### Properties

- **`Vector2 velocity`**: Current movement speed in pixels/second.
- **`Vector2 previousPosition`**: Position from the previous physics frame (used for spatial crossing detection).
- **`Scalar mass`**: Mass of the body (Default: `1.0`).
- **`Scalar restitution`**: Bounciness factor (0.0 = no bounce, 1.0 = perfect bounce).
- **`Scalar friction`**: Friction coefficient (not yet fully implemented in solver).
- **`Scalar gravityScale`**: Multiplier for global gravity (Default: `1.0`).
- **`CollisionShape shape`**: The geometric shape used for detection (Default: `AABB`).
- **`Scalar radius`**: Radius used if `shape` is `CIRCLE` (Default: `0`).
- **`bool bounce`**: If `true`, the actor will bounce off surfaces based on its restitution (Default: `false`).
- **`bool sensor`**: When true, the body generates collision events but does not produce physical response (no impulse, no penetration correction). Use for triggers, collectibles.
- **`bool oneWay`**: When true, the body only blocks from one side (e.g. one-way platform: land from above, pass through from below).
- **`void* userData`**: Optional pointer or packed value (e.g. tile coordinates) for game logic. Use `physics::packTileData` / `unpackTileData` from `physics/TileAttributes.h` for tile metadata.

#### Constructors

- **`PhysicsActor(Scalar x, Scalar y, int w, int h)`**
    Constructs a new PhysicsActor.

- **`PhysicsActor(Vector2 position, int w, int h)`**
    Constructs a new PhysicsActor using a position vector.

#### Public Methods

- **`void update(unsigned long deltaTime)`**
    Updates the actor state, applying physics integration and checking world boundary collisions.

- **`void setVelocity(const Vector2& v)`**
    Sets the linear velocity of the actor. Also supports `(Scalar x, Scalar y)` and `(float x, float y)`.

- **`const Vector2& getVelocity() const`**
    Gets the current velocity vector.

- **`void updatePreviousPosition()`**
    Updates the previous position to the current position. Should be called at the start of each physics frame to track position history for spatial crossing detection (e.g., one-way platforms). This is automatically called by `CollisionSystem::update()`.

- **`Vector2 getPreviousPosition() const`**
    Gets the position from the previous physics frame. Used internally for one-way platform validation.

- **`void setPosition(Vector2 pos)`**
    Sets the position and syncs previous position. When position is set directly (not via physics integration), the previous position is also updated to prevent false crossing detection.

- **`void setRestitution(Scalar r)`**
    Sets the restitution (bounciness). 1.0 means perfect bounce, < 1.0 means energy loss.

- **`void setFriction(Scalar f)`**
    Sets the friction coefficient (0.0 means no friction).

- **`void setSensor(bool s)`**
    Sets whether this body is a sensor (trigger). Sensors fire `onCollision` but do not receive impulse or penetration correction.

- **`bool isSensor() const`**
    Returns true if this body is a sensor.

- **`void setOneWay(bool w)`**
    Sets whether this body is a one-way platform (blocks only from one side, e.g. from above).

- **`bool isOneWay() const`**
    Returns true if this body is a one-way platform.

- **`void setUserData(void* ptr)`**
    Sets optional user data (e.g. tile coordinates or game-specific pointer).

- **`void* getUserData() const`**
    Gets the current user data.

- **`void setLimits(const LimitRect& limits)`**
    Sets custom movement limits for the actor.

- **`void setWorldBounds(int w, int h)`**
    Defines the world size for boundary checking, used as default limits.

- **`WorldCollisionInfo getWorldCollisionInfo() const`**
    Gets information about collisions with the world boundaries.

- **`void resetWorldCollisionInfo()`**
    Resets the world collision flags for the current frame.

---

### LimitRect

**Inherits:** None

Bounding rectangle for world-collision resolution. Defines the limits of the play area.

#### Properties

- **`left`**: Left boundary (-1 for no limit).
- **`top`**: Top boundary (-1 for no limit).
- **`right`**: Right boundary (-1 for no limit).
- **`bottom`**: Bottom boundary (-1 for no limit).

#### Constructors

- **`LimitRect(int l, int t, int r, int b)`**
    Constructs a LimitRect with specific bounds.

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

### StaticActor

**Inherits:** [PhysicsActor](#physicsactor)

An immovable body that other objects can collide with. Ideal for floors, walls, and level geometry. Static bodies are placed in the **static layer** of the spatial grid (rebuilt only when entities are added or removed), reducing per-frame cost in levels with many tiles.

#### Constructors

- **`StaticActor(Scalar x, Scalar y, int w, int h)`**
    Constructs a new StaticActor.

- **`StaticActor(Vector2 position, int w, int h)`**
    Constructs a new StaticActor using a position vector.

**Example:**

```cpp
auto floor = std::make_unique<StaticActor>(0, 230, 240, 10);
floor->setCollisionLayer(Layers::kWall);
scene->addEntity(floor.get());
```

---

### SensorActor

**Inherits:** [StaticActor](#staticactor)

A static body that acts as a **trigger**: it generates `onCollision` callbacks but does not produce any physical response (no impulse, no penetration correction). Use for collectibles, checkpoints, damage zones, or area triggers.

**Include:** `physics/SensorActor.h`

**Constructors:** Same as `StaticActor`; internally calls `setSensor(true)`.

```cpp
SensorActor coin(x, y, 16, 16);
coin.setCollisionLayer(Layers::kCollectible);
scene->addEntity(&coin);
// In player's onCollision: if (other->isSensor()) { collectCoin(other); }
```

---

### KinematicActor

**Inherits:** [PhysicsActor](#physicsactor)

A body that is moved manually via code but still interacts with the physics world (stops at walls, pushes objects). Ideal for players and moving platforms.

#### Constructors

- **`KinematicActor(Scalar x, Scalar y, int w, int h)`**
    Constructs a new KinematicActor.

- **`KinematicActor(Vector2 position, int w, int h)`**
    Constructs a new KinematicActor using a position vector.

#### Public Methods

- **`bool moveAndCollide(Vector2 relativeMove)`**
    Moves the actor by `relativeMove`. If a collision occurs, it stops at the point of contact and returns `true`.
- **`Vector2 moveAndSlide(Vector2 velocity)`**
    Moves the actor, sliding along surfaces if it hits a wall or floor. Returns the remaining velocity.

- **`bool is_on_ceiling() const`**
    Returns true if the body collided with the ceiling during the last `moveAndSlide` call.

- **`bool is_on_floor() const`**
    Returns true if the body collided with the floor during the last `moveAndSlide` call.

- **`bool is_on_wall() const`**
    Returns true if the body collided with a wall during the last `moveAndSlide` call.

**Example:**

```cpp
void Player::update(unsigned long dt) {
    Vector2 motion(0, 0);
    if (input.isButtonDown(0)) motion.x += 100 * dt / 1000.0f;
    
    // Automatic sliding against walls
    moveAndSlide(motion);
}
```

---

### RigidActor

**Inherits:** [PhysicsActor](#physicsactor)

A body fully simulated by the physics engine. It is affected by gravity, forces, and collisions with other bodies. Ideal for debris, boxes, and physical props.

#### Constructors

- **`RigidActor(Scalar x, Scalar y, int w, int h)`**
    Constructs a new RigidActor.

- **`RigidActor(Vector2 position, int w, int h)`**
    Constructs a new RigidActor using a position vector.

#### Properties

- **`bool bounce`**: Whether the object should use restitution for bounces.

**Example:**

```cpp
auto box = std::make_unique<RigidActor>(100, 0, 16, 16);
box->setCollisionLayer(Layers::kProps);
### CircleActor (Pattern)

While the engine defines `RigidActor` and `StaticActor`, creating a circular object is done by setting the `shape` property.

**Structure:**

```cpp
class MyCircle : public RigidActor {
public:
    MyCircle(Scalar x, Scalar y, Scalar r) : RigidActor(x, y, r*2, r*2) {
        shape = CollisionShape::CIRCLE;
        radius = r;
    }
};
```

box->bounce = true; // Make it bouncy
scene->addEntity(box.get());

```

---



---

### Collision Primitives

**Inherits:** None

Lightweight geometric primitives and helpers used by the physics and collision systems.

#### Types

- **`struct Circle`**
    Represents a circle in 2D space.

  - `Scalar x, y` – center position.
  - `Scalar radius` – circle radius.

- **`struct Segment`**
    Represents a line segment between two points.

  - `Scalar x1, y1` – start point.
  - `Scalar x2, y2` – end point.

#### Helper Functions

- **`bool intersects(const Circle& a, const Circle& b)`**
    Returns true if two circles overlap.

- **`bool intersects(const Circle& c, const Rect& r)`**
    Returns true if a circle overlaps an axis-aligned rectangle.

- **`bool intersects(const Segment& s, const Rect& r)`**
    Returns true if a line segment intersects an axis-aligned rectangle.

- **`bool sweepCircleVsRect(const Circle& start, const Circle& end, const Rect& rect, Scalar& tHit)`**
    Performs a simple sweep test between two circle positions against a rectangle.  
    Returns true if a collision occurs between `start` and `end`, writing the normalized hit time in `tHit` (`0.0f` = at `start`, `1.0f` = at `end`).

---

### DefaultLayers

**Inherits:** None

Namespace with common collision layer constants:

- `kNone`: 0 (No collision)
- `kAll`: 0xFFFF (Collides with everything)

---

### TileAttributes (physics)

**Include:** `physics/TileAttributes.h`  
**Namespace:** `pixelroot32::physics`

Helpers for encoding tile metadata in `PhysicsActor::userData`, used by tilemap collision builders and game logic. Supports both a **flags-based** API (recommended for new code) and a legacy **behavior enum** API.

#### TileFlags (recommended)

- **`enum TileFlags : uint8_t`**: Bit flags for tile behavior (1 byte per tile, no strings at runtime). Values: `TILE_NONE`, `TILE_SOLID`, `TILE_SENSOR`, `TILE_DAMAGE`, `TILE_COLLECTIBLE`, `TILE_ONEWAY`, `TILE_TRIGGER` (bits 6–7 reserved).
- **`packTileData(uint16_t x, uint16_t y, TileFlags flags)`**: Packs coords (10+10 bits) and flags (8 bits) into `uintptr_t` for `setUserData()`.
- **`unpackTileData(uintptr_t packed, uint16_t& x, uint16_t& y, TileFlags& flags)`**: Unpacks for use in `onCollision`.
- **`getTileFlags(const TileBehaviorLayer& layer, int x, int y)`**: O(1) lookup with bounds check; returns `TILE_NONE` (0) when out of bounds.
- **`isSensorTile(TileFlags flags)`** / **`isOneWayTile(TileFlags flags)`** / **`isSolidTile(TileFlags flags)`**: Derive physics config from flags for the collision builder.

#### TileBehaviorLayer

- **`struct TileBehaviorLayer`**: `const uint8_t* data`, `uint16_t width`, `uint16_t height`. Points to a dense array (1 byte per tile) exported by the Tilemap Editor. Use with `getTileFlags()` for O(1) lookups.

#### Legacy (deprecated for new code)

- **`enum class TileCollisionBehavior`**: `SOLID`, `SENSOR`, `ONE_WAY_UP`, `DAMAGE`, `DESTRUCTIBLE`.
- **`packTileData(x, y, TileCollisionBehavior)`** / **`unpackTileData(..., TileCollisionBehavior&)`**: Same encoding with 4-bit behavior.
- **`packCoord(x, y)`** / **`unpackCoord(packed, x, y)`**: Legacy 16+16 bit encoding for coords only.

---

### TileConsumptionHelper

**Include:** `physics/TileConsumptionHelper.h`  
**Namespace:** `pixelroot32::physics`

Helper for **consumible tiles** (e.g. coins, pickups): removes the tile’s physics body from the scene and updates the tilemap’s `runtimeMask` so the tile is no longer drawn. Reuses `TileMapGeneric::runtimeMask` (no separate consumed mask).

- **`struct TileConsumptionConfig`**: Optional config: `updateTilemap`, `logConsumption`, `validateCoordinates`.
- **`TileConsumptionHelper(Scene& scene, void* tilemap, const TileConsumptionConfig& config)`**: Constructor. `tilemap` is `TileMapGeneric*` (any of `Sprite`, `Sprite2bpp`, `Sprite4bpp`).
- **`bool consumeTile(Actor* tileActor, uint16_t tileX, uint16_t tileY)`**: Removes `tileActor` from the scene and sets `setTileActive(tileX, tileY, false)` on the tilemap. If `tileActor == nullptr`, only updates the tilemap mask.
- **`bool consumeTileFromUserData(Actor* tileActor, uintptr_t packedUserData)`**: Unpacks coords/flags from userData and consumes only if `TILE_COLLECTIBLE` is set.
- **`bool isTileConsumed(uint16_t tileX, uint16_t tileY) const`**: Returns whether the tile is inactive in the tilemap.
- **`bool restoreTile(uint16_t tileX, uint16_t tileY)`**: Sets the tile active again (visual only; does not re-add a physics body).

**Convenience functions:**

- **`consumeTileFromCollision(tileActor, packedUserData, scene, tilemap, config)`**: One-shot consumption from an `onCollision` callback.
- **`consumeTilesBatch(scene, tilemap, tiles[][2], count, config)`**: Updates `runtimeMask` for multiple tiles (no entity removal; use for clearing areas or reset).

---

### TileCollisionBuilder

**Include:** `physics/TileCollisionBuilder.h`  
**Namespace:** `pixelroot32::physics`

High-level builder that generates `StaticActor` or `SensorActor` bodies from a `TileBehaviorLayer`. Iterates all tiles with non-zero flags, creates the appropriate physics body, configures it (sensor, one-way), packs coords and flags into `userData`, and adds it to the scene. This is the recommended way to populate physics for tilemap-based levels.

#### TileCollisionBuilderConfig

```cpp
struct TileCollisionBuilderConfig {
    uint8_t tileWidth;      // Width of each tile in world units (e.g., 16)
    uint8_t tileHeight;     // Height of each tile in world units (e.g., 16)
    uint16_t maxEntities;   // Maximum entities to create (safety limit)

    TileCollisionBuilderConfig(uint8_t w = 16, uint8_t h = 16, uint16_t max = 0xFFFF);
};
```

#### Class Definition

```cpp
class TileCollisionBuilder {
public:
    TileCollisionBuilder(pixelroot32::core::Scene& scene, 
                         const TileCollisionBuilderConfig& config = TileCollisionBuilderConfig());

    int buildFromBehaviorLayer(const TileBehaviorLayer& layer, uint8_t layerIndex = 0);
    int getEntitiesCreated() const;
    void reset();
};
```

#### Public Methods

- **`TileCollisionBuilder(Scene& scene, const TileCollisionBuilderConfig& config)`**  
  Constructs the builder bound to a scene.

- **`int buildFromBehaviorLayer(const TileBehaviorLayer& layer, uint8_t layerIndex = 0)`**  
  Iterates all tiles in the layer. For each tile with `flags != TILE_NONE`:
  - Creates `StaticActor` (solid, one-way) or `SensorActor` (sensor, damage, collectible)
  - Configures via `setSensor()` / `setOneWay()` from flags
  - Sets `setCollisionLayer(kDefaultItemCollisionLayer)` and `setCollisionMask(kDefaultItemCollisionMask)`
  - Calls `setUserData(reinterpret_cast<void*>(packTileData(x, y, flags)))`
  - Adds to scene via `scene.addEntity()`
  
  Returns the total number of entities created.

- **`int getEntitiesCreated() const`**  
  Returns the count from the last `buildFromBehaviorLayer()` call.

- **`void reset()`**  
  Resets `entitiesCreated` to 0. Does not clear the scene.

#### Convenience Helper

```cpp
inline int buildTileCollisions(
    pixelroot32::core::Scene& scene,
    const TileBehaviorLayer& layer,
    uint8_t tileWidth = 16,
    uint8_t tileHeight = 16,
    uint8_t layerIndex = 0
);
```

One-liner that creates a builder, calls `buildFromBehaviorLayer()`, and returns the count.

#### Usage Example

```cpp
#include "physics/TileCollisionBuilder.h"

void GameScene::init() override {
    // Behavior layer exported by Tilemap Editor (dense uint8_t[] array)
    TileBehaviorLayer layer = { behaviorData, 32, 32 };

    // Basic usage (one-liner)
    int count = buildTileCollisions(*this, layer, 16, 16, 0);

    // Or with explicit config
    TileCollisionBuilderConfig config(16, 16, 2048);  // 16x16 tiles, max 2048 bodies
    TileCollisionBuilder builder(*this, config);
    int entities = builder.buildFromBehaviorLayer(layer, 0);
}
```

#### Integration with onCollision

After collision bodies are created, use `userData` in callbacks to identify the tile:

```cpp
void PlayerActor::onCollision(Actor* other) override {
    if (other->getUserData()) {
        uintptr_t packed = reinterpret_cast<uintptr_t>(other->getUserData());
        uint16_t tx, ty;
        TileFlags flags;
        unpackTileData(packed, tx, ty, flags);

        if (flags & TILE_COLLECTIBLE) {
            TileConsumptionHelper helper(*scene, tilemap);
            helper.consumeTileFromUserData(other, packed);
        }
        if (flags & TILE_DAMAGE) {
            takeDamage();
        }
    }
}
```

#### Memory Considerations

- Each created actor is a heap allocation (`new StaticActor` / `new SensorActor`).
- Call `scene.clearEntities()` before rebuilding to avoid duplicates.
- On ESP32, keep `maxEntities` reasonable; 32×32 tiles with every tile solid = 1024 bodies.

---

### CollisionSystem

**Inherits:** None

The central physics system implementing **Flat Solver**. Manages collision detection and resolution with fixed timestep for deterministic behavior. Uses a **dual-layer spatial grid** (static + dynamic) to minimize per-frame work when many static tiles are present, and a **fixed-size contact pool** (`PHYSICS_MAX_CONTACTS`, default 128; overridable via build flags) to avoid heap allocations in the hot path.

#### Key Logic: "The Flat Solver"

The solver executes in strict order:

1. **Detect Collisions**: Rebuilds static grid if dirty, clears dynamic layer, inserts RIGID/KINEMATIC into dynamic layer; queries grid for potential pairs; narrowphase and contact generation. Contacts are stored in a fixed array; excess contacts are dropped when the pool is full.
2. **Solve Velocity**: Impulse-based collision response (2 iterations by default); sensor contacts are skipped.
3. **Integrate Positions**: Updates positions: `p = p + v * dt` (RIGID only).
4. **Solve Penetration**: Baumgarte stabilization with slop threshold; sensor contacts skipped.
5. **Trigger Callbacks**: Calls `onCollision()` for all contacts.

#### Public Constants

- **`FIXED_DT`**: Fixed timestep (`1/60s`)
- **`SLOP`**: Minimum penetration to correct (`0.02f`)
- **`BIAS`**: Position correction factor (`0.2f`)
- **`VELOCITY_ITERATIONS`**: Impulse solver iterations (`2`)
- **`VELOCITY_THRESHOLD`**: Zero restitution below this speed (`0.5f`)
- **`CCD_THRESHOLD`**: CCD activation threshold (`3.0f`)

#### Public Methods

- **`void update()`**  
  Executes the full physics pipeline. Called automatically by `Scene::update()`.

- **`void detectCollisions()`**  
  Broadphase and narrowphase detection. Populates contact list.

- **`void solveVelocity()`**  
  Impulse-based velocity solver. Applies collision response.

- **`void integratePositions()`**  
  Updates positions using velocity. Only affects `RigidActor`.

- **`void solvePenetration()`**  
  Position correction using Baumgarte stabilization.

- **`void triggerCallbacks()`**  
  Invokes `onCollision()` for all contacts.

- **`bool needsCCD(PhysicsActor* body)`**  
  Returns true if body needs Continuous Collision Detection (fast-moving circles).

- **`bool sweptCircleVsAABB(PhysicsActor* circle, PhysicsActor* box, Scalar& outTime, Vector2& outNormal)`**  
  Performs swept test for CCD. Returns collision time (0.0-1.0) and normal.

- **`bool validateOneWayPlatform(PhysicsActor* actor, PhysicsActor* platform, const Vector2& collisionNormal)`**
  Validates whether a one-way platform collision should be resolved based on spatial crossing detection. Returns `true` if the collision should be resolved (actor crossed from above), `false` otherwise. This method checks:
  - If the platform is a one-way platform
  - If the collision normal points upward (actor above platform)
  - If the actor crossed the platform surface from above (using previous position)
  - If the actor is moving downward or stationary
  - Rejects horizontal collisions (side collisions with one-way platforms)

- **`size_t getEntityCount() const`**  
  Returns number of entities in the system.

- **`void clear()`**
  Removes all entities, resets the contact count, and clears the spatial grid (both static and dynamic layers).

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
    > **Note:** The scene does **not** take ownership of the entity. You must ensure the entity remains valid as long as it is in the scene (typically by holding it in a `std::unique_ptr` within your Scene class).

- **`void removeEntity(Entity* entity)`**
The engine defines default limits in `platforms/EngineConfig.h`: `MAX_LAYERS` (default 4) and `MAX_ENTITIES` (default 64). These are guarded with `#ifndef`, so you can override them from your project without modifying the engine.

- **`void clearEntities()`**
    Removes all entities from the scene. Does not delete the entity objects.

#### Overriding scene limits (MAX_LAYERS / MAX_ENTITIES)

The engine defines default limits in `platforms/EngineConfig.h`: `MAX_LAYERS` (default 3) and `MAX_ENTITIES` (default 32). These are guarded with `#ifndef`, so you can override them from your project without modifying the engine.

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

- **`std::optional<Scene*> getCurrentScene() const`**
    Gets the currently active scene, or std::nullopt if no scene is active.

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

- **`void drawText(std::string_view text, int16_t x, int16_t y, Color color, uint8_t size)`**
    Draws a string of text using the native bitmap font system. Uses the default font set in `FontManager`, or a custom font if provided via the overloaded version.
  - **text**: The string to render (ASCII characters 32-126 are supported).
  - **x, y**: Position where text starts (top-left corner).
  - **color**: Color from the `Color` enum (uses sprite palette context).
  - **size**: Scale multiplier (1 = normal, 2 = double, 3 = triple, etc.).

- **`void drawText(std::string_view text, int16_t x, int16_t y, Color color, uint8_t size, const Font* font)`**
    Draws text using a specific font. If `font` is `nullptr`, uses the default font from `FontManager`.

- **`void drawTextCentered(std::string_view text, int16_t y, Color color, uint8_t size)`**
    Draws text centered horizontally at a given Y coordinate using the default font.

- **`void drawTextCentered(std::string_view text, int16_t y, Color color, uint8_t size, const Font* font)`**
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

- **`void setOffset(int x, int y)`**
    Sets the hardware alignment offset for the display.

- **`void setRotation(uint8_t rotation)`**
    Sets the hardware rotation of the display.

- **`void drawSprite(const Sprite& sprite, int x, int y, Color color, bool flipX = false)`**
    Draws a 1bpp monochrome sprite described by a `Sprite` struct using a palette `Color`. Bit 0 of each row is the leftmost pixel, bit (`width - 1`) is the rightmost pixel.

- **`void drawSprite(const Sprite2bpp& sprite, int x, int y, uint8_t paletteSlot = 0, bool flipX = false)`**
    Available when `PIXELROOT32_ENABLE_2BPP_SPRITES` is defined. Draws a packed 2bpp sprite using the specified sprite palette slot. Index `0` is treated as transparent.
    - **paletteSlot**: Sprite palette slot (0-7). If context is active, this parameter is overridden by the context slot.
    *Optimized:* Uses `uint16_t` native access and supports MSB-first bit ordering for high performance.

- **`void drawSprite(const Sprite4bpp& sprite, int x, int y, uint8_t paletteSlot = 0, bool flipX = false)`**
    Available when `PIXELROOT32_ENABLE_4BPP_SPRITES` is defined. Draws a packed 4bpp sprite using the specified sprite palette slot. Index `0` is treated as transparent.
    - **paletteSlot**: Sprite palette slot (0-7). If context is active, this parameter is overridden by the context slot.
    *Optimized:* Uses `uint16_t` native access and supports MSB-first bit ordering for high performance.

- **`void drawSprite(const Sprite2bpp& sprite, int x, int y, bool flipX = false)`**
    Legacy overload for backward compatibility. Equivalent to `drawSprite(sprite, x, y, 0, flipX)`.

- **`void drawSprite(const Sprite4bpp& sprite, int x, int y, bool flipX = false)`**
    Legacy overload for backward compatibility. Equivalent to `drawSprite(sprite, x, y, 0, flipX)`.

- **`void setSpritePaletteSlotContext(uint8_t slot)`**
    Sets the sprite palette slot context for multi-palette sprites. When active, all subsequent `drawSprite` calls for 2bpp/4bpp sprites will use this slot regardless of the `paletteSlot` parameter. This is useful for batch rendering with the same palette.
    - **slot**: Palette slot (0-7). To disable context, call with 0xFF or use default.

- **`uint8_t getSpritePaletteSlotContext() const`**
    Gets the current sprite palette slot context.
    - **Returns**: Current palette slot, or 0xFF if context is inactive.

- **`void drawMultiSprite(const MultiSprite& sprite, int x, int y)`**
    Draws a layered sprite composed of multiple 1bpp `SpriteLayer` entries. Each layer is rendered in order using `drawSprite`, enabling multi-color NES/GameBoy-style sprites.

- **`void drawTileMap(const TileMap& map, int originX, int originY, Color color)`**
    Draws a tile-based background using a compact `TileMap` descriptor built on 1bpp `Sprite` tiles. Includes automatic Viewport Culling.

- **`void drawTileMap(const TileMap2bpp& map, int originX, int originY)`**
    Available when `PIXELROOT32_ENABLE_2BPP_SPRITES` is defined. Draws a 2bpp tilemap. Optimized with Viewport Culling and Palette LUT Caching. If `map.paletteIndices` is non-null, each cell can use a different background palette slot (0–7); otherwise all cells use the default background palette (slot 0).

- **`void drawTileMap(const TileMap4bpp& map, int originX, int originY)`**
    Available when `PIXELROOT32_ENABLE_4BPP_SPRITES` is defined. Draws a 4bpp tilemap. Optimized with Viewport Culling and Palette LUT Caching. If `map.paletteIndices` is non-null, each cell can use a different background palette slot (0–7); otherwise all cells use the default background palette (slot 0).

---

### Platform Optimizations (ESP32)

The engine includes several low-level optimizations for the ESP32 platform to maximize performance:

- **DMA Support**: Buffer transfers to the display are handled via DMA (`pushImageDMA`), allowing the CPU to process the next frame while the current one is being sent to the hardware.
- **IRAM Execution**: Critical rendering functions (`drawPixel`, `drawSpriteInternal`, `resolveColor`, `drawTileMap`) are decorated with `IRAM_ATTR` to run from internal RAM, bypassing the slow SPI Flash latency.
- **Palette Caching**: Tilemaps cache the resolved RGB565 LUT per tile. The cache key is the pair (tile palette pointer, background palette pointer). When `paletteIndices` is used, the LUT is rebuilt only when either the tile’s palette or the cell’s background palette slot changes (`lastTilePalettePtr` / `lastBackgroundPalettePtr`), minimizing redundant work.
- **Viewport Culling**: All tilemap rendering functions automatically skip tiles that are outside the current screen boundaries.

- **`void setDisplaySize(int w, int h)`**
    Sets the logical display size.

- **`void setDisplayOffset(int x, int y)`**
    Sets a global offset for all drawing operations.

- **`void setContrast(uint8_t level)`**
    Sets the display contrast/brightness (0-255).

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

- **`static int16_t textWidth(const Font* font, std::string_view text, uint8_t size = 1)`**
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

The `Camera2D` class provides a 2D camera system for managing the viewport and scrolling of the game world. It handles coordinate transformations and target following with configurable dead zones.

#### Public Methods

- **`Camera2D(int viewportWidth, int viewportHeight)`**
    Constructs a new `Camera2D` with the specified viewport dimensions.

- **`void setBounds(float minX, float maxX)`**
    Sets the horizontal boundaries for the camera. The camera's x position will be clamped within this range.

- **`void setVerticalBounds(float minY, float maxY)`**
    Sets the vertical boundaries for the camera. The camera's y position will be clamped within this range.

- **`void setPosition(float x, float y)`**
    Sets the camera's position directly. The position will be clamped to the current bounds.

- **`void followTarget(float targetX)`**
    Updates the camera position to follow a target's x coordinate. The camera uses a "dead zone" in the center of the screen where the camera won't move.

- **`void followTarget(float targetX, float targetY)`**
    Updates the camera position to follow a target's x and y coordinates. The camera uses a "dead zone" in the center of the screen where the camera won't move.

- **`float getX() const`**
    Returns the current x position of the camera.

- **`float getY() const`**
    Returns the current y position of the camera.

- **`void apply(Renderer& renderer) const`**
    Applies the camera's transformation to the renderer. This should be called before drawing world objects.

- **`void setViewportSize(int width, int height)`**
    Updates the viewport size (usually logical resolution).

---

### Color

**Inherits:** None

The `Color` module manages the engine's color palettes and provides the `Color` enumeration for referencing colors within the active palette. It also maintains a **background palette slot bank** (number of slots from `MAX_BACKGROUND_PALETTE_SLOTS` in `EngineConfig.h`, default 8) for multi-palette 2bpp/4bpp tilemaps, where each cell can select a slot via the tilemap's optional `paletteIndices` array.

#### PaletteType (Enum)

Enumeration of available color palettes.

- `PR32` (Default): The standard PixelRoot32 palette (vibrant, general purpose).
- `NES`: Nintendo Entertainment System inspired palette.
- `GB`: Game Boy inspired palette (4 greens).
- `GBC`: Game Boy Color inspired palette.
- `PICO8`: PICO-8 fantasy console palette.

#### Public Methods

- **`static void setPalette(PaletteType type)`**
    Sets the active color palette for the engine (Single Palette Mode).
    *Note: This sets both background and sprite palettes to the same value. Does not enable dual palette mode. This should typically be called once during game initialization (e.g., in the first Scene's `init()` method).*

- **`static void setCustomPalette(const uint16_t* palette)`**
    Sets a custom color palette defined by the user (Single Palette Mode).
  - **palette**: Pointer to an array of 16 `uint16_t` values (RGB565).
  - **Warning**: The array must remain valid for the duration of its use (e.g., use `static const` or global arrays). The engine does not copy the data.
  - *Note: Sets both background and sprite palettes to the same value. Does not enable dual palette mode.*

- **`static void enableDualPaletteMode(bool enable)`**
    Enables or disables dual palette mode.
  - **enable**: `true` to enable dual palette mode (separate palettes for backgrounds and sprites), `false` for Single Palette Mode.

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

#### Background palette slot bank (multi-palette tilemaps)

For 2bpp/4bpp tilemaps, the engine supports **multiple background palettes per cell**. A bank of slots (default 8, configurable via **`MAX_BACKGROUND_PALETTE_SLOTS`** in `EngineConfig.h` or build flag `-DMAX_BACKGROUND_PALETTE_SLOTS=N`) holds palette pointers; slot 0 is the default and is kept in sync with `setBackgroundPalette` / `setBackgroundCustomPalette`. If a tilemap provides an optional `paletteIndices` array, each cell can select a slot (0 to N−1) so different areas use different palettes (e.g. ground, water, lava) without changing tile data.

- **`static void initBackgroundPaletteSlots()`**
    Initializes all background palette slots to the default palette. Call at engine startup if using multi-palette tilemaps; otherwise slots are initialized lazily when setting the background palette.

- **`static void setBackgroundPaletteSlot(uint8_t slotIndex, PaletteType palette)`**
    Sets a background palette slot by type (for multi-palette tilemaps).
  - **slotIndex**: Slot 0–7. Slot 0 is the default; setting it also updates the global background palette.
  - **palette**: The palette type for this slot.

- **`static void setBackgroundCustomPaletteSlot(uint8_t slotIndex, const uint16_t* palette)`**
    Sets a background palette slot with a custom RGB565 palette.
  - **slotIndex**: Slot 0–7. Slot 0 is the default; setting it also updates the global background palette.
  - **palette**: Pointer to 16 `uint16_t` RGB565 values; must remain valid.

- **`static const uint16_t* getBackgroundPaletteSlot(uint8_t slotIndex)`**
    Returns the palette pointer for a background slot (for renderer use). If slot is not set, returns slot 0; if slot 0 is not set, returns global background palette. Never returns `nullptr`.
  - **slotIndex**: Slot 0–7.

#### Sprite palette slot bank (multi-palette sprites)

For 2bpp/4bpp sprites, the engine supports **multiple palettes** via a sprite palette slot bank (default 8 slots, configurable via **`MAX_SPRITE_PALETTE_SLOTS`** in `EngineConfig.h` or build flag `-DMAX_SPRITE_PALETTE_SLOTS=N`). Slot 0 is the default and is kept in sync with `setSpritePalette` / `setSpriteCustomPalette`. The Renderer uses the slot index passed in `drawSprite(sprite, x, y, paletteSlot, flipX)`, or the current context slot set via `setSpritePaletteSlotContext()`.

- **`static void initSpritePaletteSlots()``**
    Initializes all sprite palette slots to the default palette. Call at engine startup if using multi-palette sprites; otherwise slots are initialized lazily.

- **`static void setSpritePaletteSlot(uint8_t slotIndex, PaletteType palette)`**
    Sets a sprite palette slot by type (for multi-palette sprites).
  - **slotIndex**: Slot 0–7. Slot 0 is the default; setting it also updates the global sprite palette.
  - **palette**: The palette type for this slot.

- **`static void setSpriteCustomPaletteSlot(uint8_t slotIndex, const uint16_t* palette)`**
    Sets a sprite palette slot with a custom RGB565 palette.
  - **slotIndex**: Slot 0–7. Slot 0 is the default; setting it also updates the global sprite palette.
  - **palette**: Pointer to 16 `uint16_t` RGB565 values; must remain valid.

- **`static const uint16_t* getSpritePaletteSlot(uint8_t slotIndex)`**
    Returns the palette pointer for a sprite slot. If slot is not set, returns slot 0; if slot 0 is not set, returns the built-in PR32 palette. Never returns `nullptr`.
  - **slotIndex**: Slot 0–7.

- **`static uint16_t resolveColor(Color color)`**
    Converts a `Color` enum value to its corresponding RGB565 `uint16_t` representation based on the currently active palette (Single Palette Mode).

- **`static uint16_t resolveColor(Color color, PaletteContext context)`**
    Converts a `Color` enum value to its corresponding RGB565 `uint16_t` representation based on the context (dual palette mode) or current active palette (Single Palette Mode).
  - **context**: `PaletteContext::Background` for backgrounds/tilemaps, `PaletteContext::Sprite` for sprites.

- **`static uint16_t resolveColorWithPalette(Color color, const uint16_t* palette)`**
    Converts a `Color` enum value to RGB565 using an explicit palette (used internally for per-cell tilemap palette resolution).
  - **color**: The `Color` enum value.
  - **palette**: Pointer to 16 `uint16_t` RGB565 palette; if `nullptr`, returns 0.

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

Generic descriptor for tile-based backgrounds. It stores level data as an array of indices mapping to a tileset. Supports optional tile animations via `TileAnimationManager`.

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

- **`uint8_t* runtimeMask`**  
  Optional bitmask for runtime tile activation (1 bit per cell). If non-null, tiles whose bit is 0 are skipped by `drawTileMap`. Use `initRuntimeMask()`, `isTileActive()`, `setTileActive()`; size is `(width * height + 7) / 8` bytes.

- **`const uint8_t* paletteIndices`**  
  Optional per-cell background palette index (for 2bpp/4bpp multi-palette tilemaps only). If `nullptr`, all cells use the default background palette (slot 0). If non-null, array size must be `width * height`; each byte uses bits 0–2 for the palette slot (0–7), bits 3–7 reserved for future use. Use with `setBackgroundPaletteSlot` / `setBackgroundCustomPaletteSlot` to assign palettes to slots. Typically filled by the editor or export tools; can be stored in PROGMEM.

- **`TileAnimationManager* animManager`**  
  Optional pointer to a `TileAnimationManager` for tile animations. If non-null, the renderer will resolve animated tile frames automatically. Set to `nullptr` (default) to disable animations with zero overhead. See [Tile Animation System](#tile-animation-system) for details.

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

## Tile Animation System

The Tile Animation System enables frame-based tile animations (water, lava, fire, conveyor belts, etc.) while maintaining static tilemap data and ESP32-optimized performance. Animations are defined at compile-time and resolved at render-time with O(1) lookup.

### Design Philosophy

- **Static Tilemap Data**: Tilemap indices never change; animation is a view-time transformation
- **Zero Dynamic Allocations**: All data in fixed-size arrays or PROGMEM
- **O(1) Frame Resolution**: Hash table lookup for instant frame resolution
- **Retro Console Pattern**: Inspired by NES/SNES tile animation systems
- **Minimal CPU Overhead**: <1% of frame budget on ESP32

### TileAnimation (Struct)

**Namespace:** `pixelroot32::graphics`

Defines a single tile animation sequence. All data stored in PROGMEM/flash to minimize RAM usage.

#### Properties

- **`uint8_t baseTileIndex`**  
  First tile in the animation sequence (e.g., 42 for water animation starting at tile 42).

- **`uint8_t frameCount`**  
  Number of frames in the animation (e.g., 4 for a 4-frame water cycle).

- **`uint8_t frameDuration`**  
  Number of game frames to display each animation frame (e.g., 8 = each frame displays for 8 game ticks).

- **`uint8_t reserved`**  
  Padding for alignment (reserved for future use).

#### Example

```cpp
// Water animation: tiles 42-45, 4 frames, 8 ticks per frame
const TileAnimation waterAnim = { 42, 4, 8, 0 };

// Lava animation: tiles 46-47, 2 frames, 6 ticks per frame
const TileAnimation lavaAnim = { 46, 2, 6, 0 };
```

**Memory:** 4 bytes per animation (stored in PROGMEM).

---

### TileAnimationManager (Class)

**Namespace:** `pixelroot32::graphics`

Manages tile animations for a tilemap. Provides O(1) frame resolution via lookup table. All animation definitions stored in PROGMEM. Zero dynamic allocations.

#### Constructor

```cpp
TileAnimationManager(
    const TileAnimation* animations,
    uint8_t animCount,
    uint16_t tileCount
);
```

**Parameters:**

- **`animations`**: PROGMEM array of `TileAnimation` definitions
- **`animCount`**: Number of animations in the array
- **`tileCount`**: Number of tiles in tileset (from `TileMapGeneric::tileCount`)

**Note:** Uses fixed-size lookup table (`MAX_TILESET_SIZE`) to comply with PixelRoot32's zero-allocation policy.

#### Public Methods

- **`void step()`**  
  Advances all animations by one step. Call once per frame in `Scene::update()`.
  
  **Complexity:** O(animations × frameCount) - typically 4-32 operations (~1-7 µs on ESP32).

- **`void reset()`**  
  Resets all animations to frame 0. Useful for restarting animations or synchronizing with game events.

- **`uint8_t resolveFrame(uint8_t tileIndex) const`**  
  Resolves tile index to current animated frame.
  
  **Parameters:**
  - `tileIndex`: Base tile index from tilemap
  
  **Returns:** Current frame index (may be same as input if tile is not animated).
  
  **Performance:** O(1) array lookup, IRAM-optimized, no branches in hot path (~0.1 µs per call).

#### Memory Usage

| Component | Size | Location |
|-----------|------|----------|
| Lookup table | `MAX_TILESET_SIZE` bytes | RAM |
| Manager state | 9 bytes | RAM |
| Animation definitions | 4 bytes × N | PROGMEM (flash) |
| **Total (256 tiles)** | **265 bytes RAM** | ~0.08% of ESP32 DRAM |

**Configuration:** Adjust `MAX_TILESET_SIZE` in `EngineConfig.h` or build flags:

```ini
# For smaller tilesets (saves RAM)
build_flags = -D MAX_TILESET_SIZE=64   # 73 bytes RAM
build_flags = -D MAX_TILESET_SIZE=128  # 137 bytes RAM
build_flags = -D MAX_TILESET_SIZE=256  # 265 bytes RAM (default)
```

---

### Integration with TileMapGeneric

To enable animations for a tilemap, add the optional `animManager` pointer to `TileMapGeneric`:

```cpp
template<typename T>
struct TileMapGeneric {
    // ... existing fields ...
    TileAnimationManager* animManager = nullptr;  // Optional animation support
};
```

**Backward Compatibility:** Setting `animManager = nullptr` (default) disables animations with zero overhead.

---

### Usage Example

#### Step 1: Define Animations (Scene Header)

```cpp
// scenes/water_level.h
#pragma once
#include "graphics/Renderer.h"
#include "graphics/TileAnimation.h"

namespace scenes::water_level {

using namespace pixelroot32::graphics;

// Tileset with animated frames
PIXELROOT32_SCENE_FLASH_ATTR const Sprite2bpp tiles[] = {
    { nullptr, nullptr, 0, 0, 0 },           // Tile 0: empty
    { grassData, grassPalette, 8, 8, 4 },    // Tile 1: grass (static)
    { waterFrame0, waterPalette, 8, 8, 4 },  // Tile 2: water frame 0
    { waterFrame1, waterPalette, 8, 8, 4 },  // Tile 3: water frame 1
    { waterFrame2, waterPalette, 8, 8, 4 },  // Tile 4: water frame 2
    { waterFrame3, waterPalette, 8, 8, 4 },  // Tile 5: water frame 3
    { lavaFrame0, lavaPalette, 8, 8, 4 },    // Tile 6: lava frame 0
    { lavaFrame1, lavaPalette, 8, 8, 4 },    // Tile 7: lava frame 1
    // ... more tiles ...
};

// Animation definitions (PROGMEM)
PIXELROOT32_SCENE_FLASH_ATTR const TileAnimation animations[] = {
    { 2, 4, 8, 0 },  // Water: base=2, 4 frames, 8 ticks/frame
    { 6, 2, 6, 0 },  // Lava: base=6, 2 frames, 6 ticks/frame
};

constexpr uint8_t ANIM_COUNT = 2;
constexpr uint16_t MAX_TILE_INDEX = 64;

// Tilemap data (indices reference base tiles)
uint8_t backgroundIndices[] = {
    1, 1, 1, 1, 1, 1, 1, 1,
    1, 2, 2, 2, 2, 2, 2, 1,  // Water tiles use base index (2)
    1, 6, 6, 6, 6, 6, 6, 1,  // Lava tiles use base index (6)
    1, 1, 1, 1, 1, 1, 1, 1,
};

TileMap2bpp backgroundLayer = {
    backgroundIndices,
    8, 4,     // width, height
    tiles,
    8, 8,     // tileWidth, tileHeight
    64,       // tileCount
    nullptr,  // runtimeMask
    nullptr   // paletteIndices
};

// Animation manager instance
TileAnimationManager animManager(animations, ANIM_COUNT, MAX_TILE_INDEX);

} // namespace scenes::water_level
```

#### Step 2: Scene Implementation

```cpp
// WaterLevelScene.cpp
#include "WaterLevelScene.h"
#include "scenes/water_level.h"

using namespace scenes::water_level;

void WaterLevelScene::init() {
    // Link animation manager to tilemap
    backgroundLayer.animManager = &animManager;
}

void WaterLevelScene::update(unsigned long deltaTime) {
    // Advance animations once per frame
    animManager.step();
    
    // Update other game logic...
    Scene::update(deltaTime);
}

void WaterLevelScene::draw(Renderer& renderer) {
    renderer.beginFrame();
    
    // Render animated tilemap (automatic frame resolution)
    renderer.drawTileMap(backgroundLayer, 0, 0);
    
    // Draw other entities...
    Scene::draw(renderer);
    
    renderer.endFrame();
}
```

---

### Performance Characteristics

#### CPU Cost

**Per-frame overhead:**

| Operation | Complexity | Typical Cost (ESP32 @ 240MHz) |
|-----------|------------|-------------------------------|
| `step()` | O(animations × frameCount) | 1-7 µs (4-8 animations) |
| `resolveFrame()` per tile | O(1) | ~0.1 µs |
| **Total (20×15 tilemap, 50% visible)** | - | **~7 µs (0.04% of 16ms frame)** |

**Scalability:**

| Tilemap Size | Visible Tiles | Animation Cost | % of 16ms Frame |
|--------------|---------------|----------------|-----------------|
| 20×15 (300) | 150 | 7 µs | 0.04% |
| 40×30 (1200) | 300 | 14 µs | 0.09% |
| 64×64 (4096) | 400 | 18 µs | 0.11% |

**Conclusion:** Animation overhead is negligible (<0.2% of frame budget) even on large tilemaps.

#### Memory Cost

| Tileset Size | Lookup Table | Total RAM | % of ESP32 DRAM |
|--------------|--------------|-----------|-----------------|
| 64 tiles | 64 bytes | 73 bytes | 0.02% |
| 128 tiles | 128 bytes | 137 bytes | 0.04% |
| 256 tiles | 256 bytes | 265 bytes | 0.08% |

**PROGMEM (Flash):** 4 bytes × number of animations (e.g., 8 animations = 32 bytes).

---

### Advanced Features

#### Controlling Animation Speed

Control global animation speed by calling `step()` conditionally:

```cpp
void MyScene::update(unsigned long deltaTime) {
    // Half speed: advance every 2 frames
    if (frameCounter % 2 == 0) {
        animManager.step();
    }
    
    // Double speed: advance twice per frame
    animManager.step();
    animManager.step();
}
```

#### Pausing Animations

Simply don't call `step()` to freeze animations:

```cpp
void MyScene::update(unsigned long deltaTime) {
    if (!isPaused) {
        animManager.step();
    }
}
```

#### Synchronizing Animations

Use `reset()` to restart animations at specific game events:

```cpp
void MyScene::onLevelStart() {
    animManager.reset();  // All animations start from frame 0
}
```

#### Multiple Animation Managers

Each tilemap layer can have its own animation manager:

```cpp
TileAnimationManager backgroundAnimManager(bgAnims, 2, 64);
TileAnimationManager foregroundAnimManager(fgAnims, 4, 64);

backgroundLayer.animManager = &backgroundAnimManager;
foregroundLayer.animManager = &foregroundAnimManager;

// Update both in Scene::update()
backgroundAnimManager.step();
foregroundAnimManager.step();
```

---

### Limitations and Considerations

1. **Shared Animation State**: All instances of a tile share the same animation state. For independent animations, use different tile indices.

2. **Sequential Frames**: Frames must be sequential in the tileset (e.g., tiles 42, 43, 44, 45). Non-sequential frame sequences are not supported in v1.0.

3. **Global Timing**: All animations advance together by default. Per-animation timing requires multiple managers or conditional `step()` calls.

4. **Static Tilemap Data**: Tilemap indices never change. Animation is purely a rendering transformation.

5. **Maximum Animations**: Limited by `uint8_t` (255), but practical limit is ~16-32 for memory reasons.

---

### Compatibility

| Feature | 1bpp Tilemap | 2bpp Tilemap | 4bpp Tilemap |
|---------|--------------|--------------|--------------|
| Basic Animation | ✅ | ✅ | ✅ |
| Per-Cell Palette | ❌ | ✅ | ✅ |
| Runtime Mask | ✅ | ✅ | ✅ |
| Viewport Culling | ✅ | ✅ | ✅ |

**Note:** Tile animations work with all tilemap types and are compatible with existing features (runtime mask, per-cell palettes, viewport culling).

---

### Tile Attribute System

The tile attribute system provides runtime access to custom metadata attached to tiles in tilemaps. Attributes are defined in the PixelRoot32 Tilemap Editor and exported as PROGMEM structures optimized for ESP32.

#### Design Overview

**Key Concepts:**

- **Editor Workflow**: Attributes are defined at two levels in the editor:
  - **Tileset Defaults**: Common attributes shared by all instances of a tile
  - **Instance Overrides**: Per-position attributes that override defaults
- **Export Process**: The editor merges defaults and overrides, exporting only final resolved values
- **Runtime Access**: Game code queries attributes by layer index and tile position
- **Memory Efficiency**: Only tiles with attributes are exported (sparse representation)

**Memory Layout:**

All attribute data is stored in flash memory (PROGMEM) on ESP32 to minimize RAM usage:

```
Flash Memory (PROGMEM)
├── String literals (keys and values)
├── TileAttribute arrays (key-value pairs per tile)
├── TileAttributeEntry arrays (position + attributes per layer)
└── LayerAttributes arrays (layer metadata)
```

---

### TileAttribute

**Inherits:** None

Represents a single key-value metadata pair for a tile. Both strings are stored in flash memory (PROGMEM).

#### Properties

- **`const char* key`**  
  Attribute key (PROGMEM string). Common examples: `"type"`, `"solid"`, `"interactable"`, `"damage"`.

- **`const char* value`**  
  Attribute value (PROGMEM string). All values are strings; convert to int/bool as needed in game code.

#### Usage Notes

- Use `strcmp_P()` or similar PROGMEM-aware functions to compare keys
- Values are always strings; parse to appropriate types in game logic
- Both pointers reference flash memory, not RAM

#### Example

```cpp
// Querying an attribute
const char* solidValue = get_tile_attribute(0, 10, 5, "solid");
if (solidValue && strcmp_P(solidValue, "true") == 0) {
    // Tile is solid
}

// Converting string values
const char* damageValue = get_tile_attribute(0, x, y, "damage");
if (damageValue) {
    int damage = atoi(damageValue);
    player.takeDamage(damage);
}
```

---

### TileAttributeEntry

**Inherits:** None

Associates a tile position (x, y) with its metadata attributes. Only tiles with non-empty attributes are included in the exported data.

#### Properties

- **`uint16_t x`**  
  Tile X coordinate in layer space (not pixel coordinates).

- **`uint16_t y`**  
  Tile Y coordinate in layer space (not pixel coordinates).

- **`uint8_t num_attributes`**  
  Number of attributes for this tile (maximum 255).

- **`const TileAttribute* attributes`**  
  PROGMEM array of attribute key-value pairs.

#### Query Pattern

```cpp
// Manual query (low-level)
for (uint16_t i = 0; i < layer.num_tiles_with_attributes; i++) {
    const TileAttributeEntry& tile = layer.tiles[i];
    if (tile.x == targetX && tile.y == targetY) {
        // Found tile, search attributes
        for (uint8_t j = 0; j < tile.num_attributes; j++) {
            if (strcmp_P(tile.attributes[j].key, "solid") == 0) {
                // Found "solid" attribute
                const char* value = tile.attributes[j].value;
                break;
            }
        }
        break;
    }
}
```

#### Performance Notes

- Tiles array is typically small (only tiles with attributes)
- Linear search is acceptable for most use cases
- Consider caching frequently accessed attributes in game logic

---

### LayerAttributes

**Inherits:** None

Organizes all tile metadata for a single tilemap layer. Provides efficient lookup of attributes by tile position.

#### Properties

- **`const char* layer_name`**  
  Layer name (PROGMEM string). Matches the name defined in the Tilemap Editor (e.g., `"Background"`, `"Collision"`).

- **`uint16_t num_tiles_with_attributes`**  
  Number of tiles with attributes in this layer.

- **`const TileAttributeEntry* tiles`**  
  PROGMEM array of tiles with attributes (sparse representation).

#### Usage Example

```cpp
// Typical usage in a scene
#include "game_assets/level1.h" // Generated scene header

void GameScene::init() {
    // Query attribute for tile at (10, 5) in layer 0
    const char* tileType = get_tile_attribute(0, 10, 5, "type");
    
    if (tileType && strcmp_P(tileType, "door") == 0) {
        // Tile is a door, check if it's locked
        const char* locked = get_tile_attribute(0, 10, 5, "locked");
        if (locked && strcmp_P(locked, "true") == 0) {
            // Door is locked
        }
    }
}

void GameScene::checkCollision(int tileX, int tileY) {
    // Check if tile is solid
    const char* solid = get_tile_attribute(0, tileX, tileY, "solid");
    if (solid && strcmp_P(solid, "true") == 0) {
        // Handle collision with solid tile
        return true;
    }
    return false;
}
```

#### Helper Functions

Generated scene headers typically include helper functions for easier attribute access:

```cpp
// Generated in scene header (e.g., level1.h)
namespace game_assets {

// Query attribute by layer index, position, and key
const char* get_tile_attribute(
    uint8_t layer_idx,
    uint16_t x,
    uint16_t y,
    const char* key
);

// Check if tile has any attributes
bool tile_has_attributes(
    uint8_t layer_idx,
    uint16_t x,
    uint16_t y
);

}
```

#### Memory Efficiency

**Sparse Representation:**

- Only tiles with attributes are stored
- Empty tiles: 0 bytes overhead
- Typical tile with 3 attributes: ~40 bytes (depends on key/value lengths)

**Example Memory Usage:**

```
Tilemap: 32x32 tiles (1024 total)
Tiles with attributes: 50 (4.9%)
Average attributes per tile: 2
Average key length: 8 bytes
Average value length: 6 bytes

Memory calculation:
- TileAttributeEntry: 6 bytes × 50 = 300 bytes
- TileAttribute: 8 bytes × 100 = 800 bytes
- String data: (8 + 6) × 100 = 1400 bytes
Total: ~2.5 KB in flash memory
```

#### Performance Considerations

**Query Performance:**

- O(n) where n = number of tiles with attributes in layer
- Typically very fast (n is usually small, < 100)
- Consider caching frequently accessed attributes

**Optimization Strategies:**

```cpp
// Cache attributes during scene initialization
class GameScene : public Scene {
    struct TileCache {
        bool isSolid;
        bool isInteractable;
        int damage;
    };
    
    std::unordered_map<uint32_t, TileCache> tileCache;
    
    void init() override {
        // Pre-cache attributes for all tiles
        for (int y = 0; y < mapHeight; y++) {
            for (int x = 0; x < mapWidth; x++) {
                if (tile_has_attributes(0, x, y)) {
                    TileCache cache;
                    
                    const char* solid = get_tile_attribute(0, x, y, "solid");
                    cache.isSolid = solid && strcmp_P(solid, "true") == 0;
                    
                    const char* interact = get_tile_attribute(0, x, y, "interactable");
                    cache.isInteractable = interact && strcmp_P(interact, "true") == 0;
                    
                    const char* dmg = get_tile_attribute(0, x, y, "damage");
                    cache.damage = dmg ? atoi(dmg) : 0;
                    
                    uint32_t key = (y << 16) | x;
                    tileCache[key] = cache;
                }
            }
        }
    }
    
    bool isTileSolid(int x, int y) {
        uint32_t key = (y << 16) | x;
        auto it = tileCache.find(key);
        return it != tileCache.end() && it->second.isSolid;
    }
};
```

#### Common Attribute Patterns

**Collision Detection:**

```cpp
const char* solid = get_tile_attribute(layer, x, y, "solid");
if (solid && strcmp_P(solid, "true") == 0) {
    // Tile blocks movement
}
```

**Interaction System:**

```cpp
const char* type = get_tile_attribute(layer, x, y, "type");
if (type && strcmp_P(type, "door") == 0) {
    const char* locked = get_tile_attribute(layer, x, y, "locked");
    if (!locked || strcmp_P(locked, "false") == 0) {
        // Door is unlocked, can open
    }
}
```

**Damage Zones:**

```cpp
const char* damage = get_tile_attribute(layer, x, y, "damage");
if (damage) {
    int damageAmount = atoi(damage);
    player.takeDamage(damageAmount);
}
```

**Tile Behavior:**

```cpp
const char* animated = get_tile_attribute(layer, x, y, "animated");
if (animated && strcmp_P(animated, "true") == 0) {
    const char* speed = get_tile_attribute(layer, x, y, "speed");
    int animSpeed = speed ? atoi(speed) : 1;
    // Update tile animation
}
```

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

Configuration settings for initializing displays. Supports both physical (hardware) and logical (rendering) resolutions.

#### Properties

- **`type`**: The display type (`ST7789`, `ST7735`, `NONE`, `CUSTOM`).
- **`rotation`**: Display rotation (0-3 or degrees).
- **`physicalWidth, physicalHeight`**: Actual hardware resolution.
- **`logicalWidth, logicalHeight`**: Rendering resolution (game logic).
- **`xOffset, yOffset`**: Alignment offsets.

#### Custom Displays

To use a custom display driver, use the `PIXELROOT32_CUSTOM_DISPLAY` macro:

```cpp
auto config = PIXELROOT32_CUSTOM_DISPLAY(new MyDriver(), 240, 240);
```

For more details, see the [Extensibility Guide](EXTENDING_PIXELROOT32.md).

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

### ParticleEmitter

**Inherits:** [Entity](#entity)

Manages a pool of particles to create visual effects (fire, smoke, explosions).

#### Public Methods

- **`ParticleEmitter(Vector2 position, const ParticleConfig& cfg)`**
    Constructs a new emitter with specific configuration.

- **`void burst(Vector2 position, int count)`**
    Emits a burst of particles from a specific location.

---

### ParticleConfig

**Inherits:** None

Configuration parameters for a particle emitter.

#### Properties

- **`Color startColor`**: Color at the beginning of the particle's life.
- **`Color endColor`**: Color at the end of the particle's life.
- **`Scalar minSpeed`**: Minimum initial speed.
- **`Scalar maxSpeed`**: Maximum initial speed.
- **`Scalar gravity`**: Y-axis force applied to particles.
- **`Scalar friction`**: Velocity damping factor (0.0 - 1.0).
- **`uint8_t minLife`**: Minimum lifetime in frames/ticks.
- **`uint8_t maxLife`**: Maximum lifetime in frames/ticks.
- **`bool fadeColor`**: If true, interpolates color from startColor to endColor.
- **`Scalar minAngleDeg`**: Minimum emission angle in degrees.
- **`Scalar maxAngleDeg`**: Maximum emission angle in degrees.

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

Configuration structure for `InputManager`. Defines the mapping between logical inputs and physical pins (ESP32) or keyboard keys (Native/SDL2).

- **`std::vector<int> inputPins`**: (ESP32) List of GPIO pins.
- **`std::vector<uint8_t> buttonNames`**: (Native) List of scancodes/keys.
- **`int count`**: Total number of configured inputs.

**Constructor:**

- **`InputConfig(int count, ...)`**
    Variadic constructor to easily list pins/keys.

Example:

```cpp
// 3 inputs: Left, Right, Jump
InputConfig input(3, 12, 14, 27); 
```

---

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

- **`UIButton(std::string_view t, uint8_t index, float x, float y, float w, float h, std::function<void()> callback, TextAlignment textAlign = TextAlignment::CENTER, int fontSize = 2)`**
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

- **`UILabel(std::string_view t, float x, float y, Color col, uint8_t sz)`**
    Constructs a new UILabel.
- **`void setText(std::string_view t)`**
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
