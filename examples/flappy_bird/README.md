# Flappy Bird Example

A complete Flappy Bird game implementation using PixelRoot32.

## Features

- Bird movement with physics-based jump
- Dynamic pipe spawning and scrolling
- Collision detection (pipes and boundaries)
- Real-time score display
- Smooth game state transitions (Waiting, Running, Game Over)

## Build

```bash
# Native (PC with SDL2)
pio run -e native

# ESP32
pio run -e esp32c3
```

## Upload to ESP32

```bash
pio run -e esp32c3 --target upload
```

## Controls

- Action Button (0) to jump
- Goal: Fly through the gap in the pipes to score points
- Avoid: Hitting the pipes or the screen top/bottom boundaries

