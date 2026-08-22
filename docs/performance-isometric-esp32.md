# PixelRoot32 — Isometric Rendering Performance on ESP32

Scope: identify what limits FPS in isometric scenes on the ESP32, given the
current render architecture and the ST7789 240×240 @ 40 MHz target panel.
Evidence: full-source audit of the render pipeline plus a real profiling
session from `Isometric Example Export`.

Companion document: `docs/performance-audit-esp32.md` (general ESP32 audit).
Finding IDs (C-1, C-5, M-2, …) reference that document.

## TL;DR

Two limits, different nature:

1. **Draw (CPU) 21–43 ms** — what *currently* limits FPS. The isometric
   tilemap is fully redrawn every frame (dynamic layer, no cache, no dirty
   regions).
2. **Present (SPI bus) ~18 ms** — a hard ceiling of **55 FPS**. Not the
   current limiter, but the floor every CPU optimization converges toward.

Measured FPS 17–25 ≈ `1 / (Draw + Present)`. Draw varies 21→43 ms because the
camera scrolls into denser regions of the map (more non-empty tiles per
viewport) and recovers in sparser ones.

## 1. Hardware boundary conditions

| Resource | Value | Consequence |
|---|---|---|
| Panel | ST7789 240×240 | 115,200 bytes/frame at RGB565 |
| SPI clock | 40 MHz (panel limit) | 23.04 ms/frame nominal; ~18 ms measured with deferred tail |
| SPI divider | ESP32 has no step between 40 and 80 MHz | "raise the clock" is not available |
| CPU | 2×240 MHz LX6 | draw path runs on core 1 |
| FPU | FP32 add/mul only; slow divide/sqrt | avoid floats in hot paths |

Raising the SPI clock is out of scope (no usable divider step and the panel is
out of spec above 40 MHz). The ceiling only moves by pushing **fewer bytes**,
never by CPU work.

## 2. Render pipeline as it stands

```
Engine::draw()
 ├─ beginFrame()          → clearBuffer() full 57.6 KB (dirty regions OFF)
 ├─ sceneManager.draw()
 │    ├─ drawTileMap(ground, 0,0, LayerType::Dynamic, ISO_PROJECTION)
 │    └─ player.draw()    → drawSprite 64×64 4bpp
 └─ present()             → sendBufferScaled() (bus-bound)
```

Key facts from source:

- `PIXELROOT32_ENABLE_DIRTY_REGIONS` defaults to **0**
  (`include/platforms/EngineConfig.h:145`). With it off,
  `Renderer::beginFrame()` does a full `clearBuffer()` every frame
  (`src/graphics/Renderer.cpp:254-258`). The dirty grid is compiled out; the
  tilemap's `markRect` calls are `if constexpr`-gated
  (`src/graphics/Renderer.cpp:1229`).
- `drawTileMapProjectedImpl` (`src/graphics/Renderer.cpp:1125`) culls cells via
  `cellRangeForScreenRect`, then for each non-empty cell computes
  `cellToScreenX/Y` and blits through `drawSpriteInternal`
  (`src/graphics/Renderer.cpp:1217-1259`).
- `drawSpriteInternal` 4bpp (`src/graphics/Renderer.cpp:674`) writes directly
  to the 8bpp framebuffer with a hoisted packed LUT, but iterates **all 1024
  nibbles** of a 32×32 tile — including the ~50% transparent diamond padding.
- `sendBufferScaled` (`src/drivers/esp32/TFT_eSPI_Drawer.cpp:457`) overlaps
  palette conversion with DMA and defers the final block to the next frame
  (`:692-696`), but the intermediate `dmaWait()` calls still serialize the game
  loop against the bus for ~16 ms/frame.
- Projection math is integer-only: `cellToScreenX/Y` are multiply+add
  (`include/math/Projection.h:144-152`), `screenToCell` is one floor division
  by a power-of-two determinant (`:172-186`). **Projection is not a
  bottleneck.**

## 3. Real-session log analysis

`Isometric Example Export`, ESP32dev + ST7789 240×240, `-Os`,
`PIXELROOT32_ENABLE_PROFILING` + `PIXELROOT32_ENABLE_DEBUG_OVERLAY` on.

```
[TFT sendBufferScaled avg/25 fr] total 18070u | setup 27u | scale 2195u |
        dmaWait* 15683u | pushDMA 149u | endWrite* 13u | 55 FPS
FPS: 25 | Update: 458us | Events: 0us | Draw: 21773us | Present: 18916us
...
FPS: 17 | Update: 661us | Events: 1us | Draw: 43523us | Present: 19336us  ← peak
FPS: 21 | Update: 542us | Events: 0us | Draw: 31155us | Present: 19077us
```

Reading the numbers:

| Phase | Cost | Notes |
|---|---|---|
| `dmaWait` | 15.68 ms | bus occupied — the 55 FPS ceiling |
| `scale` | 2.19 ms | 8bpp→RGB565 1:1 (logical == physical, no 2×) |
| `Present` | ~19 ms | ~constant; deferred tail only recovers ~2 ms |
| `Update` | ~0.5 ms | player logic + input, negligible |
| `Collision` / `PhysicsInt` | ~0 | no physics in this scene |
| **`Draw`** | **21–43 ms** | **the variable, dominant cost** |

The deferral (`dmaPending`, `:692-696`) recovers only the last block's transfer
(~2.2 ms) by overlapping it with the *next* frame's update+draw. It does not
hide the remaining ~16 ms of `dmaWait` inside `present()`, so each frame is
still serialized as `Draw + Present`.

FPS tracks Draw inversely (Draw 21.7 ms → 25 FPS; Draw 43.5 ms → 17 FPS). The
variation follows camera position: denser ground regions blit more non-empty
tiles.

## 4. Isometric-specific costs

| Cost | Detail | Weight |
|---|---|---|
| Per-tile 4bpp alpha blit | 1024 nibbles/tile, ~50% transparent padding still iterated | **dominant** |
| Full redraw every frame | `LayerType::Dynamic`, no cache, no dirty regions | **dominant** |
| Diamond overdraw | painter-order overlap → ~1.5–2× pixel writes vs screen | high |
| Full invalidation on scroll | camera follows player; every tile shifts each frame while walking | medium |
| Projection arithmetic | int mul/add + shift-divide | ~0 |

The visible set is roughly a 240×240 diamond of cells. With the map's 36×29
extent and the `{464,24, 16,8, -16,8}` basis, `cellRangeForScreenRect` returns
~250–300 cells; the ground's non-empty subset (~50–75% of it) is what actually
blits. That is ~150–200 tiles × 1024 px ≈ 150–205k nibble iterations per frame
at ~10–20 cycles each — the 13–40 ms that dominates `Draw`.

## 5. Bottleneck determination

1. **Primary (current limiter): CPU draw of the isometric tilemap.** Dynamic
   layer + no cache + per-tile full 4bpp blit. Variable with map density.
2. **Secondary (hard ceiling): SPI present.** ~18 ms = 55 FPS max at full
   240×240 RGB565 push. Irrelevant only until CPU draw drops below ~5 ms.

The two compound: even an idle scene (zero draw) cannot exceed 55 FPS today;
and the current draw cost keeps the game at 17–25 FPS regardless of the bus.

## 6. Feasible optimizations (impact × complexity)

Ordered; all preserve the generic engine design (opt-in per layer/flag).

### P0 — immediate, trivial risk

1. **12-bit color on the wire (C-5)** — already implemented behind
   `PIXELROOT32_TFT_12BIT_COLOR` (`52ed4a0`), default off. 25% fewer bus bytes
   → ceiling 55 → **73 FPS**. Gate is the outstanding per-panel hardware spike.
2. **Letterbox (C-6)** — `physicalWidth/Height` + offsets already plumbed
   (`include/graphics/DisplayConfig.h`, consumed at
   `TFT_eSPI_Drawer.cpp:488`). 240×200 → 52 FPS; 240×176 → 59 FPS. Zero engine
   work, per-game config.
3. **Serial at 921600** — the 115200 profiling burst contaminates numbers by
   ~30–45 ms/s (audit §9).
4. **Pre-packed 8bpp LUT (M-2 extension)** — `drawSpriteInternal` re-packs the
   16-entry LUT **per tile call** (`Renderer.cpp:696-701`) even though the
   tilemap loop already caches the RGB565 LUT across tiles
   (`Renderer.cpp:1247-1257`). Hoisting the 8bpp pack to that same cache level
   removes ~180 × 16 pack ops/frame.

   **Status: implemented as change `iso-perf-blit-fastpath`.** A new optional
   `packedLUT` parameter on `drawSpriteInternal` (4bpp + 2bpp) lets the
   projected tilemap loop build the 8bpp table once at the cache-invalidation
   boundary and pass it through. On cache hit (consecutive tiles share
   palette), the per-call pack is skipped entirely. Two distinct arrays
   (`packedCachedLUT4bpp[16]`, `packedCachedLUT2bpp[4]`) are selected by
   `if constexpr` on `TileT` for zero runtime branch.

### P1 — the real CPU win, isometric-specific

5. **Static ground + restore.** The ground layer is static; only the camera and
   the player change. The example chose `Dynamic` because it is the only layer
   (the map must repaint over the player's trail). Two compatible fixes, both
   opt-in:
   - Extend `StaticTilemapLayerCache` to the **projected overload** — it
     currently only calls the orthogonal `drawTileMap`
     (`src/graphics/StaticTilemapLayerCache.cpp:25`). Idle frame → restore =
     one 57.6 KB `memcpy` (~0.5 ms) instead of a 21–43 ms redraw.

     **Status: covered by `StaticLayerSnapshot`.** The projected-path equivalent
     of "static ground + restore" already exists as
     `LayerType::Static + ISO_PROJECTION + StaticLayerSnapshot` (see
     `examples/iso_dungeon/src/RoomRenderer.cpp:83-94`). Extending
     `StaticTilemapLayerCache` for projected overloads would duplicate that
     API; the `iso_dungeon` example is the reference consumer.
   - **Dirty-skip per tile** — blit only tiles whose screen rect intersects
     dirty cells. Player idle → 2–4 tiles instead of ~200.

     **Status: implemented as change `iso-perf-cached-ground`.** Skip predicate
     inserted in `drawTileMapProjectedImpl` (`src/graphics/Renderer.cpp:1226+`),
     gated on `PIXELROOT32_ENABLE_DIRTY_REGIONS=1` + `LayerType::Dynamic` +
     camera-stationary + `selectiveRestoreValidThisFrame_`. The
     `selectiveRestoreValidThisFrame_` gate is required so the predicate is
     disabled when `beginFrame()` ran a full `clearBuffer()` (no surviving
     pixels outside prev-dirty to skip into). New helper
     `DirtyGrid::intersectsPrevDirty` carries the per-tile screen-rect check.
     See `openspec/changes/iso-perf-cached-ground/specs/tilemap-projected-dirty-skip/spec.md`.
6. **Span-limited 4bpp blit** — precompute per-row opaque `minX/maxX` per tile
   at load/export; skip leading/trailing transparent nibbles. ~50% fewer
   iterations for diamond tiles, generic for any 4bpp sprite with padding.

   **Status: implemented as change `iso-perf-blit-fastpath`.** Optional
   `rowMinX` / `rowMaxX` pointer fields on `Sprite4bpp` and `Sprite2bpp` carry
   the per-row opaque span; `drawSpriteInternal` clamps the inner column
   loop to `[rowMinX[row], rowMaxX[row])` when both are non-null and
   `flipX == false`. `flipX` always bypasses span limits (mirrored layout
   invalidates the precomputed min/max). New helper `computeSpanTable()`
   populates caller-owned static buffers at init time — `iso_dungeon`
   computes spans for all 6 of its tiles. Convex-row limitation: only
   leading/trailing transparent runs are skipped; interior transparent
   pixels still iterate. See
   `openspec/changes/iso-perf-blit-fastpath/specs/sprite-blit-fastpath/spec.md`.

### P2 — only if needed (continuous scroll)

7. Overdraw reduction (skip fully-covered tiles) — complex, ghosting risk. Low
   priority for now.
8. Partial SPI push (C-4) — **does not apply**: isometric scrolling dirties the
   whole screen every frame.

## 7. What NOT to do

- **Raise the SPI clock** — no 40→80 MHz divider step, panel out of spec.
- **Micro-optimize CPU elsewhere** (solver, particles, depth sort) — this scene
  runs `PhysicsInt 0`, `Collision 0`, and the projected path is already
  painter-order with no depth sort.
- **Put hot buffers in PSRAM** — 4–10× slower.
- **Lower `LOGICAL_WIDTH/HEIGHT`** — does not reduce bus bytes (scaler refills
  to physical).
- **Depth-sort the ground** — `rowMajorIsPainterOrder(ISO_PROJECTION)` already
  makes row-major iteration back-to-front; adding a sort would be pure cost.

## 8. Realistic targets (ST7789 240×240 @ 40 MHz)

| Scenario | Draw | Present | FPS |
|---|---|---|---|
| Today (walking) | 21–43 ms | 18 ms | 17–25 |
| + static ground / cache (idle) | ~0.5 ms | 18 ms | ~45–55 |
| + span-limited blit (walking) | ~8–12 ms | 18 ms | ~33–38 |
| + C-5 12-bit color | — | 13.5 ms | ~55–73 |
| + letterbox 240×176 | — | 16.9 ms | ~59 |

## 9. Recommendation

The current limiter is the **per-tile redraw of the isometric tilemap**, not
the bus. Recommended order:

1. Make the ground static + restore (cache/dirty) — biggest FPS recovery.
2. Span-limited 4bpp blit — cuts the remaining per-tile cost roughly in half.
3. Validate 12-bit color (C-5) on a real panel — moves the ceiling past 55.

All three are opt-in and preserve the engine's layer/flag architecture. The
bus remains the long-run floor; 60+ FPS at full 240×240 requires C-5 or C-6,
not more CPU work.
