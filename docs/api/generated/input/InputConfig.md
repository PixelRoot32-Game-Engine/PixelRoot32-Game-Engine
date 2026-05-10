# InputConfig

<Badge type="info" text="Struct" />

**Source:** `InputConfig.h`

## Description

Configuration structure for the InputManager.

Maps logical inputs to physical GPIO pins (ESP32) or keyboard scancodes (Native/SDL2).
Uses fixed-size std::array instead of std::vector for zero-allocation and
deterministic memory usage on ESP32.

Usage:
// ESP32
pr32::input::InputConfig config(BTN_UP, BTN_DOWN, BTN_LEFT, BTN_RIGHT);

// Native (SDL2)
pr32::input::InputConfig config(SDL_SCANCODE_UP, SDL_SCANCODE_DOWN);

// Empty
pr32::input::InputConfig config{};
InputManager

## Methods

### `if constexpr(sizeof...(args) > 0)`
