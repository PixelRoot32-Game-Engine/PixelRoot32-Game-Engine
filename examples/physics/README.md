# Physics Demo Example

Demonstrates the physics system with KinematicActor and RigidActor.

## Features

- KinematicActor movement
- RigidActor with gravity
- AABB collision detection
- Sensors
- Touch input support (ESP32 CYD only)

## Build

```bash
# Native (PC with SDL2)
pio run -e native

# ESP32 (ESP32 Dev Board)
pio run -e esp32dev

# ESP32 CYD (ESP32 CYD Board)
pio run -e esp32cyd
```

