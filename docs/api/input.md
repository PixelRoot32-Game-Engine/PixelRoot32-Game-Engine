# API Reference: Input Module

> **Source of truth:**
> - `include/input/InputManager.h`
> - `include/input/InputConfig.h`
> - `include/input/TouchManager.h`
> - `include/input/TouchAdapter.h`
> - `include/input/ActorTouchController.h`

## Overview

The Input module manages user interactions across different control methods (keyboard, gamepad, touch). The `InputManager` acts as a unified abstraction layer, polling the underlying hardware (e.g., SDL2 events on PC, or physical buttons/touch drivers on ESP32) and exposing a simple state API for the game.

## Key Concepts

### InputManager

The main interface for querying input state. It maps physical buttons or keys to abstract game actions (Up, Down, Left, Right, A, B, Start, Select).
- Can be queried per-frame (e.g., `isPressed(InputButton::A)` or `justPressed(InputButton::A)`).

### InputConfig

Defines the hardware mapping for physical buttons on the ESP32.
- **Fields**: `pinUp`, `pinDown`, `pinLeft`, `pinRight`, `pinA`, `pinB`, `pinStart`, `pinSelect`.
- Passed during `Engine` initialization.

### Touch Input System

*(Requires `PIXELROOT32_ENABLE_TOUCH=1`)*

A comprehensive touch processing pipeline designed for resistive screens (like the XPT2046) but adaptable to others. It handles raw sampling, noise filtering, calibration, gesture recognition, and event dispatching.

#### Usage Example

```cpp
auto& input = engine.getInputManager();
if (input.hasTouch()) {
    auto& touch = input.getTouchManager();
    if (touch.isTouched()) {
        auto point = touch.getLastPoint();
        // point.x and point.y are already mapped to screen coordinates
    }
}
```

### TouchManager

The central hub for touch processing. It coordinates the hardware adapter, applies calibration matrices, manages the touch state machine (tracking PRESS, DRAG, RELEASE), and dispatches `TouchEvent`s to registered listeners.

### TouchCalibration

Handles the conversion from raw ADC values to logical screen coordinates using a 3-point calibration matrix.
- Often requires a separate calibration scene to capture the points (`updateCalibrationPoint`) and compute the matrix (`computeMatrix`), which should then be saved to non-volatile storage.

### TouchPoint & TouchEvent

- **`TouchPoint`**: Represents a physical coordinate (`x`, `y`, `z`/pressure).
- **`TouchEvent`**: An event dispatched through the UI system (`type`, `point`, `delta`, `handled` flag). Types include `PRESS`, `RELEASE`, `DRAG`, `CLICK`, and `LONG_PRESS`.

### ActorTouchController

A utility component that allows a `PhysicsActor` (or any `Entity`) to respond to touch events. By registering with the `TouchManager`, the controller automatically performs hit-testing against the actor's bounds and translates events.

## Configuration & Notes

### XPT2046 Build Flags

If using the default XPT2046 adapter on ESP32, you must configure the SPI pins in `platformio.ini`:

| Macro | Description |
|-------|-------------|
| `TOUCH_CS` | Chip Select pin |
| `TOUCH_IRQ` | Interrupt Request pin |
| `TOUCH_MOSI` | MOSI pin (can share with display) |
| `TOUCH_MISO` | MISO pin (can share with display) |
| `TOUCH_CLK` | Clock pin (can share with display) |

**Order Pipeline Note**: Events flow from Hardware -> `TouchAdapter` -> `TouchManager` (filters & state machine) -> UI System (top-to-bottom) -> `ActorTouchController` -> Game logic.

## Related Documentation

- [API Reference](index.md) - Main index
- [UI Module](ui.md) - Touch widgets and hit testing
- [Touch Input Architecture](../architecture/touch-input.md) - Deep dive into the touch pipeline