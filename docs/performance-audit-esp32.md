# PixelRoot32 Engine — Performance Audit

Branch: `refactor/performace`. Method: full-source audit of every subsystem (core loop, rendering, tilemap, sprites/UI/particles/fonts, physics/collisions, audio/APU, input/touch, ESP32 drivers, memory/allocations). All findings cite `file:line` evidence; anything not measurable from code alone is flagged **requires profiling**.

> **Note on status:** the findings below are the original analysis and are left unchanged. Items that have since been implemented carry a **Status** line with the commit that landed them; everything without one is still open. Line numbers in the citations refer to the code as audited.

## 0. Implementation Status

| Finding | Status | Commit |
|---|---|---|
| **C-1 (1)** — deferred final `dmaWait()` | ✅ Implemented, not yet hardware-verified | `d6dc9ae` |
| **C-2** — 1bpp direct-framebuffer fast path | ✅ Implemented, not yet hardware-verified | `d6dc9ae` |
| **H-1** — line-buffer fallback bug (60-line block) | ✅ Fixed, not yet hardware-verified | `d6dc9ae` |
| **C-5** — 12-bit RGB444 on the wire | ⚠️ Implemented behind `PIXELROOT32_TFT_12BIT_COLOR`, **off by default**, hardware spike still pending | `52ed4a0` |
| **C-1 (2)** — full overlap via core-0 display task | Open | — |
| **C-3** — `checkCollision` via SpatialGrid | Open | — |
| **C-4** — partial dirty-region SPI push | Open | — |
| **C-6** — reduced push area (letterbox) | Open (per-game config, no engine work) | — |
| All **H-2…H-7**, **M-\***, **L-\*** | Open | — |

The ESP32 driver is not reachable by the native test runner, so nothing in `d6dc9ae` or `52ed4a0` has been measured on a panel yet. The projected numbers in §7 remain projections.

## 1. Executive Summary

The engine is architecturally disciplined: no per-frame heap allocation anywhere in the game loop, no STL node containers, fixed-size pools, logging compiled out in release, a pre-swapped palette LUT, and a correct intra-frame DMA pipeline. The FPS limiters are therefore *structural*, not hygiene problems. Three bottlenecks dominate:

1. **The SPI bus + synchronous `present()`** — every presented frame pushes the full screen (115.2 KB at 240×240 RGB565). **The panels in use cap out at 40 MHz**, which fixes a **hard ceiling of 43.4 FPS at 240×240 and 32.6 FPS at 240×320** that no CPU optimization can cross. On top of that, `present()` blocks in `dmaWait()` until the last byte is clocked out, so real FPS ≈ `1 / (update + draw + ~23 ms)` — the CPU idles through most of every frame *and* the frame is bus-bound. Raising the clock is not available (the ESP32 SPI divider offers no step between 40 and 80 MHz, and 80 is out of spec for these panels), so the ceiling can only be moved by **pushing fewer bytes**: partial dirty-region windows (C-4), 12-bit color on the wire (C-5), or a smaller push area (C-6).
2. **The 1bpp draw path** (all text, all `MultiSprite`, all 1bpp tilemaps) issues a **virtual `drawPixel()` call per set pixel** instead of using the direct-framebuffer fast path that 2bpp/4bpp already have — 40–100 cycles/pixel vs ~4–8. Full-screen 1bpp content alone can cost 10–19 ms/frame.
3. **Kinematic collision bypasses the broad phase**: `CollisionSystem::checkCollision` linearly scans all entities and `moveAndSlide` invokes it up to ~50 times per actor per frame (~3,200 tests), while the maintained `SpatialGrid` goes unused on this path. Combined with one-`StaticActor`-per-solid-tile level building, platformer characters pay milliseconds per frame.

Fixing (2) and (3) recovers CPU headroom, and the deferred DMA wait closes the gap to the current cap — together they take the shipped examples from ~30 FPS to a bus-bound **~43 FPS on ST7789 240×240**. Going *beyond* that on this hardware requires one of the byte-count levers: partial push (C-4) buys 100+ FPS on localized-motion games but nothing on full-screen scrollers; 12-bit color (C-5) buys a flat 25% everywhere (→ 57.9 FPS ceiling); a letterboxed 240×176 viewport (C-6) buys 59.2 FPS today with zero engine work. **Realistic targets: 43 FPS full-screen scrollers, 60+ FPS for everything else** — and 60 FPS full-screen only if C-5 lands and CPU work fits in 17 ms.

## 2. Target Hardware

| Resource | ESP32 (LX6) | ESP32-S3 (LX7) | ESP32-C3 (RISC-V) | Engine relevance |
|---|---|---|---|---|
| CPU | 2×240 MHz | 2×240 MHz | 1×160 MHz | Main loop core 1, audio task core 0 (`PR32_DEFAULT_MAIN_CORE`/`AUDIO_CORE`, `include/platforms/PlatformDefaults.h:142`) |
| FPU | FP32 add/mul HW; **no HW divide/sqrt**; double = software | same | none (`Scalar` = `Fixed16`) | Every `/`, `sqrtf`, `expf`, `double` in a hot path is multi-cycle software |
| Int divide | multi-cycle (~13 cy) | same | HW (M ext.), multi-cycle | Per-pixel `/`/`%` matters |
| SRAM | ~320 KB (~200–300 usable) | 512 KB | 400 KB | Color pipeline uses ~117 KB @240×240 (see §5) |
| PSRAM | optional, SPI, 40–80 MB/s effective | optional octal | none | Any hot buffer landing there is a 4–10× slowdown |
| Display | SPI TFT (ST7789 @40 MHz, ILI9341 @55 MHz configured) | same | same | 921,600–1,228,800 bits/frame → bus is the frame floor |
| DMA | SPI DMA, internal-RAM buffers only | same | same | Engine uses double line-buffer DMA pipeline correctly |

### 2.1 SPI clock is capped at 40 MHz on the current panels

**Hardware constraint (confirmed by the maintainer): the panels in use do not run reliably above 40 MHz.** This is not a tuning knob — it is a fixed boundary condition for everything below, and it removes "raise the SPI clock" from the optimization set entirely.

It also matters that the ESP32 SPI master derives its clock from the 80 MHz APB and can only select `80/N`. ESP-IDF's divider search uses the 80 MHz "equals sysclk" mode only when the requested frequency exceeds ¾ of APB (60 MHz); anything below that falls to the divider search, whose next step down is 80/2 = 40 MHz. **There is no usable step between 40 and 80 MHz.** Consequence: the `-D SPI_FREQUENCY=55000000` in the ILI9341_2 example configs (`examples/physics/platformio.ini:115`, `examples/2048/platformio.ini:92`, `examples/tic_tac_toe/platformio.ini:114`, `examples/animated_tilemap/platformio.ini:91`) almost certainly resolves to **40 MHz in practice** — those boards are already running at the same clock as the ST7789 ones despite the config claiming otherwise. *Requires verification:* read back the configured divider or scope SCLK.

Pure-bus FPS ceilings at the real 40 MHz, full-panel push (`tft.setAddrWindow(xOffset, yOffset, physicalWidth, physicalHeight)`, `src/drivers/esp32/TFT_eSPI_Drawer.cpp:312` — the window is always the whole physical area):

| Panel | Bytes/frame | Transfer @40 MHz | **Hard ceiling** |
|---|---|---|---|
| ST7789 240×240 | 115,200 | 23.04 ms | **43.4 FPS** |
| ILI9341_2 240×320 | 153,600 | 30.72 ms | **32.6 FPS** |

These ceilings are unreachable-by-construction limits: no CPU optimization can exceed them while a full frame is pushed every frame. **60 FPS at full-panel push is physically impossible on this hardware.** Breaking past them requires pushing *fewer bytes* — see §2.2.

### 2.2 The three bandwidth levers that remain

With the clock fixed, only the byte count is negotiable. Ranked by leverage:

| Lever | Bytes saved | New ceiling (240×240) | Cost |
|---|---|---|---|
| **Partial push** (dirty-region address windows) | scales with actual motion | 100+ FPS on localized-motion games; no gain on full-screen scrolling | High effort, engine change |
| **12-bit color on the wire** (RGB444, COLMOD 0x03) | flat 25% | **57.9 FPS** | Medium effort, needs hardware spike |
| **Reduced push area** (letterbox via `physicalWidth/Height` + offsets) | proportional | 240×176 → 59.2 FPS; 176×176 → 80.7 FPS | Zero engine work, game-design tradeoff |

Note that **lowering `LOGICAL_WIDTH/HEIGHT` does *not* help the bus** — the scaler upscales to physical during scan-out (`sendBufferScaled`, `:323-372`), so a 72×40 logical game still pushes 115,200 bytes. Logical resolution buys CPU time only. The `physicalWidth/physicalHeight` + `xOffset/yOffset` fields in `DisplayConfig` (`include/graphics/DisplayConfig.h:64-72`) are the ones that actually shrink the transfer, and they are already plumbed through `BaseDrawSurface::setPhysicalSize/setOffset` (`include/graphics/BaseDrawSurface.h:76-83`) — this lever is available today with no engine change, only a one-time black fill of the unused border.

## 3. Performance Bottlenecks

### 3.1 Display Driver / Present

#### C-1. Synchronous `present()` — CPU parked in `dmaWait()` for most of the frame — **Critical**
- **Problem:** `sendBufferScaled()` (`src/drivers/esp32/TFT_eSPI_Drawer.cpp:301-485`) overlaps palette conversion with DMA *within* the frame, but conversion costs ~2–4 ms while the SPI transfer costs ~23 ms; the difference is burned in `tft.dmaWait()` (`:464`, `:481`) and the function does not return until the last byte is out. `Engine.cpp:283` has a `// waitForDMA` comment above `processEvents()`, but the ESP32 implementation is `return true;` (`TFT_eSPI_Drawer.cpp:569`) — frame-level overlap was intended and never built.
- **Impact:** avg FPS (dominant). 8 ms of game work → ~32 FPS instead of the 43 FPS bus ceiling.
- **Conditions:** every presented frame, all TFT targets.
- **Solutions (ascending effort):**
  1. Defer the final `dmaWait()+endWrite()` to the start of the next `present()` (or before any other SPI use — touch!). Reclaims one block's transfer time. Difficulty: Low. Risk: Medium (shared-bus touch must wait first).
  2. Full overlap: display task on core 0 with a completed-frame handoff. Requires a second 57.6 KB framebuffer (double buffer) or a "don't draw while pushing" contract. Difficulty: High. Risk: High. API impact on render loop.
- **Expected benefit:** (1) closes most of the gap between current FPS and the 43.4 ceiling; (2) hides the transfer entirely, making the frame cost `max(CPU, 23.04 ms)` instead of `CPU + 23.04 ms`. **Neither raises the ceiling** — at a fixed 40 MHz clock, overlap can only get you *to* 43.4 FPS, never past it. See C-4/C-5 for the levers that move the ceiling itself.
- **Profiling:** the existing `PIXELROOT32_ENABLE_PROFILING` split (scale vs dmaWait, `:520-535`) already measures the wait share — use it before investing in (2).
- **Status:** ✅ **Solution (1) implemented in `d6dc9ae`.** `sendBufferScaled()` leaves the frame's last block in flight and flushes it at the top of the next call; `waitForPendingDMA()` guards the touch bridge, `freeScalingBuffers()`, the destructor, `init()` and `setRotation()`. The per-frame `currentBuffer` reset is gone so the double buffer keeps alternating across frames. Solution (2) (core-0 display task) remains open. Not yet measured on hardware.

#### C-4. Full-frame SPI push is unconditional — dirty regions never reach the bus — **Critical (the ceiling itself)**
- **Problem:** `sendBuffer()` always calls `sendBufferScaled()`, which opens one address window over the entire physical area (`TFT_eSPI_Drawer.cpp:312`) and pushes every pixel, even when the DirtyGrid knows only a handful of 8×8 cells changed. The dirty system's benefit is confined to framebuffer *clears* (`Renderer.cpp:263-277`). The only bus-level saving available today is the all-or-nothing whole-frame skip (`Engine.cpp:291-305`).
- **Impact:** this is what fixes the ceiling at 43.4 / 32.6 FPS. With the clock capped at 40 MHz, it is the single change that can produce a *sustainable* FPS increase rather than merely reaching the current cap.
- **Conditions:** every presented frame. The win scales inversely with how much of the screen actually changes — large on localized-motion games (`snake`, `tic_tac_toe`, `2048`, `brick_breaker`, `space_invaders`, menu/UI screens), near-zero on full-screen scrollers (`midway_clone`, `legend_of_clone`).
- **Solution:** emit one `setAddrWindow` + `pushPixelsDMA` per merged dirty row-span instead of one per frame. `DirtyGrid::clearFramebuffer8FromPrev` (`src/graphics/DirtyGrid.cpp:215-315`) already walks and merges contiguous cells per scanline — reuse that span walker to drive the push. Push the union of `prev ∪ curr` cells so trails are erased.
- **Break-even math:** each window costs 3 commands + 8 data bytes ≈ 2.2 µs of bus plus DC toggles and transaction setup ≈ **10–20 µs all-in**. Against a 23.04 ms full frame that is negligible: 25 spans covering 20% of the screen ≈ 0.5 ms overhead + 4.6 ms transfer = **5.1 ms → ~196 FPS bus ceiling**; even a pathological 100 spans over 50% coverage ≈ 2.0 ms + 11.5 ms = 13.5 ms → **~74 FPS**. The overhead only dominates below roughly 2% coverage, where absolute cost is already trivial.
- **Trade-offs:** implement for `logical == physical` first (span → window is identity); integer scale factors (the `is2x` fast path, `:323`) map cleanly by multiplication; arbitrary LUT scaling is mappable but messier — fall back to full push there. Requires the ≥60% cutover from H-4 so heavy-motion frames degrade gracefully to a single full window rather than 400 tiny ones. API impact: new optional `DrawSurface::sendBufferPartial(const DirtyGrid&)`; games unchanged.
- **Difficulty:** High. **Regression risk:** Medium-High (tearing/ghosting). **Requires profiling:** measure real dirty-cell counts per frame per example first — that number decides whether this pays off for a given game.

#### C-5. 12-bit color on the wire would cut 25% of bus time at zero visual cost — **Critical (opportunity)**
- **Problem/opportunity:** the engine pushes RGB565 (2 bytes/pixel) from an **8bpp framebuffer** — the source has at most 256 distinct colors and the palette LUT already quantizes to RGB332-class precision (`packRgb565ToTftSprite8`, `Renderer.cpp:38-43`). Both ST7789 and ILI9341 support 12-bit RGB444 via `COLMOD` (0x3A) = 0x03, packing 2 pixels into 3 bytes. RGB444 (4/4/4 bits) carries **strictly more precision than the RGB332 the framebuffer already holds**, so the downgrade is visually lossless for this engine.
- **Impact:** flat **25% reduction** in bytes pushed on every frame, unconditionally, independent of scene content — the only lever here that helps full-screen scrollers. 240×240: 115,200 → 86,400 bytes, 23.04 → 17.28 ms, ceiling **43.4 → 57.9 FPS**. 240×320: 30.72 → 23.04 ms, ceiling **32.6 → 43.4 FPS**.
- **Solution:** send `COLMOD 0x03` at init; change `paletteLUT` to hold 12-bit values and pack pixel pairs into 3 bytes while filling the line buffers. The pack is a few extra ALU ops per pixel pair — and the CPU is currently *idle* waiting on DMA (C-1), so this trades a free resource for a scarce one.
- **Trade-offs:** TFT_eSPI does not expose a 12-bit push path natively; the engine already builds its own line buffers and calls `pushPixelsDMA`, so it can pack bytes itself, but the `COLMOD` command and byte-stream format need a hardware spike to confirm on each panel. Odd pixel counts per span need padding handling. Combines multiplicatively with C-4 (25% off whatever partial push already saved).
- **Difficulty:** Medium. **Regression risk:** Medium (panel-specific; keep RGB565 as a build-flag fallback). **Requires validation:** yes — bench on one ST7789 and one ILI9341 board before committing.
- **Status:** ⚠️ **Implemented in `52ed4a0` behind `PIXELROOT32_TFT_12BIT_COLOR`, default `0`.** The driver sends `COLMOD 0x03` at init and packs the line buffers as RGB444; a 768-byte pair LUT serves the 2× fast path. The audit's "visually lossless" claim was tightened when the code landed: the RGB332 → RGB444 mapping is **bijective** — all 256 framebuffer colours stay distinguishable, asserted by `test/unit/test_rgb444/test_rgb444.cpp` — but it is *not* bit-exact, since red and green shades shift slightly (blue is exact). Panels whose `PHYSICAL_DISPLAY_WIDTH` is not a multiple of 4 keep RGB565 and log a warning, because `pushPixelsDMA` counts 16-bit words (an even width is not sufficient: 242 → 363 bytes/line). Line buffers shrink 25% (28,800 → 21,600 bytes each at 60 lines, 240-wide), net of the pair LUT. **The hardware spike is still outstanding** — the panel accepting `COLMOD 0x03`, the rendered result and the predicted FPS gain are all unverified, which is why the flag ships off.

#### C-6. Reduced push area is available today with zero engine work — **Critical (immediate lever)**
- **Opportunity:** `DisplayConfig` already carries `physicalWidth/physicalHeight` and `xOffset/yOffset` (`include/graphics/DisplayConfig.h:64-72`), plumbed to `BaseDrawSurface::setPhysicalSize/setOffset` (`include/graphics/BaseDrawSurface.h:76-83`) and consumed directly by the address window (`TFT_eSPI_Drawer.cpp:312`). A game can therefore push a sub-region of the panel *right now*, with no engine change.
- **Impact at 40 MHz, 240-wide panel:**

  | Push area | Bytes | Transfer | Ceiling |
  |---|---|---|---|
  | 240×240 (full) | 115,200 | 23.04 ms | 43.4 FPS |
  | 240×200 | 96,000 | 19.20 ms | 52.1 FPS |
  | 240×176 | 84,480 | 16.90 ms | 59.2 FPS |
  | 200×200 | 80,000 | 16.00 ms | 62.5 FPS |
  | 176×176 | 61,952 | 12.39 ms | 80.7 FPS |

- **Trade-offs:** black borders on the panel; the unused region must be filled once at init (it is never written again). This is a game-design decision, not an engine defect — but for a game that needs 60 FPS on this hardware today, a 240×176 letterboxed viewport is the only zero-risk way to get it. **Do not confuse this with `LOGICAL_WIDTH/HEIGHT`**, which saves CPU but pushes the same number of bytes.
- **Difficulty:** Trivial (per-game config). **Regression risk:** None (opt-in).

#### H-1. Line-buffer fallback bug: the 60-line DMA block is dead code — **High**
- **Problem:** `buildScaleLUTs()` (`TFT_eSPI_Drawer.cpp:205-217`) tests `if (lineBuffer[0] && !lineBuffer[1])` before buffer 1 is ever allocated — always true — so it unconditionally frees buffer 0 and downgrades to the 30-line fallback. The tuning knob `PIXELROOT32_TFT_ESPI_LINES_PER_BLOCK` does nothing.
- **Impact:** 2× the DMA sync points/descriptor setups per frame (~0.1–0.3 ms + jitter). Mostly a correctness-of-intent bug.
- **Solution:** allocate both buffers at optimal size; on failure of either, free both and retry at fallback size. Verify DMA-capable internal RAM headroom (2×28.8 KB) with audio/WiFi active. Difficulty: Low. Risk: Low-Medium.
- **Status:** ✅ **Fixed in `d6dc9ae`** exactly as proposed — both buffers are allocated at the optimal size and both are freed and retried at the fallback size if either fails. `PIXELROOT32_TFT_ESPI_LINES_PER_BLOCK=60` is now reachable, so the documented default finally applies. `52ed4a0` added the missing log line when the fallback does kick in, so hardware frame timings can be read against the block size the board actually got.

#### H-2. Framebuffer may silently land in PSRAM — **High (PSRAM boards only)**
- **Problem:** `spr.createSprite()` (`TFT_eSPI_Drawer.cpp:82-85`) never calls `spr.setAttribute(PSRAM_ENABLE, false)`; stock TFT_eSPI allocates sprites via `ps_malloc` when PSRAM exists. Every hot loop in the engine (draw, cache memcpy, LUT conversion, transitions) would then read/write PSRAM through the SPI cache — 4–10× slower, defeating all the `MALLOC_CAP_INTERNAL` care taken for line buffers.
- **Impact:** avg FPS, frame time, jitter — unbounded, silent, on S3/WROVER boards.
- **Solution:** one line — `spr.setAttribute(PSRAM_ENABLE, false)` before `createSprite` (57.6 KB fits internal SRAM), behind a config flag for very large logical resolutions. Difficulty: Trivial. **Requires verification:** the dependency is a fork (`library.json:31`) — confirm its default.

#### M-1. Build flags: `-Os` on CYD envs, LTO silently disabled — **Medium**
- **Problem:** `platformio.ini:28-33` sets `-Os` and `-flto` then `-fno-lto` (later flag wins — LTO dead). `esp32dev` example env appends `-O2`; the `esp32cyd` env does not → CYD builds the entire engine, including the scale loops, at `-Os`.
- **Solution:** put `-O2` in `[base_esp32]`; delete one of the contradictory LTO flags. Trade-off: flash size. Difficulty: Trivial.

### 3.2 Rendering / Framebuffer

#### C-2. 1bpp path: virtual `drawPixel()` per set pixel — **Critical**
- **Problem:** `drawSprite` 1bpp (`src/graphics/Renderer.cpp:443-486`), scaled variant (`:680-731`), and 1bpp `drawTileMap` (`:887` calls `drawSprite` per tile) route every set bit through `getDrawSurface().drawPixel()` — virtual dispatch → `TFT_eSprite::drawPixel` (which re-does bounds, rotation and depth branching). The 2bpp (`:545-549`) and 4bpp (`:600-637`) paths already write `logicalFrameBuffer8` directly; 1bpp never got the fast path. Consumers: **all text** (`drawText` → `drawSprite`, `:322-327`), every `MultiSprite` layer (`:669-677`), every 1bpp tile.
- **Impact:** ~40–100 cycles/pixel vs ~4–8. Full-viewport 1bpp tilemap ≈ 10–19 ms/frame — alone below 60 FPS. Text-heavy HUDs pay ms-scale.
- **Solution:** replicate the 4bpp fast path: when `fb8 != nullptr`, resolve + `packRgb565ToTftSprite8` once, write rows directly with clipping hoisted out of the inner loop, `if (bits == 0) continue;` per row, iterate set bits with `__builtin_ctz`. Keep the virtual fallback for U8G2/SDL. No API change. Difficulty: Low-Medium. Risk: Low (golden-image test both drivers).
- **Status:** ✅ **Implemented in `d6dc9ae`** as proposed: the colour pack is hoisted out of both loops (a 1bpp sprite is a single colour), empty rows are skipped, and the virtual path stays as the fallback for surfaces without an 8bpp buffer. `test/unit/test_graphics/test_renderer_sprite1bpp.cpp` asserts both branches produce identical output.

#### H-3. `StaticTilemapLayerCache` is pure overhead while scrolling — **High**
- **Problem:** cached frames `memcpy` a full framebuffer (57.6 KB, `src/graphics/StaticTilemapLayerCache.cpp:215-229`); any camera-sample change forces redraw **plus** a full `memcpy(cache, fb, …)` snapshot — during continuous scrolling (the shipped scrolling examples) the cache adds a full-frame copy on top of the full redraw, every frame. Cache buffer is plain `std::malloc` (`:81`) — can land in PSRAM.
- **Solution:** (a) skip the snapshot while the camera is moving (snapshot only after N stable frames); (b) allocate with `MALLOC_CAP_INTERNAL`; (c) document `setFramebufferCacheEnabled(false)` for scrolling scenes. Difficulty: Low. Risk: Low.

#### H-4. Dirty-region system: default OFF, worst case worse than OFF — **High (footgun)**
- **Problem:** `PIXELROOT32_ENABLE_DIRTY_REGIONS` defaults to 0 (`include/platforms/EngineConfig.h:144-146`) and only optimizes framebuffer *clears* — **the SPI push is always full-frame**. When enabled, a Dynamic full-viewport tilemap without an `animManager` marks ~900 cells every frame (`Renderer.cpp:872-885`, `DirtyGrid.cpp:120-142`), making selective clear ≈ full clear + grid bookkeeping. No "≥N% dirty → full clear" cutover exists.
- **Solution:** cutover in `beginFrame` when `countPrevMarkedCells() > ~60%` of cells. This cutover is also a **prerequisite for C-4** (partial SPI push): without it, a heavy-motion frame would emit hundreds of tiny address windows instead of degrading to one full window.
- Also: floor-division sizing (`DirtyGrid.cpp:90-97`) leaves an unclearable border on non-multiple-of-8 widths (135/170 px panels) — ghosting bug; ceil-divide + clip.

#### M-2. `packRgb565ToTftSprite8` per opaque pixel instead of per palette entry — **Medium**
- **Problem:** palette LUTs are built as RGB565 then every written pixel re-packs 565→332 (~8-10 ops; `Renderer.cpp:546, 610-636`). Full-coverage 4bpp scene ≈ 2–4 ms/frame.
- **Solution:** pre-pack a `uint8_t lut8[16]` at LUT-build time; inner loop becomes `dstRow[x] = lut8[val]`. Difficulty: Low. Risk: Very low.

#### M-3. Scaled path: 2 integer divisions per destination pixel — **Medium**
- **Problem:** `(dstRow*h)/dstHeight`, `(dstCol*w)/dstWidth` per pixel (`Renderer.cpp:702,708`); text at `size > 1` routes every glyph here.
- **Solution:** 16.16 fixed-point stepped accumulators or a per-sprite `srcCol[]` stack map; dedicated integer pixel-replication loop for the integer-scale text case (no division at all). Difficulty: Low-Medium.

#### M-4. Transition effects — per-pixel div/mod and full-frame passes — **Medium (transition-time)**
- DiagonalWipe: `i % width`, `i / width` + `switch` per pixel, both 8bpp and RGB565 paths (`src/graphics/TransitionEffect.cpp:297-299`, `:400-402`) ≈ 6–8 ms/frame during the wipe. Fix: nested y/x loops with incremental `lineValue`, direction hoisted. Difficulty: Low.
- Iris: full-pixel `dx²+dy²` test per frame (`:265-274`, `:484-493`) ≈ 1–3 ms. Fix: per-row span + `memset` prefix/suffix; replace the r²==0 byte loop with `memset`.
- Fade: 256-entry LUT rebuilt every frame even at unchanged progress (`:174-209`). Fix: cache last progress. Low.
- **Interaction bug:** the redraw-skip gate (`Engine.cpp:291-305`) ignores `isTransitioning()` — a scene legitimately returning `shouldRedrawFramebuffer()==false` freezes the visual transition while its timer runs. Fix: `redraw |= sceneManager.isTransitioning();` — trivial.

#### M-5. Latent: unaligned `uint16_t*` loads in 2bpp row reads — **Medium (latent)**
- `Renderer.cpp:532` casts `sprite.data + row*rowStrideBytes` to `uint16_t*`; odd strides (width 12) give odd offsets → LoadStoreAlignment exceptions/UB on Xtensa depending on region. **Requires verification** of exporter padding; fix by byte-assembly or a 2-byte stride contract + `static_assert`.

### 3.3 Game Loop / Timing

#### M-6. No frame pacing; ms-resolution delta — **Medium**
- **Problem:** ESP32 loop free-runs (`yield()` only, `Engine.cpp:316`); `deltaTime` from `millis()` (`:342-344`) aliases 0/1 ms when redraw is skipped and 16/17 ms at ~60 FPS — entity `update(dt)` motion jitters (physics is insulated by the fixed-step `PhysicsScheduler`; entities are not). The heartbeat `millis()` check (`:226,273`) runs even with profiling off.
- **Solution:** µs-derived delta (single `esp_timer` read; convert to ms at the boundary), opt-in frame cap via `vTaskDelayUntil` for redraw-skipping scenes; wrap the heartbeat block in `if constexpr (EnableProfiling)`; reuse the captured timestamp for the touch-release path (`:372`). Difficulty: Medium (ms API touchpoints). Risk: Medium (timing-sensitive game code).

#### M-7. `PhysicsScheduler` burst catch-up — **Medium (jitter)**
- **Problem:** `MAX_STEPS_NORMAL=1`, backlog gate at 2.5 frames then 4 steps in one frame (`include/physics/PhysicsScheduler.h:38-76`): below 60 FPS the sim runs slow, banks time, then lurches 4 steps on an already-slow frame — periodic sawtooth stutter. No render interpolation.
- **Solution:** `MAX_STEPS_NORMAL=2` (repay small deficits immediately) or clamp the accumulator (~2 frames, accepting slow-motion under overload); full fix is render interpolation (`previousPosition` already exists on `PhysicsActor`). Constants: trivial. **Requires profiling** for burst frequency per game.

#### L-1. Synchronous `Scene::init()` inside the SceneSwap tick (`SceneManager.cpp:77-96`) — one-frame spike, possible audio hiccup on single-core; optional incremental-init hook. **Low-Medium, per-game.**

### 3.4 Physics / Collisions / Entities

#### C-3. `checkCollision` bypasses the SpatialGrid: O(N) scan ×~50 per `moveAndSlide` — **Critical**
- **Problem:** `CollisionSystem::checkCollision` (`src/physics/CollisionSystem.cpp:517-555`) iterates **all** entities linearly — never touches the grid. `KinematicActor::moveAndCollide` (`KinematicActor.cpp:61-153`) calls it 10× per probe (1 target + 8 binary-search + 1 refresh); `slide()` runs up to 4 probes; `moveAndSlide` adds depenetration + snap. Worst case one platformer character ≈ 50 scans × 64 entities ≈ **3,200 layer+narrow-phase tests/frame**, each with virtual `getHitBox()` calls.
- **Impact:** 0.5–2 ms/frame per kinematic character; scales linearly with tile-actor count (see H-5), spikes exactly when colliding.
- **Solution:** route through `grid.getPotentialColliders()` (grid already maintained per physics step; `queryId` dedup exists); cache the candidate list once per `moveAndCollide` — the set doesn't change between bisection iterations, only the AABB test does. Caveat: dynamic cells are one-frame stale for moves issued before the physics step; static (tile) cells are always current. Difficulty: Medium. Risk: Medium (one-way-platform ordering).

#### H-5. One heap `StaticActor` per solid tile — **High**
- **Problem:** `TileCollisionBuilder` (`src/physics/TileCollisionBuilder.cpp:41-76`) does raw `new` per non-zero tile: 50+ bodies on a modest map — saturates `MAX_ENTITIES=64`, multiplies every O(N) loop (C-3, the 4 per-step entity passes), fragments the small heap (~120 B ×N), and slows scene load.
- **Solution:** merge horizontal runs of plain-SOLID tiles into single wide `StaticActor`s (keep 1:1 for sensors/consumables — they need per-tile `userData`); allocate from the existing `SceneArena`/`ObjectPool` instead of `new`. Difficulty: Medium. Risk: Low if restricted to SOLID-only runs.

#### H-6. SpatialGrid is screen-sized, clamps world coordinates into border cells — **High (scrolling games)**
- **Problem:** grid dimensioned by `LogicalWidth/Height` (`include/physics/SpatialGrid.h:25-27`); coordinates outside the screen clamp to edge cells (`SpatialGrid.cpp:57-147`). In any scrolling game every off-screen actor collapses into border cells → per-cell caps (12) silently drop actors (**missed collisions/tunneling**) and edge queries degrade toward O(N²).
- **Solution:** wrap instead of clamp (`ix & (cols-1)`, hash-grid semantics — narrow phase rejects aliases; 2 ANDs, zero RAM) or configurable world extent (RAM cost scales with world size). Difficulty: Low (wrap). Risk: Medium.

#### M-8. sqrt/divide density in solver and kinematics — **Medium**
- `needsCCD`: `velocity().length()` (sqrtf) per candidate pair (`CollisionSystem.cpp:614-622`) → compare squared, evaluate once per body.
- Solver: up to 5 float divides per contact per iteration (`:423-437`, `:476-485`); ~600 divides/frame at contact cap → cache `invMass` on `PhysicsActor`, hoist per-contact.
- `sweptCircleVsAABB`: sqrt + up to 8 in-loop divides (`:633-649`) → squared thresholds + additive `t`.
- Kinematic slide: ~12 sqrts + ~10 divides/frame grinding a wall (`KinematicActor.cpp:188-196,536`; `Vector2::normalize` = sqrt + 2 divides) → reuse lengths, derive travel from the bisection fraction.
- All: Difficulty Low(-Medium), Risk Low. No profiling needed for the counts.

#### M-9. Four full-entity passes with virtual filtering per physics step (`CollisionSystem.cpp:118-179, 451-465`) — ~256 pointer-chases + ~192 virtual calls to find the same few bodies. Fix: dense `PhysicsActor*` sublists maintained in add/remove; fuse the integrate passes. **Medium; profiling for gain size.** `getHitBox()` virtual + rebuilt per call (`PhysicsActor.cpp:122-126`) compounds this — de-virtualize after auditing downstream overrides.

### 3.5 Sprites / UI / Particles / Fonts

- **M-10. Static UI re-rasterizes every frame** — `Scene::draw` draws all visible entities every frame (`Scene.cpp:134-152`); `UILabel` re-runs the glyph loop (F1 path) and re-marks cells even when nothing changed; the dirty system never converges for static UI. Fix options: opt-in "static" widget flag (safe), per-widget damage flags (robust, High risk of stale-pixel bugs), glyph-run strip cache. **Requires profiling first** — SPI push may mask the win until C-1 lands.
- **L-2. Buttons recompute label pixel width every frame** for aligned text (`UIButton.cpp:104`, `UITouchButton.cpp:252`; dead branch in `FontManager.cpp:41-46`) → cache on `setLabel`. Trivial.
- **L-3. `UISpriteRow` recomputes icon metrics O(icons × kMaxStates) per frame** (`UISpriteRow.cpp:135-183`) → cache in `recalcSize()`. Trivial.
- **L-4. Particle fade**: float divide + double `resolveColor` + float lerp per particle per frame (`ParticleEmitter.cpp:114-124`) → resolve endpoints at `burst()`, integer lerp `r1 + ((r2-r1)*t8 >> 8)`. Also `update()` ignores `deltaTime` (`:86-87`) — frame-rate-dependent particle physics reads as jitter when FPS varies. Low.
- **L-5. Particle draw**: virtual `fillRect` + `markRect` per 2×2 quad (`:128-146`) → direct fb8 quad writes when available. Low; profiling if multiple emitters.
- **L-6. Depth sort runs every frame when enabled** (`Scene.cpp:106-137`, insertion sort — right choice for nearly-sorted, O(n²) on churn) → skip when no depth key changed; cache sort keys. Low; **requires profiling**.
- Positive: UI layout is event-driven (no per-frame re-layout); `UILabel::setText` change-guarded; tile animation O(anims) with IRAM LUT resolve; particles pool-allocated; UI callbacks are raw function pointers.

### 3.6 Audio / APU

Audio renders in a dedicated FreeRTOS task on core 0 (`ESP32_I2S_AudioBackend.cpp:78-86`; game loop on core 1) — `i2s_write(portMAX_DELAY)` blocks only the audio task. **No FPS impact on dual-core parts.** Findings affect latency, core-0 headroom, and RAM:

- **M-11. `AudioConfig::blockSize` ignored**: backends hardcode 512 samples dual-core (`ESP32_I2S_AudioBackend.cpp:90-91`) → ~70 ms worst-case SFX latency at 22050 Hz and bursty core-0 load. Plumb the config (Easy).
- **M-12. `std::exp` per sample per voice** on float-path sweeps/pitch envelopes (`PixelRoot32-APU/src/ApuCore.cpp:397,528`) ≈ 3–6% of core 0 per sweeping voice; the Q15 path already has the exp2 LUT. Step every N samples or adopt the LUT (golden-PCM churn risk).
- **M-13. Command queue ≈ 12 KB SRAM** (`AudioCommandQueue.h:36-101`: 128 × ~96 B, floor static-asserted at 128) → lower to 32 (~3 KB, saves ~9 KB); move type-specific fields into the union.
- **M-14. NES frame counter uses software `double` per sample when enabled** (`ApuCore.cpp:1392-1401`) → Q16.16 integer cycles. Free when NES mode off (early-out verified).
- **L-7.** Per-sample float divides in mixer/LFO/vibrato (`:2928,2919,2866,2678,2830`) → precomputed reciprocals (golden-PCM caveat). DAC backend: float per sample for 0.7× + 2 KB buffers on a 4 KB task stack (`ESP32_DAC_AudioBackend.cpp:104-135`) → integer scale in place.
- **L-8. Single-core parts**: audio task at priority 18 preempts the game loop ~172×/s, 5–15% CPU in slices → inherent; document voice budgets on C3.
- Positive: SPSC lock-free queue, zero allocation, no per-sample virtual calls, bounded sequencer catch-up.

### 3.7 Input / Touch

- **H-7. Placeholder XPT2046 SPI path fabricates a permanent center-screen touch** (`XPT2046Adapter.cpp:297-327`: raw reads return constants; pressure 100 > threshold 50) on configs where neither GPIO-SPI nor the TFT bridge is active — phantom TouchDown→LongPress forever + per-frame dispatcher work. Fix: return `count=0` or `#error`. Correctness-High, trivial.
- **M-15. TFT_eSPI touch bridge probes the shared display SPI bus every frame, ungated** (`TFT_eSPI_TouchBridge.cpp:59-66`): Z-read + transaction setup per frame even with no finger, serialized against display DMA (worst case lands mid-transfer → ms-scale stall). Fix: gate on the T_IRQ GPIO (one `digitalRead`), poll ≤120 Hz, schedule after `present()`. Easy.
- **M-16. Bit-banged XPT2046 read ≈ 0.3–0.6 ms blocking per frame while touched** (`XPT2046Adapter.cpp:60-156`: 18 register reads × ~80 `digitalWrite/Read` + µs delays, in the game loop) → drop median window 9→5, direct `GPIO.out_w1ts/w1tc` writes, rate-limit. Shows up as drag jitter exactly during interaction.
- **L-9.** `ActorTouchController::onTouchDown` runs a per-actor diagnostic loop with log formatting on every tap (`ActorTouchController.cpp:153-164`) — ~50 ms hitch per tap in debug builds; wrap in `#if PIXELROOT32_DEBUG_MODE`. **L-10.** Multi-touch ids > 0 never get release events (`Engine.cpp:370-374`) — stuck state machines (GT911 boards).
- Positive: fixed ring buffers, integer state-machine math, no allocation, non-blocking button polls.

## 4. Cross-Subsystem Bottlenecks

1. **Present ↔ everything**: while `present()` blocks (~20 ms), no update/physics/draw runs. Every CPU optimization is invisible until C-1 is fixed; conversely, after C-1, previously-masked costs (scale loop at `-Os`, UI re-raster) become measurable. Fix order matters.
2. **Touch ↔ display bus**: the ungated per-frame touch probe (M-15) serializes against display DMA on the shared bus — input polling can stall the present tail and vice versa. Any deferred-dmaWait change (C-1 fix 1) **must** wait before touch transactions or corrupt the panel.
3. **Dirty grid ↔ SPI push**: dirty regions only reduce clears; the unconditional full-frame push (C-4) makes them nearly pointless today. This is the clearest example of a subsystem that is individually well-built but delivers no benefit because a downstream stage ignores its output — the DirtyGrid computes exactly the information the bus needs and then throws it away.
4. **Redraw-skip ↔ transitions** (M-4 interaction bug): the whole-frame skip silently breaks visual transitions.
5. **Tile bodies ↔ kinematics**: H-5 (one body per tile) multiplies C-3 (O(N) scans) — fixing either helps; fixing both is multiplicative.
6. **Heap fragmentation ↔ DMA buffers**: per-tile `new` at scene load interleaves with `heap_caps_malloc(MALLOC_CAP_DMA)` requests; after several scene swaps the 28.8–57.6 KB contiguous DMA allocations are the first to fail (the accidental 30-line fallback currently masks this).
7. **Audio ↔ game loop on single-core** (L-8) and audio hiccup risk during synchronous `Scene::init()` (L-1).

## 5. Memory and Bandwidth Analysis

Static/persistent buffers (TFT color path, logical = physical):

| Buffer | @240×240 | @320×240 | Memory |
|---|---|---|---|
| 8bpp sprite framebuffer | 57.6 KB | 76.8 KB | internal heap (TFT_eSPI) — **verify not PSRAM (H-2)** |
| DMA line buffers ×2 (30-line fallback active) | 28.8 KB | 38.4 KB | internal + DMA-capable |
| palette/x/y LUTs | ~1.5 KB | ~1.7 KB | internal |
| StaticTilemapLayerCache (optional) | 57.6 KB | 76.8 KB | default heap |
| SpatialGrid static tables | ~7.5 KB | ~9.5 KB | .bss (resident even if physics unused — F9) |
| CollisionSystem (contacts + ptrs) | ~5.4 KB | ~5.4 KB | scene |
| Audio command queue | ~12 KB | ~12 KB | scheduler (shrinkable to ~3 KB) |
| Touch queue + audio stacks + I2S DMA | ~7.2 KB | ~7.2 KB | internal |

> **Post-`d6dc9ae` correction:** the "30-line fallback active" row above described the H-1 bug, not the intended configuration. With the fallback logic fixed, a board that gets the 60-line blocks carries **2 × 28.8 KB = 57.6 KB** of line buffers at 240×240 (double the row's figure), and the core pipeline is correspondingly ~146 KB. With `PIXELROOT32_TFT_12BIT_COLOR=1` (`52ed4a0`) those buffers shrink 25% to 2 × 21.6 KB = 43.2 KB, plus a 768-byte pair LUT. Boards that still fail the larger allocation fall back to 30 lines and now log that they did.

Core pipeline ≈ **117 KB** @240×240 before game data; + tilemap cache ≈ 175 KB — tight in ~200–300 KB usable SRAM; @320×240 with all features ≈ 165–230 KB (explains the 30-line fallback). No PSRAM tier exists at all (F8) — the tilemap cache snapshot is the best PSRAM candidate on WROVER/S3 boards (~1.4 ms/frame restore penalty vs freeing ~25% of SRAM; **requires profiling**).

Worst-case bytes moved per frame @240×240: full clear 57.6 KB + scene redraw ~58–115 KB + cache restore 115.2 KB + transition pass 115.2 KB + scan-out conversion ~173 KB ≈ **~520–580 KB CPU traffic**, plus the fixed **115.2 KB SPI push** (23 ms @40 MHz — the frame floor).

## 6. CPU Usage Analysis

- **Core 1 (game):** dominated by `dmaWait()` idle (C-1). Real work: draw path (C-2 for 1bpp content, M-2/M-3 for 4bpp/scaled), physics (C-3/H-5/H-6 for kinematic games), full-frame clears (~0.3–0.5 ms), touch transport (M-15/M-16 during interaction). No allocation, no release logging.
- **Core 0 (audio):** ~few % baseline; +3–6% per sweeping voice (M-12), +2–3% with NES frame counter (M-14); bursty at 23 ms intervals (M-11). WiFi/BT, if enabled, also lands here.
- **Costly-op inventory:** integer div/mod per pixel (DiagonalWipe, scaled path), float div per contact-iteration (solver), sqrtf per candidate pair (CCD) and per slide iteration, `expf` per audio sample (sweeps), software `double` (NES counter only). All have listed integer/precompute fixes.
- **IRAM budget:** ~20+ functions carry `IRAM_ATTR`, including both large `sendBufferScaled` bodies and four physics phases — keep it on the DMA feed loop and `resolveColor`; **measure** (map file) before keeping it on physics phases that chase DRAM pointers anyway.

## 7. Frame-Time Analysis

Representative frame @240×240 ST7789 40 MHz today (kinematic platformer, 4bpp tiles + text HUD):

| Phase | Est. cost | Notes |
|---|---|---|
| update: input + touch transport | 0.1–0.6 ms | worst during drag (M-16) |
| update: entities + physics | 0.5–2.5 ms | C-3 dominates when colliding |
| clear + draw | 2–6 ms | +10–19 ms if 1bpp full-screen content (C-2) |
| cache restore/snapshot | 0.5–1 ms | scrolling: pure overhead (H-3) |
| present (conversion hidden, bus-bound) | **~23 ms** | ~20 ms of it CPU-idle in `dmaWait` |
| **Total** | **~27–33 ms → 30–37 FPS** | ceiling 43.4 FPS |

Jitter sources (frame-time variance, distinct from the mean): physics burst catch-up sawtooth (M-7), collision-probe spikes (C-3 runs the binary search only when colliding), depth-sort churn (L-6), touch-probe DMA stalls (M-15), transition passes (M-4), ms-resolution delta aliasing (M-6), 1 s profiling Serial burst in debug builds (30–45 ms @115200 — raise to 921600).

Projected frame budgets at the fixed 40 MHz clock:

| Stage of work | Present cost | Game work | Frame time | FPS |
|---|---|---|---|---|
| Today | 23.0 ms (blocking) | 4–10 ms | 27–33 ms | 30–37 |
| + deferred wait, 1bpp fast path, grid collision | 23.0 ms (overlapped) | 3–6 ms | ~23 ms | **43** (bus-bound, at ceiling) |
| + 12-bit color (C-5) | 17.3 ms | 3–6 ms | ~17.3 ms | **58** |
| + letterbox 240×176 (C-6) instead | 16.9 ms | 3–6 ms | ~16.9 ms | **59** |
| + partial push (C-4), localized motion | 3–7 ms | 3–6 ms | ~7–10 ms | **100+** |
| + partial push, full-screen scroller | ~23 ms | 3–6 ms | ~23 ms | 43 (no gain — everything is dirty) |

The decisive observation: once the deferred wait lands, **the CPU stops being the bottleneck in every configuration**. Further CPU work (particles, UI caching, solver divides) only buys headroom, not FPS, until a byte-count lever moves the bus ceiling.

## 8. Recommended Optimizations

### Priority 0 — Establish the real baseline (do first, costs nothing)
0. **Verify the effective SPI clock** on the ILI9341_2 boards. If 55 MHz is silently resolving to 40 MHz (§2.1), those configs are misleading and their true ceiling is 32.6 FPS, not the 44.8 the config implies. Correct the example configs to state 40 MHz so nobody tunes against a fiction.

### Priority 1 — Reach the current ceiling (CPU-side; gets you to 43 FPS)
1. ✅ **C-1 (1)** (`d6dc9ae`)**:** defer final `dmaWait()+endWrite()` to next present, guarding the touch bridge on the shared bus. Low effort — this is what converts `CPU + 23 ms` into `max(CPU, 23 ms)`.
2. ✅ **C-2** (`d6dc9ae`)**:** 1bpp direct-fb8 fast path (text, MultiSprite, 1bpp tilemaps). Low-Medium effort, up to 10–19 ms/frame recovered on 1bpp content.
3. **C-3:** route `checkCollision` through SpatialGrid + cache candidates per `moveAndCollide`. Medium effort, 0.5–2 ms/frame per kinematic actor. **Still open.**

*After these three, the engine is bus-bound at 43.4 FPS (ST7789) / 32.6 FPS (ILI9341) and further CPU work does not raise FPS.*

### Priority 1B — Move the ceiling (bus-side; the only path past 43 FPS)
Pick based on the game's motion profile — these are not all-or-nothing, and C-5 composes with both others:

- **C-6 (letterbox)** — available today, zero engine risk. Use when a specific game needs 60 FPS now. 240×176 → 59.2 FPS.
- ⚠️ **C-5 (12-bit RGB444)** (`52ed4a0`, opt-in via `PIXELROOT32_TFT_12BIT_COLOR`, default off) — flat 25% on every frame including full-screen scrollers, no colours lost given the 8bpp framebuffer. **The hardware spike per panel is still the gate**: the code exists, the validation does not. → 57.9 FPS ceiling at full 240×240.
- **C-4 (partial dirty-region push)** — largest win (100+ FPS) but only for localized-motion games; requires H-4's ≥60% cutover first. Measure per-example dirty-cell counts before committing to the effort.

### Priority 2 — High
4. **H-2:** `PSRAM_ENABLE(false)` guard on the sprite framebuffer (verify fork default). Trivial.
5. ✅ **H-1** (`d6dc9ae`)**:** fix the line-buffer fallback logic (restore 60-line blocks). Low.
6. **M-1:** `-O2` in `[base_esp32]`, resolve the LTO flag contradiction. Trivial.
7. **H-5:** tile-run merging + arena allocation for tile bodies. Medium.
8. **H-6:** SpatialGrid wrap-not-clamp (mandatory before trusting scrolling samples). Low.
9. **H-3:** tilemap cache snapshot hysteresis + `MALLOC_CAP_INTERNAL`. Low.
10. **H-4:** dirty-grid ≥60% cutover + ceil-divide border fix. Low.
11. **H-7:** kill the phantom-touch placeholder path. Trivial.

### Priority 3 — Medium
12. **M-2** packed 8-bit palette LUT; **M-3** division-free scaling; **M-4** DiagonalWipe/Iris rewrites + `redraw |= isTransitioning()` (do the gate fix immediately — trivial); **M-5** verify 2bpp stride alignment.
13. **M-8** invMass caching + sqrt/divide removal in solver/CCD/slide; **M-9** dense physics sublists.
14. **M-15/M-16** touch IRQ gating, post-present scheduling, faster GPIO transport.
15. **M-6** µs delta + heartbeat `if constexpr`; **M-7** scheduler pacing constants (after profiling).
16. **M-11** plumb audio blockSize; **M-12** sweep exp stepping; **M-13** shrink command queue to ~3 KB; **M-14** integer NES counter.
17. **M-10** static-UI damage tracking — only after C-1/C-2 land and profiling shows UI share.

### Priority 4 — Low
18. L-2/L-3 cached UI metrics; L-4/L-5 particle integer lerp + direct quads; L-6 depth-sort skip; L-7 audio reciprocals + DAC integer scale; L-9 debug-gate the touch diagnostic loop; L-10 multi-touch release synthesis; L-1 incremental scene init hook; fade-LUT caching; `InteractionTracker` redundant re-sort deletion (`InteractionTracker.cpp:97`); SDL2 `drawTileDirect` palette bug + transparency contract (simulator fidelity).

## 9. Profiling and Benchmarking Plan

1. **Instrument what exists first:** `PIXELROOT32_ENABLE_PROFILING` already splits scale vs dmaWait in `sendBufferScaled` and phases in `Engine`. Raise `monitor_speed`/`Serial.begin` to 921600 first (the 115200 burst costs 30–45 ms/s and contaminates the numbers).
2. **Baseline matrix:** esp32dev + CYD (ST7789 240×240 @40 MHz) and ILI9341 @55 MHz, running `animated_tilemap`, `legend_of_clone` (depth sort + kinematics), `midway_clone` (scrolling + cache). Record: avg FPS, p95 frame time, dmaWait share, per-phase µs.
3. **Targeted measurements before the corresponding fixes:**
   - dmaWait share (justifies C-1 tier 3 vs cheap fixes);
   - collision-phase time via `gProfilerCollisionTime` with 1/2/4 kinematic actors and tile counts 25/50/100 (C-3, H-5);
   - dirty-cell counts per frame on scrolling vs static scenes (H-4 threshold);
   - `sizeof(AudioCommand)` + `uxTaskGetStackHighWaterMark` on audio tasks (M-13, DAC stack);
   - `idf.py size`/map file: IRAM usage per target (F7); heap high-water + largest free block after 10 scene swaps (fragmentation, H-5);
   - PSRAM board run with/without `PSRAM_ENABLE(false)` (H-2).
4. **Regression protection:** golden-image tests (native SDL2 path — after fixing its `drawTileDirect` palette bug) for every renderer/transition change; golden-PCM tests already pin the APU — any reciprocal/LUT audio change must regenerate goldens deliberately.
5. **Effective SPI clock check (P0):** read back the configured divider (or scope SCLK) on an ILI9341_2 board configured at 55 MHz to confirm whether it resolves to 40 MHz. No clock sweep upward — 40 MHz is the panel limit and the ESP32 offers no step between 40 and 80.
6. **Dirty-cell census (gates C-4):** instrument `countPrevMarkedCells()` per frame across `snake`, `2048`, `brick_breaker`, `space_invaders` (localized motion) vs `midway_clone`, `legend_of_clone` (full-screen scroll). The mean and p95 coverage fraction per example is the number that decides whether partial push is worth the High-difficulty effort — and for which games it should be enabled.
7. **12-bit color spike (gates C-5):** ⚠️ **still outstanding, but no longer needs a throwaway branch** — build with `-DPIXELROOT32_TFT_12BIT_COLOR=1` (`52ed4a0`) on one ST7789 and one ILI9341 board. Confirm the panel accepts `COLMOD 0x03`, measure the real transfer time, and eyeball color fidelity against the RGB565 build. Until this runs, the flag stays off by default.

## 10. Final Recommendations

- **Accept the ceiling, then decide how to move it.** With 40 MHz panels, 43.4 FPS (240×240) and 32.6 FPS (240×320) are physical limits at full-frame push. Every CPU optimization in this document is about *reaching* that number, not exceeding it. Do not spend effort on particle lerps or solver divides expecting FPS — they buy headroom and battery, nothing more, until a byte-count lever lands.
- **Sequence matters:** fix the present-path first (P1.1) — it is what makes the CPU work overlap the bus at all; then the 1bpp path and collision broad phase. Measure again *before* starting any P1B work: the dirty-cell census decides C-4, the hardware spike decides C-5. *(P1.1 and the 1bpp path landed in `d6dc9ae`; the collision broad phase is still open, and no hardware measurement has been taken yet.)*
- **The pragmatic 60 FPS answer today is C-6.** If a specific game needs 60 FPS on current hardware this month, a letterboxed 240×176 viewport gets there with a config change and zero regression risk. It costs screen area, not correctness — and unlike C-4/C-5 it needs no engine work and no hardware validation.
- **C-5 is the best engine-wide bet.** A flat 25% bandwidth cut that applies to every game including scrollers, no colours lost because the framebuffer is already 8bpp, and it spends CPU cycles that are currently idle. The TFT_eSPI plumbing unknown is resolved — `52ed4a0` builds the packed line buffers itself behind `PIXELROOT32_TFT_12BIT_COLOR` — but the **panel-side spike is still the gate**, so the flag ships off.
- **Do the trivial batch in one PR:** PSRAM guard, line-buffer fix, build flags, transition redraw gate, heartbeat guard, phantom-touch fix, `InteractionTracker` sort deletion — near-zero risk, immediate wins.
- **Guard the shared SPI bus contract** before any deferred-DMA change: touch bridge must `dmaWait` first. ✅ Done in `d6dc9ae` via the public `TFT_eSPI_Drawer::waitForPendingDMA()`, wired into the touch bridge, `freeScalingBuffers()`, the destructor, `init()` and `setRotation()`. **This is now a standing API contract:** any new user of the shared SPI bus must call it first.
- **Don't invest in per-widget damage tracking, partial SPI windows, or a core-0 display task until profiling after P1** — their value depends on what P1 leaves on the table.
- The engine's zero-allocation, fixed-pool architecture is the right foundation; nothing here requires an architectural rewrite. The two API-visible items are optional: incremental scene init hook and a `blockSize` getter plumb. All P1/P2 fixes are internal.
