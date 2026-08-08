# Room/Screen Example

> **Demonstration example** — showcases the `RoomGraph` API. May not be 100% finished; provided as an example of what you can build.

Minimal 2-room layout: two adjacent rooms (240×240 each), connected left-to-right. Arrow keys trigger `enterRoom`, which clamps the camera to the target room and fires an `onEnter` callback.

## Requirements (build flags)

| Flag | Required | Notes |
|------|----------|-------|
| `PIXELROOT32_ENABLE_GAMEPLAY_ROOM=1` | Yes | Enables `RoomGraph<N>` |
| `PIXELROOT32_ENABLE_AUDIO=1` | No | Audio backend for SDL2/ESP32 |

## Controls

- **Left/Right arrow keys**: transition between Room 0 ↔ Room 1.
- Room label and background color change on transition.

## Build

From `examples/room_screen`:

```bash
pio run -e native
pio run -e esp32dev
```

## Upload (ESP32)

```bash
pio run -e esp32dev --target upload
```
