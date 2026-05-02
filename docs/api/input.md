# API Reference: Input Module

This document covers the input system, including physical buttons/keyboard and touchscreen input in PixelRoot32.

> **Note:** This is part of the [API Reference](../API_REFERENCE.md). See the main index for complete documentation.

---

## Input Module Overview

Handles **physical buttons** / **keyboard** via `InputManager`, and **touch screens** via `TouchManager` and optional `ActorTouchController`.

---

## InputManager

**Include:** `input/InputManager.h`

**Inherits:** None

Handles input polling, debouncing, and state tracking for physical buttons (ESP32) or keyboard (Native/SDL2).

### Public Methods

- **`InputManager(const InputConfig& config)`**
    Constructs the InputManager with a specific configuration.

- **`void init()`**
    Initializes the input pins.

- **`void update(unsigned long dt)`** (ESP32)
    Updates input state by polling hardware pins.

- **`void update(unsigned long dt, const uint8_t* keyboardState)`** (Native/SDL2)
    Updates input state based on SDL keyboard state.

- **`bool isButtonPressed(uint8_t buttonIndex) const`**
    Returns true if button was just pressed this frame (UP → DOWN).

- **`bool isButtonReleased(uint8_t buttonIndex) const`**
    Returns true if button was just released this frame (DOWN → UP).

- **`bool isButtonDown(uint8_t buttonIndex) const`**
    Returns true if button is currently held down.

- **`bool isButtonClicked(uint8_t buttonIndex) const`**
    Returns true if button was clicked (pressed and released in same frame).

---

## InputConfig

**Inherits:** None

Configuration structure for `InputManager`. Defines the mapping between logical inputs and physical pins (ESP32) or keyboard keys (Native/SDL2).

- **`std::vector<int> inputPins`**: (ESP32) List of GPIO pins.
- **`std::vector<uint8_t> buttonNames`**: (Native) List of scancodes/keys.
- **`int count`**: Total number of configured inputs.

### Constructor

- **`InputConfig(int count, ...)`**
    Variadic constructor to easily list pins/keys.

### Example

```cpp
// 3 inputs: Left, Right, Jump
InputConfig input(3, 12, 14, 27);
```

---

## Touch Input Overview

Touch is **not** wired through `Engine::run()` automatically. Platform code should call `TouchManager::update`, then `getEvents`, then feed the current `Scene` via `processTouchEvents` (see `Engine.h` class comment and [Touch Input Architecture](../architecture/ARCH_TOUCH_INPUT.md)).

**Typical loop fragment**

```cpp
touchManager.update(frameDt);
pixelroot32::input::TouchEvent events[pixelroot32::input::TOUCH_EVENT_QUEUE_SIZE];
uint8_t n = touchManager.getEvents(events, pixelroot32::input::TOUCH_EVENT_QUEUE_SIZE);
if (n > 0) {
    auto scene = engine.getCurrentScene();
    if (scene.has_value() && scene.value()) {
        scene.value()->processTouchEvents(events, n);
    }
}
engine.run();
```

**Scene hooks** (see [API Core](API_CORE.md#scene)): `processTouchEvents` dispatches to UI (if enabled) then `onUnconsumedTouchEvent`.

---

## TouchManager

**Include:** `input/TouchManager.h`

Touch event aggregation layer that polls the active touch adapter (XPT2046 or GT911), clamps coordinates to display bounds, and provides raw touch points. Gesture detection is handled by Engine's TouchEventDispatcher.

### Public Methods

| Method | Description |
|--------|-------------|
| `TouchManager(int16_t maxX, int16_t maxY)` | Clamping bounds (use physical panel size). |
| `bool init()` | Initializes adapter; call **`setCalibration` first** (see [Touch Input Architecture](../architecture/ARCH_TOUCH_INPUT.md)). |
| `void update(unsigned long dt)` | Poll hardware and refresh internal buffer. |
| `void setCalibration(const TouchCalibration&)` | Copies calibration to manager **and** adapter. |
| `uint8_t getTouchPoints(TouchPoint* points) const` | Raw pressed points for the current frame. Returns only active touches - use `Engine::setTouchManager()` for automatic release detection. |
| `uint8_t getActiveCount() const` | Number of active points. |
| `bool isTouchActive() const` | Any finger/stylus down. |
| `bool isConnected() const` | Adapter initialization / health. |

**Constants:** `TouchManager::CIRCULAR_BUFFER_SIZE`, `TOUCH_MAX_POINTS` (5).

---

## TouchCalibration

**Include:** `input/TouchAdapter.h` (class `TouchCalibration`)

Maps raw controller coordinates to screen space; optional **rotation** enum `TouchRotation`.

### API

| Method | Description |
|--------|-------------|
| `static TouchCalibration forResolution(int16_t w, int16_t h)` | Sets `displayWidth` / `displayHeight` and heuristic `scaleX` / `scaleY` (12-bit ADC span). |
| `TouchPoint transform(int16_t rawX, int16_t rawY, bool pressed, uint8_t id, uint32_t ts) const` | Scale, offsets, rotation, clamp. |
| `void setRotation(TouchRotation)` / `applyRotation` | Match TFT rotation when needed. |

### Public Fields

- `scaleX`, `scaleY`, `offsetX`, `offsetY`
- `displayWidth`, `displayHeight`, `rotation`

---

## TouchPoint and TouchEvent

**Includes:** `input/TouchPoint.h`, `input/TouchEvent.h`, `input/TouchEventTypes.h`

### TouchPoint

- **`x`, `y`**, **`pressed`**, **`id`**, timestamp — normalized sample after adapter + clamp.

### TouchEvent

- compact gesture record — **`TouchEventType`** (`TouchDown`, `TouchUp`, `DragStart`, `DragMove`, `DragEnd`, `Click`, …), **`x`**, **`y`**, **`flags`** (e.g. consumed), **`id`**, **`timestamp`**. 

Use **`isConsumed()`**, **`setConsumed()`** for UI routing.

---

## ActorTouchController

**Include:** `input/ActorTouchController.h`

Registers up to 8 `Actor*` targets; on `handleTouch`, performs hit test (with optional slop), drag threshold (`kDragThreshold`), and updates `Actor::position` on drag.

### Public Methods

| Method | Description |
|--------|-------------|
| `bool registerActor(Actor*)` / `unregisterActor` | Pool membership. |
| `void handleTouch(const TouchEvent&)` | Route by `TouchEventType`. |
| `void setTouchHitSlop(int16_t pixels)` | Expand hit-test rect **per side** (resistive calibration slack). |
| `int16_t getTouchHitSlop() const` | Current slop. |
| `bool isDragging() const` | Threshold exceeded and actor locked. |
| `Actor* getDraggedActor() const` | Hit actor (may be non-null before drag threshold). |
| `void reset()` | Clears pool and drag state (not slop). |

---

## XPT2046 Build Flags (ESP32)

Used by `src/input/adapters/XPT2046Adapter.cpp` (SPI or **`XPT2046_USE_GPIO_SPI`** bit-bang). Define via `build_flags` in `platformio.ini`.

| Macro | Default | Purpose |
|-------|---------|---------|
| `XPT2046_USE_GPIO_SPI` | off | Separate GPIO bus (e.g. 2432S028R); pins: `XPT2046_GPIO_IRQ`, `MOSI`, `MISO`, `CLK`, `CS`. |
| `XPT2046_GPIO_SWAP_AXES` | `0` | Swap ADC axes before mapping (vertical/horizontal swapped on screen). |
| `XPT2046_GPIO_MIRROR_X` | `0` | Flip X in screen space after map: `x = displayWidth - x`. |
| `XPT2046_GPIO_VENDOR_COORDS` | `0` | Vendor-style X/Y swap (see adapter). |
| `XPT2046_GPIO_USE_RAW_RANGE` | `0` | Linear map `XPT2046_RAW_*_LO/HI` → full width/height instead of `transform` heuristics. |
| `XPT2046_RAW_X_LO`, `XPT2046_RAW_X_HI`, `XPT2046_RAW_Y_LO`, `XPT2046_RAW_Y_HI` | code defaults | Usable ADC window for raw-range mode. |
| `XPT2046_CAL_OFFSET_X`, `XPT2046_CAL_OFFSET_Y` | optional | Pixel nudge **after** mirror. |

Order after read: **map → vendor coords → mirror X → CAL offsets → clamp** (see [Touch Input Architecture](../architecture/ARCH_TOUCH_INPUT.md)).

---

## Related Documentation

- [API Reference](../API_REFERENCE.md) - Main index
- [API Core](API_CORE.md) - Engine, Scene
- [API UI](API_UI.md) - Touch widgets
- [Touch Input Architecture](../architecture/ARCH_TOUCH_INPUT.md)