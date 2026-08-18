# Sprites Demo Example

> **⚠️ Demonstration example** — This project is provided **as an example** to showcase the capabilities of the PixelRoot32 Game Engine and what you can build with it. It may **not be 100% functional or finished**; some features can be incomplete, experimental, or work in progress.

Demonstrates **2BPP** (4 colors) and **4BPP** (16 colors) **sprites** on the same scene, including **animation** and popup assets under `src/assets/`. Entities are owned by the scene and updated/drawn through the usual **`Scene`** pipeline.

Press **A** to cycle the engine's palette modes:

| Mode | Tables | What you should see |
|---|---|---|
| **Single** | `setPalette(PR32)` | One table. A colour index resolves to the same RGB wherever it is drawn — background and sprites agree. |
| **Dual** | `setDualPalette(NES, GB)` | Two tables. Background repaints from NES, sprites from GB. |
| **Dual inverted** | `setDualPalette(GB, NES)` | The same two tables with the roles swapped: GB background, NES sprites. |

The third mode is not filler. It shows the split is a property of **where a pixel
came from**, not of the palettes: nothing about NES makes it a background table.
Measured against the engine's own `Color.cpp`, the inversion is exact — the
background table in `Dual inverted` is byte-for-byte the sprite table from
`Dual`, and vice versa, all 16 entries.

## The background

The scene behind the sprites — banded sky, sun, stepped hills, chequered ground —
exists so the background table has something to repaint. A black background would
make dual mode indistinguishable from single mode no matter how correct the code.

The strip along the bottom is the **background table itself**, index 0 to 15 left
to right. That is the readable part: a mode switch shows exactly which entries
moved. The sprites in front repaint from the other table at the same moment.

One trap worth naming, because it silently produces a demo that shows nothing:
the ground chequer uses `DarkRed` and `Purple`, not `DarkRed` and `Brown`.
`Brown` is an **alias** of `DarkRed` in `Color.h` (as are `Gold`/`Yellow`,
`LightGray`/`DarkGray`/`Gray`, `Pink`/`Magenta`), so that pair would collapse to
one index and paint a flat block in every palette.

### The part that is easy to get wrong

Drawing a primitive *behind* everything does not make it a background draw.
`Renderer` primitives — `drawFilledRectangle`, `drawLine`, `drawText` — default to
**`PaletteContext::Sprite`**, so in dual mode they resolve through the *sprite*
table unless you say otherwise. Only `drawTileMap` sets the background context
on its own.

The background entity therefore has to declare it:

```cpp
gfx::PaletteContext bgContext = gfx::PaletteContext::Background;
gfx::PaletteContext* savedContext = renderer.getRenderContext();
renderer.setRenderContext(&bgContext);

// ... background draws ...

renderer.setRenderContext(savedContext);   // restore before bgContext dies
```

`setRenderContext` stores the **pointer**, not the value, so the context object
must outlive every draw that uses it — restore before it leaves scope.

Skip this and dual mode renders identically to single mode, which looks like the
feature is broken rather than like the call is missing.

## Requirements (build flags)

- **`PIXELROOT32_ENABLE_2BPP_SPRITES`**
- **`PIXELROOT32_ENABLE_4BPP_SPRITES`**

[`SpritesDemoScene.h`](src/SpritesDemoScene.h) is compiled only when at least one of these is defined (see `#if` guard in the header).

See **`platformio.ini`** for **`native`** and **`esp32dev`**.

## Platforms

| Environment | Display |
|-------------|---------|
| **`native`** | SDL2, 240×240 |
| **`esp32dev`** | **ST7789** 240×240 (TFT_eSPI defines in `platformio.ini`) |

## Features

- **2bpp and 4bpp** sprite drawing on one screen
- **Sprite animation** via demo entities
- **Single and dual palette modes**, toggled at runtime with **A**
- **Asset headers** (`Sprites.h`, `SpritesPopup.h`) as reference for embedding bitmaps

## Documentation links

- [Graphics API — sprites](../../docs/api/graphics.md)
- [Graphics API — palettes](../../docs/api/graphics.md)
- [Core — Scene / Entity](../../docs/api/core.md)

## Build

From **`examples/sprites`**:

```bash
pio run -e native
pio run -e esp32dev
```

## Upload (ESP32)

```bash
pio run -e esp32dev --target upload
```
