# ESP32 Performance Guide - PixelRoot32

## 🔥 Hot Path Rules

**Hot paths** = `update()`, `draw()`, collision detection loops, audio callbacks.

**Prohibited in hot paths:**

| Feature | Why | Alternative |
|---------|-----|-------------|
| `std::optional` | Extra branching/code size | Raw pointers or sentinel values |
| Virtual calls | Vtable indirection | Templates or function pointers |
| `new` / `malloc` | Heap fragmentation | Pre-allocated pools |
| `std::vector::push_back` | Potential reallocation | Fixed-size arrays with flags |
| Logging / `log()` | String formatting overhead | Debug-only counters or periodic logging |
| `std::rand()` | Slow, uses division | `math::randomScalar()` (Xorshift) |

---

## ⚡ Performance (ESP32 Focus)

### Inlining

- Define trivial accessors (e.g., `getHitBox`, `getX`) in the header (`.h`) to allow compiler inlining.
- Keep heavy implementation logic in `.cpp`.

### Fast Randomness

`std::rand()` is slow and uses division. Use `math::randomScalar()` or `math::randomRange()` (which use optimized Xorshift algorithms compatible with `Fixed16`) for visual effects.

### Collision Detection

- Use simple AABB (Axis-Aligned Bounding Box) checks first. Use Collision Layers (`GameLayers.h`) to avoid checking unnecessary pairs.
- For very fast projectiles (bullets, lasers), prefer lightweight sweep tests:
  - Represent the projectile as a small `physics::Circle` and call `physics::sweepCircleVsRect(startCircle, endCircle, targetRect, tHit)` against potential targets.
  - Use sweep tests only for the few entities that need them; keep everything else on basic AABB to avoid unnecessary CPU cost.

### Dirty Region Selective Clear

Reduces framebuffer clearing overhead by tracking which 8×8 pixel cells were actually drawn to in the previous frame, utilizing a **double dirty grid** pipeline.

- **Benefit**: Replaces full-screen `memset` with targeted **selective row-run 8bpp clearing**. It skips untouched rows entirely and uses `__builtin_popcount` optimizations to quickly identify blocks of dirty cells.
- **RAM cost**: 64–226 bytes (depends on resolution and cell size).
- **When it pays off**: Games with mostly static backgrounds and small moving sprites.
- **Profiling flag**: `PIXELROOT32_ENABLE_DIRTY_REGION_PROFILING=1`
- **Metric**: `dirty_ratio` — fraction of cells marked dirty. Good values are <0.5; >0.8 suggests full clear is cheaper.

```ini
; Enable in platformio.ini
build_flags =
    -DPIXELROOT32_ENABLE_DIRTY_REGIONS=1
    -DPIXELROOT32_ENABLE_DIRTY_REGION_PROFILING=1
```

> **Tip:** If `dirty_ratio` > 0.8, disable dirty regions and use full clear—it avoids the tracking overhead.

### Static Layer Snapshot (`PIXELROOT32_ENABLE_STATIC_LAYER_SNAPSHOT`)

Stops a static layer being redrawn at all, where dirty regions only make the *clear* cheaper.

- **What it is for**: static layers that **game code** draws. `StaticTilemapLayerCache` already covers layers the engine can redraw itself, but it owns the `TileMap4bpp` and repaints it — so it needs one to exist. An isometric or oblique floor has none, because `drawTileMap` assumes axis-aligned cells; such a floor is drawn sprite-per-cell.
- **Benefit**: the layer is drawn once. Later frames restore it — and with dirty regions on, only over the cells the previous frame's movers touched. A 7×7 isometric room drops from 49 `drawSprite` calls (~35,000 pixels of 4bpp decode) to a few hundred bytes copied.
- **RAM cost**: one logical framebuffer of **heap** per allocating scene — ~57 KB at 240×240. This is the whole trade, and it is why the flag defaults to `0`.
- **When it pays off**: when the static layer is expensive to draw and rarely changes, and the DRAM is there to spend. Allocate in `Scene::init()`, never in the loop.
- **When it does not**: a layer that changes every frame. Each `invalidate()` costs a full redraw *plus* a capture.

```ini
build_flags =
    -DPIXELROOT32_ENABLE_STATIC_LAYER_SNAPSHOT=1
    -DPIXELROOT32_ENABLE_DIRTY_REGIONS=1   ; per-cell restore instead of a full copy
```

> **Reality check:** on a 240×240 panel at `SPI_FREQUENCY=40000000`, pushing one frame costs ~23 ms — the frame budget. CPU saved here does not become frame rate on its own; it becomes headroom to raise the SPI clock, enable 12-bit colour, or spend on game logic. Measure the bus before optimising the draw — see the **Display Bandwidth (TFT_eSPI)** section below, and [12-bit Color on the Wire](#12-bit-color-on-the-wire-rgb444) for the cheapest way to cut that 23 ms.

See `examples/iso_dungeon` for a working consumer.

### Single-Core Resource Contention (ESP32-C3)

Single-core architectures (like the ESP32-C3) run the game logic, display transfers, and audio synthesis on a single core.

- **Priority Inversion**: Heavy display transfers (like full-screen U8G2 refreshes) can block the audio task, causing buffer underruns and audio glitches. The engine dynamically detects single-core platforms and elevates the audio task priority (e.g., to `18`) to protect audio streams.
- **Context Thrashing**: An audio priority that is *too* high (e.g., `24`) will preempt the display transfer constantly to synthesize audio, fragmenting the hardware SPI transaction and ballooning draw times (up to 4x). The engine mitigates this by balancing priority, reducing audio buffer block sizes to `128` samples, and using `taskYIELD()` for cooperative multitasking.
- **Float Operations**: Soft-float emulation on the ESP32-C3 is extremely slow. The engine provides fixed-point Q15 implementations for performance-critical inner loops (like `tickEnvelopeQ15`, LFO generation for vibrato/tremolo, HPF filtering, and audio mixer LUTs). Avoid introducing new float-based calculations inside per-sample audio loops or per-pixel drawing loops.

---

## 📺 Display Bandwidth (TFT_eSPI)

The SPI panels in use do not run reliably above **40 MHz**, so a full-frame push is **bus-bound**: no CPU optimization can cross the transfer time. This fixes a hard FPS ceiling per panel and per wire format.

| Panel | Format | Bytes/frame | Transfer @40 MHz | Hard ceiling |
|-------|--------|-------------|------------------|--------------|
| 240×240 | RGB565 | 115,200 | 23.04 ms | 43.4 FPS |
| 240×240 | RGB444 | 86,400 | 17.28 ms | 57.9 FPS |
| 240×320 | RGB565 | 153,600 | 30.72 ms | 32.6 FPS |
| 240×320 | RGB444 | 115,200 | 23.04 ms | 43.4 FPS |

> **Tip:** Lowering `LOGICAL_WIDTH`/`LOGICAL_HEIGHT` buys CPU time but **not** bus time — the scaler upscales to physical during scan-out, so the same number of bytes still goes out. Only `PHYSICAL_DISPLAY_WIDTH`/`HEIGHT` (letterboxing) or a narrower wire format shrink the transfer.

### Deferred DMA Wait

`sendBufferScaled()` leaves the frame's **last DMA block in flight** and flushes it at the top of the next call, so the tail of the SPI transfer overlaps the next frame's `update()` and `draw()` work. The frame cost becomes `max(CPU, transfer)` instead of `CPU + transfer`.

- **Always on** for the TFT_eSPI driver — there is no flag to enable or disable it.
- **Benefit**: closes most of the gap between measured FPS and the bus ceiling above; it does not raise the ceiling itself.
- **Contract**: anything else that touches the SPI bus, the panel, or the line buffers must synchronize first — see [Shared SPI Bus Contract](#shared-spi-bus-contract).

### Shared SPI Bus Contract

**Any code that touches the SPI bus, the TFT, or frees/reallocates the DMA line buffers MUST call `TFT_eSPI_Drawer::waitForPendingDMA()` first.** Skipping it either corrupts the open SPI transaction or reads a buffer that DMA is still streaming. The call is a no-op when nothing is pending.

Already wired inside the engine:

| Call site | Why |
|-----------|-----|
| `TFT_eSPI_TouchBridge` reads | Touch controller shares the display SPI bus |
| `freeScalingBuffers()` | Line buffers are freed while DMA may still read them |
| `~TFT_eSPI_Drawer()` | Same, at teardown |
| `init()` / `setRotation()` | Panel commands must not interleave with a pixel stream |
| `Engine` on a skipped frame, via `DrawSurface::flushPendingTransfers()` | A scene reporting `shouldRedrawFramebuffer() == false` skips `present()`, so nothing else would close the open transaction |

> **Why not just flush in `processEvents()`?** It runs *before* `draw()`, so it would wait out the DMA ahead of the work that block's SPI time exists to overlap — cancelling the deferral outright.

Add the same guard when you introduce a **new peripheral on the shared bus** — an SD card, a second display, or a raw SPI sensor:

```cpp
// Before ANY other transaction on the shared SPI bus
drawer.waitForPendingDMA();
sdCard.read(block, buffer);
```

### 12-bit Color on the Wire (RGB444)

> ⚠️ **Experimental — not yet verified on hardware.** The flag ships **off**. The panel accepting `COLMOD 0x03`, the rendered result and the predicted FPS gain are all still unvalidated. Enable it only on a board you can look at.

`PIXELROOT32_TFT_12BIT_COLOR=1` sends the frame as **12-bit RGB444, two pixels per three bytes**, instead of RGB565. That is a flat **25% reduction in bus time on every frame**, independent of scene content — the only lever here that also helps full-screen scrollers.

```ini
; Enable in platformio.ini (per board)
build_flags =
    -DPIXELROOT32_TFT_12BIT_COLOR=1
```

- **No colors are lost.** The framebuffer is 8bpp RGB332, so a frame carries at most 256 distinct colors. TFT_eSPI expands RGB332 into 8 red levels, 8 green levels and 4 blue levels, and all 256 combinations survive truncation to 4 bits per channel **without a single collision**. The bijection is asserted by `test/unit/test_rgb444/test_rgb444.cpp`, not assumed.
- **It is not bit-exact.** The absolute shade shifts slightly on red and green (blue is exact); what is preserved is the full set of 256 *distinguishable* colors, which is everything the 8bpp framebuffer can express.
- **Width constraint**: only applies when `PHYSICAL_DISPLAY_WIDTH % 4 == 0`. Other widths keep RGB565 and log a warning at init. `pushPixelsDMA()` counts 16-bit words, so bytes-per-line must be even — and an even width is *not* sufficient (242 px → 363 bytes/line). This excludes panels such as the 135×240 ST7789.
- **Memory effect**: each DMA line buffer shrinks 25% (28,800 → 21,600 bytes at 60 lines on a 240-wide panel), minus a 768-byte pair LUT. Net gain in DMA-capable internal RAM.

---

## 💾 Memory & Resources

**📖 For comprehensive C++17 memory management guide, see [Memory Management Guide](../../architecture/memory-system.md)**

### Smart Pointers (C++17)

Use `std::unique_ptr` for **init-time ownership** (Scenes, Actors, UI elements) to automate memory management and document ownership.

- Use `std::make_unique<T>(...)` to create objects during initialization only.
- Pass raw pointers (via `.get()`) to functions that do *not* take ownership (like `addEntity`).
- Use `std::move` only when transferring ownership explicitly.
- ⚠️ **Do not use in hot paths**: `unique_ptr` is for init-time, not runtime game loop.

### Object Pooling

Pre-allocate all game objects (obstacles, particles, enemies) during `init()`.

- Pools are for **runtime** zero-allocation recycling; `unique_ptr` is for **init-time** ownership semantics.
- Pattern: Use fixed-size arrays (e.g., `Particle particles[50]`) and flags (`isActive`) instead of `std::vector` with `push_back`/`erase`.
- Trade-off: Eliminates runtime allocations and fragmentation at the cost of a slightly higher fixed RAM footprint; dimension pools to realistic worst-case usage.

### Zero Runtime Allocation

Never use `new` or `malloc` inside the game loop (`update` or `draw`).

### String Handling

Avoid `std::string` copies. Use `std::string_view` for passing strings. For formatting, use `snprintf` with stack-allocated `char` buffers.

### Scene Arenas (`PIXELROOT32_ENABLE_SCENE_ARENA`)

Use a single pre-allocated buffer per scene for temporary entities or scratch data when you need strict zero-allocation guarantees.

- Trade-off: Very cache-friendly and fragmentation-proof, but the buffer cannot grow at runtime; oversizing wastes RAM, undersizing returns `nullptr` and requires graceful fallback logic.

---

## 🏗️ Build Profiles

PixelRoot32 supports two build profiles for different use cases:

### Embedded Profile (Default)

For ESP32 and resource-constrained hardware:

- **Zero allocation** at runtime
- **No exceptions**, `-fno-exceptions` flag
- **Deterministic** behavior prioritized
- **Modular compilation** to reduce binary size
- **All Hot Path Rules** enforced

### Native Profile (Optional)

For PC simulation and development:

- **Relaxed constraints** for faster iteration
- **Exceptions permitted** if needed for tooling
- **Debug-friendly** features enabled
- **All subsystems** can be compiled in

Use `PLATFORM_NATIVE` flag to switch profiles.

---

## 📊 Recommended Build Profiles

Choose a profile based on your game type to optimize memory usage:

| Game Type | Profile | Enabled | Disabled |
|-----------|---------|---------|----------|
| Arcade shooters/platformers | `arcade` | Audio, Physics, Particles | UI System |
| Puzzle/casual games | `puzzle` | Audio, UI System | Physics, Particles |
| Retro/minimal | `retro` | None | All |
| Educational/tools | `puzzle` or custom | Audio, UI System | Physics, Particles |

**Example platformio.ini configuration:**

```ini
[env:esp32_arcade]
extends = base_esp32, profile_arcade
build_flags =
    ${base_esp32.build_flags}
    ${profile_arcade.build_flags}

[env:esp32_puzzle]
extends = base_esp32, profile_puzzle
build_flags =
    ${base_esp32.build_flags}
    ${profile_puzzle.build_flags}

[env:native_retro]
build_flags =
    -DPLATFORM_NATIVE=1
    -DPIXELROOT32_ENABLE_AUDIO=0
    -DPIXELROOT32_ENABLE_PHYSICS=0
    -DPIXELROOT32_ENABLE_PARTICLES=0
    -DPIXELROOT32_ENABLE_UI_SYSTEM=0
```

---

## 📐 Resolution Scaling

PixelRoot32 separates **logical** resolution (what your game draws at) from **physical** resolution (the actual display), so you can target low pixel counts for performance while filling modern panels.

### When to Use

- Ship gameplay at 128×128 or 160×144 but drive a 240×240 TFT
- Keep UI and physics in logical space; only the final blit scales up

### Configure DisplayConfig

Set `logicalWidth` / `logicalHeight` for the render buffer and `physicalWidth` / `physicalHeight` for the panel. The renderer and input pipeline map between the two.

See [DisplayConfig / Engine](../../api/core.md#engine) and the architecture deep dive [Resolution Scaling](../../architecture/resolution-scaling.md) for implementation details, ESP32 considerations, and coordinate mapping.

---

## 📚 Related Documentation

| Document | Description |
|----------|-------------|
| [Memory Management Guide](../../architecture/memory-system.md) | Complete C++17 memory guide with smart pointers |
| [Platform Compatibility](../platform-compatibility.md) | Hardware matrix and feature support |
| [Architecture Index](../../architecture/architecture-index.md) | Layer architecture and subsystem navigation |
| [Rendering Guide](../rendering.md) | Core rendering pipeline |
| [Configuration Reference](../../api/config.md) | Every build flag, including the TFT_eSPI display flags |
| [Driver Layer](../../architecture/layer-drivers.md) | TFT_eSPI driver internals and the shared SPI bus contract |
| [ESP32 Performance Audit](../../performance-audit-esp32.md) | Full source audit with the bandwidth analysis behind these numbers |
