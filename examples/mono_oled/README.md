# mono_oled

A 1-bit OLED on an ESP32-C3, at 72x40 logical pixels.

Every other example in this catalogue targets a colour TFT. This one targets
the other end of the range: a monochrome SSD1306 panel driven through
`U8G2_Drawer`, on a board with one button and no touch panel. It is the only
example that exercises `PIXELROOT32_USE_U8G2`, and the only one that builds for
`esp32c3`.

## Requirements (build flags)

| Flag | Value | Why |
| ---- | ----- | --- |
| `PIXELROOT32_USE_U8G2` | defined | Selects `U8G2_Drawer` instead of the default TFT_eSPI path |
| `PIXELROOT32_ENABLE_AUDIO` | `0` | Off — the topic is the display |
| `PIXELROOT32_ENABLE_PARTICLES` | `0` | Off |
| `PIXELROOT32_ENABLE_PHYSICS` | `0` | Off |
| `PIXELROOT32_ENABLE_UI_SYSTEM` | `0` | Off — `Renderer::drawText` works without it |
| `PHYSICAL_DISPLAY_WIDTH` / `HEIGHT` | `128` / `64` | The SSD1306 controller's framebuffer |
| `LOGICAL_WIDTH` / `HEIGHT` | `72` / `40` | The window the panel actually shows |
| `X_OFF_SET` / `Y_OFF_SET` | `28` / `24` | Where that window sits inside the framebuffer |

The short flag list is deliberate and is part of the lesson — see below.

## What it demonstrates

### 1. Which half of the renderer survives on a monochrome panel

`U8G2_Drawer` implements `BaseDrawSurface`, but not all of it. Two methods are
deliberate no-ops, and the header says why:

> *Not supported — U8G2 is monochromatic, no benefit from tile-based rendering*

- `drawTileDirect()` — stubbed, so the **tilemap fast path is unavailable**.
- `getSpriteBuffer()` — stubbed, so the **sprite blit path is unavailable**.

There is also no palette: a 1-bit panel has ink and background, so
`setCustomPalette()` has nothing to swap. What remains is the set of primitives
`U8G2_Drawer` overrides with native U8G2 calls — `drawLine`, `drawRectangle`,
`drawFilledRectangle`, `drawCircle`, `drawFilledCircle` — plus text. This scene
uses only those, which is why its flag list turns off every subsystem that
depends on the paths above.

Know this before you pick the hardware. A design built around tilemaps and
4bpp sprites does not shrink onto a monochrome OLED; it has to be redrawn in
primitives.

### 2. Logical size is not physical size

The Beetle's 0.42" panel is wired to an SSD1306 configured for 128x64, but the
glass only exposes a 72x40 window into that framebuffer, starting at (28, 24).
You draw in logical coordinates and the driver applies the offset.

Nothing else in this repository shows a logical viewport offset inside a larger
physical framebuffer, and the failure mode is quiet: get `X_OFF_SET` or
`Y_OFF_SET` wrong and the image is cropped, with no error anywhere. The
GEOMETRY page prints all three pairs — logical, physical, offset — from the
same constants the driver uses, so a mismatch between what the screen says and
what the screen shows is visible rather than inferred.

### 3. Designing for one button and forty rows

`InputConfig(BTN_PIN_NEXT)` declares a single button. There is no D-pad and no
touch, so one press carries the whole interaction: it advances the page and
wraps.

The native environment declares one button too (`SPACE`), rather than the usual
six. That is intentional — a six-button simulator invites a control scheme the
target board cannot run.

At 72x40 with the built-in font at size 1 (6x8 per glyph) you get 12 characters
across and 5 rows down. Every string in the scene is written to that budget.
There is no wrapping and no ellipsis: an overlong line is drawn off the edge.

## Controls

| Input | Native | ESP32-C3 | Action |
| ----- | ------ | -------- | ------ |
| Button 0 | `SPACE` | GPIO 3 | Next page (OUTLINE → FILLED → GEOMETRY → wrap) |

A two-pixel marker sweeps along the bottom edge on every page. It is not
decoration: a frozen U8G2 buffer looks exactly like a correct still frame, so
the marker is the only way to tell a running program from a hung one at a
glance.

## Hardware

DFRobot Beetle ESP32-C3 with the on-board 0.42" SSD1306 OLED.

| Signal | Pin |
| ------ | --- |
| I2C SDA | 5 |
| I2C SCL | 6 |
| Reset | none (`255`) |
| Button | 3 |

## Build

```bash
cd examples/mono_oled

pio run -e esp32c3                    # build for the Beetle
pio run -e esp32c3 --target upload    # flash it

pio run -e native                     # build the SDL2 simulator
```

Measured on a clean `esp32c3` build: **272,664 bytes flash (20.8%)** and
**16,336 bytes RAM (5.0%)**.

### On `PIXELROOT32_NO_TFT_ESPI`

Other examples pass `-D PIXELROOT32_NO_TFT_ESPI` next to `PIXELROOT32_USE_U8G2`.
This one does not, because it makes no difference. That macro is read in exactly
one place — `PlatformDefaults.h:212` — inside the `#else` branch that
`PIXELROOT32_USE_U8G2` already skips.

Measured both ways on clean builds: identical flash and RAM to the byte, and
PlatformIO's dependency finder compiles the same three TFT_eSPI sources in each
case. Nothing references them, so `-Wl,--gc-sections` drops them at link.

### Known limitation: the native environment

`native` reproduces the *geometry* — the same 72x40 logical screen offset inside
a 128x64 surface — but not the panel. `SDL2_Drawer` is a colour surface, so the
1-bit constraint is **not enforced** there: a draw call in a colour other than
black or white shows that colour on the PC and collapses on the Beetle. The
scene therefore draws in `Color::White` only.

Use `native` for layout and logic. Use the hardware to check the display.

> **Status:** `esp32c3` is verified — it builds, links, and runs on a DFRobot
> Beetle ESP32-C3. The `native` environment is **not** verified: it does not
> currently build on the development machine this example was written on
> (MSYS2/MinGW on Windows). The failure is not specific to this example —
> `examples/flappy_bird` fails identically, and so does the compile of
> `ApuCore.cpp` from the PixelRoot32-APU dependency, which no example owns.
> Treat the native target here as untested until that toolchain problem is
> fixed.

## Documentation links

- [Renderer API](../../docs/api/graphics.md)
- [Engine configuration and display flags](../../include/platforms/EngineConfig.h)
- [Driver selection defaults](../../include/platforms/PlatformDefaults.h)
- [`U8G2_Drawer`](../../include/drivers/esp32/U8G2_Drawer.h)
