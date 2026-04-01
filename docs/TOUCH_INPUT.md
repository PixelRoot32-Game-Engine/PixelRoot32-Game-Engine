# Touch Input Architecture

This document describes how resistive and capacitive touch integrate with PixelRoot32: data flow, calibration, and platform responsibilities.

For public API details (methods, parameters), see [API Reference — Input Module](API_REFERENCE.md#touch-input-overview).

## 1. Design principles

- **Engine does not own touch hardware.** `Engine::run()` updates buttons via `InputManager` only. Touch sampling lives in platform code (`setup` / `loop` or equivalent) because controllers and buses differ per board.
- **Single coordinate space.** After the active adapter runs, coordinates are **screen pixels** in the same range as `PHYSICAL_DISPLAY_WIDTH` × `PHYSICAL_DISPLAY_HEIGHT` (or your `TouchManager` constructor bounds). Game logic, UI hit tests, and debug overlays should use this space.
- **UI before gameplay.** `Scene::processTouchEvents` runs `UIManager::processEvents` first (when `PIXELROOT32_ENABLE_UI_SYSTEM`), marks events **consumed**, then invokes `onUnconsumedTouchEvent` for each remaining event.

## 2. Pipeline (high level)

```text
Hardware (XPT2046, GT911, …)
    → Adapter readImpl()  [median sample, optional GPIO bit-bang SPI]
    → TouchCalibration / compile-time calibration macros
    → TouchPoint (x, y, pressed)
    → TouchManager::update()
         → clamp to display bounds
         → TouchEventDispatcher (gestures: down, drag, up, …)
    → TouchManager::getEvents()
    → Scene::processTouchEvents()
         → UIManager (optional)
         → onUnconsumedTouchEvent()
```

Optional gameplay layer: **`ActorTouchController`** consumes `TouchEvent`s in `onUnconsumedTouchEvent` to drag registered `Actor`s (hit test, drag threshold, position update).

## 3. Key components

| Component | Role |
|-----------|------|
| `TouchManager` | Polls adapter, clamps points, owns `TouchEventDispatcher`, exposes `getEvents` / `getTouchPoints`. |
| `TouchCalibration` | `forResolution(w,h)`, `transform`, rotation; shared with adapter via `setCalibration`. |
| `XPT2046Adapter` | ESP32 XPT2046: shared TFT SPI or **GPIO bit-bang** (`XPT2046_USE_GPIO_SPI`) for boards like ESP32-2432S028R. |
| `TouchEventDispatcher` | Converts point stream into `TouchEvent` gestures (`TouchDown`, `DragMove`, `TouchUp`, …). |
| `ActorTouchController` | Drags actors from touch; optional **hit slop** for resistive alignment. |
| `Scene::processTouchEvents` | Central entry for a frame’s touch batch; virtual `onUnconsumedTouchEvent` hook. |

## 4. Per-frame integration (recommended)

Call **once per frame**, in order:

1. `touchManager.update(deltaTimeMs);`
2. `TouchEvent buf[TOUCH_EVENT_QUEUE_SIZE];`
3. `uint8_t n = touchManager.getEvents(buf, …);`
4. If `n > 0` and a scene is active: `scene->processTouchEvents(buf, n);`
5. `engine.run();` (or your update/draw split).

Touch is intentionally processed **before** `engine.run()` in this pattern so gameplay reacts in the same frame as the sample.

## 5. Initialization order (critical on ESP32)

`TouchManager::init()` forwards the internal `TouchCalibration` to the adapter. The default `TouchCalibration` struct uses **320×240** defaults until you call `forResolution`.

**Always** set calibration for your real panel **before** `init()`:

```cpp
TouchCalibration cal = TouchCalibration::forResolution(PHYSICAL_DISPLAY_WIDTH, PHYSICAL_DISPLAY_HEIGHT);
touchManager.setCalibration(cal);
touchManager.init();
```

Wrong dimensions break horizontal mirror span (`displayWidth - x`), raw-to-screen mapping width/height, and clamping.

## 6. XPT2046 GPIO path (e.g. Sunton 2432S028R / CYD)

When `-D XPT2046_USE_GPIO_SPI` is set, the adapter uses a separate bit-banged bus (pins overridable via `XPT2046_GPIO_*` macros). After reading raw ADC channels and optional axis swap, coordinates go through this **order** (see `XPT2046Adapter.cpp`):

1. Map to screen (either `TouchCalibration::transform` **or** linear `XPT2046_GPIO_USE_RAW_RANGE` using `XPT2046_RAW_*_LO` / `HI`).
2. Optional `XPT2046_GPIO_VENDOR_COORDS` rotation-style swap.
3. **`XPT2046_GPIO_MIRROR_X`** — horizontal flip in screen space (`x = displayWidth - x`).
4. **`XPT2046_CAL_OFFSET_X` / `Y`** — final nudge in **post-mirror** pixels.
5. Clamp to `calibration.displayWidth` / `displayHeight`.

Optional debugging: **`XPT2046_DEBUG_RAW_TOUCH`** logs raw samples (throttled) while pressed to tune `RAW_*` bounds.

Typical alignment flags for CYD-class boards (exact values are project-specific):

- `XPT2046_GPIO_SWAP_AXES` — finger vertical/horizontal matched wrong screen axes.
- `XPT2046_GPIO_MIRROR_X` — left/right inverted after swap.
- `XPT2046_GPIO_USE_RAW_RANGE` — map real ADC span to full screen instead of assuming 0–4095.

## 7. Gameplay hit testing and slop

Resistive stacks often report a cluster of pixels offset from the visible sprite. **`ActorTouchController::setTouchHitSlop(n)`** expands the hit rectangle by `n` pixels per side for picking only (not rendering).

## 8. Cross-platform vs. board-specific configuration

To port touch features to a new board, it's important to understand which parts of this architecture apply generally and which are specific to particular hardware setups.

### Generic engine components (apply to all boards)

The following aspects of the touch system are hardware-agnostic and work exactly the same regardless of your board:

- **The data flow**: `TouchManager` -> `TouchEventDispatcher` -> `Scene::processTouchEvents` -> UI/Gameplay.
- **The coordinate space**: Your game logic always receives touch events in scaled screen pixels.
- **Gameplay tools**: `ActorTouchController` and its hit-slop mechanics work identically on any touchscreen.
- **Initialization**: You must always call `TouchCalibration::forResolution` and `touchManager.setCalibration()` **before** `touchManager.init()`.

### Hardware-specific configuration (board-dependent)

The underlying driver and its build flags vary drastically by device. You cannot copy-paste the `platformio.ini` touch flags from one board to another and expect them to work without adjustment.

**Example 1: ESP32-2432S028R (CYD)**
This board uses an XPT2046 resistive touch controller on a **separate bit-banged GPIO bus**, not the main TFT SPI bus. It requires specific macro overrides:
- `-D TOUCH_DRIVER_XPT2046`
- `-D XPT2046_USE_GPIO_SPI`
- Specific pins (`XPT2046_GPIO_MOSI`, etc.)
- Panel-specific tuning (`XPT2046_GPIO_SWAP_AXES`, `XPT2046_GPIO_MIRROR_X`, `XPT2046_GPIO_USE_RAW_RANGE`).

**Example 2: A standard ESP32 with shared SPI XPT2046**
Many TFT shields share the main SPI bus for both the display and the touch controller.
- You would **not** use `XPT2046_USE_GPIO_SPI`.
- The `TFT_eSPI` library typically handles the low-level SPI sharing via the `TFT_eSPI_TouchBridge` or built-in calibration.
- You only need `-D TOUCH_DRIVER_XPT2046` and `-D PIXELROOT32_USE_TFT_ESPI_DRIVER`.

**Example 3: A capacitive touchscreen (e.g., GT911)**
Capacitive screens use I²C and usually report perfect screen coordinates out of the box, requiring no axis swapping or offset calibration.
- You would use `-D TOUCH_DRIVER_GT911` instead.
- Resistive calibration flags (like `SWAP_AXES` or `MIRROR_X`) are not used.

## 9. Related documentation

- [API Reference — Touch Input](API_REFERENCE.md#touch-input-overview)
- [Architecture — InputManager](ARCHITECTURE.md) (subsection documents touch parallel path)
- `include/core/Engine.h` — documented touch loop contract in class comment
