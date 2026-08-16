# Clone of Midway — continuous scrolling, and what it costs


> **⚠️ Demonstration example** — This project is provided **as an example** to showcase the capabilities of the PixelRoot32 Game Engine and what you can build with it. It may **not be 100% functional or finished**; some features can be incomplete, experimental, or work in progress.


A vertically scrolling shooter over the Pacific. One stage, one aircraft, a sea
that never stops moving.

The name is the honest label. This is a **clone** of Capcom's *1943: The Battle
of Midway*, built to exercise one engine behaviour the rest of the examples
never touch: a camera that moves **every single frame**. All the art is
original — no ripped sprites, no ripped maps, no Capcom assets anywhere in the
tree.

It exists to answer a question the other tilemap examples cannot:

> Every tilemap example in this repository keeps its camera still and lets
> `StaticTilemapLayerCache` replay the terrain with one `memcpy`.
> **What happens when the camera never stops?**

The answer is not the one the setup implies, and it is the most useful thing
this example has to say. See [The frame budget](#the-frame-budget).

```
            world y = 0     ← north, end of stage
        +---------------+
        |   [carrier]   |   rows 5-12
        |               |
        |    islands    |
        |       ·       |
        |       ·       |
        |               |
        | ┌───────────┐ |   ← the 208 px viewport, climbing
        | │  ✈ player │ |
        | └───────────┘ |
        +---------------+
         world y = 1600   ← south, camera starts here
```

## Running it

```bash
pio run -e native -t exec     # SDL2 desktop
pio run -e esp32dev -t upload # ESP32 + ST7789 240x240
```

| Action | Desktop | ESP32 |
| --- | --- | --- |
| Fly | Arrow keys | D-pad, GPIO 32/27/33/14 |
| Gun (hold) | `SPACE` | A, GPIO 13 |
| Restart, once the run has ended | `RETURN` | B, GPIO 12 |

Movement is eight-way and **not** normalised on the diagonal — the machine this
imitates moves the same pixels per axis whether one direction is held or two,
and correcting it to a true `1/√2` makes the aircraft feel sluggish diagonally.

## What it demonstrates

| Engine capability | Where |
| --- | --- |
| A camera driven per frame, not pinned or following | `MidwayScene::updateScroll()` |
| `Camera2D` bounds — and what happens without them | `MidwayScene::init()` |
| `ObjectPool<T, N>` for bullets, enemies, explosions | `MidwayScene` members |
| Pool iteration via `nextLive()` / `kEnd` | every `MidwayScene::update*()` |
| `StaticTilemapLayerCache` **missing every frame**, measured | `MidwayScene::draw()` |
| Exported 4bpp tilemap and sprites, flash-resident | `assets/OceanTileMap.cpp` |
| Dual palette mode — sea and aircraft, 16 slots each | `MidwayScene::init()` |
| Offset bypass for a non-scrolling HUD strip | `MidwayScene::drawHud()` |
| Sprite-vs-sprite AABB with no physics engine | `MidwayScene::resolveCollisions()` |
| Press-edge input (`isButtonPressed`) for restart | `MidwayScene::update()` |

The asset shapes follow **[metroidvania](../metroidvania/)** and
**[legend_of_clone](../legend_of_clone/)**: tileset pools and map indices in
flash behind `PIXELROOT32_SCENE_FLASH_ATTR`, drawn through
`StaticTilemapLayerCache`, every header gated on
`PIXELROOT32_ENABLE_4BPP_SPRITES`.

## Screen layout

```
 0 ┌─────────────────────────┐
   │                         │
   │   playfield  240x208    │  ← the camera viewport
   │                         │
208├─────────────────────────┤
   │  SCORE 000000           │  ← HUD, 240x32, screen space
   │  PLANES 3               │
240└─────────────────────────┘
```

The playfield is anchored at screen `y = 0` with the HUD **below** it, so world
`y` maps straight to screen `y` and the camera offset is the only transform in
play. A HUD on top would push a constant offset into every world coordinate and
every hit test in the example.

## The frame budget

This is why the example exists.

### The floor nobody can move

The ESP32 driver pushes the **entire** framebuffer on every present —
`sendBufferScaled()`, called unconditionally from `TFT_eSPI_Drawer.cpp:120`.
There is no dirty-rectangle path at the driver level; `DirtyGrid` marks cells
for selective framebuffer *clears*, not for selective transmission.

So every frame, regardless of what changed:

```
240 × 240 px × 2 bytes (RGB565) = 115,200 bytes
115,200 × 8 bits ÷ 40 MHz       = 23.0 ms
```

**23.0 ms of wire time, every frame, before the game has done anything.** That
is a hard ~43 FPS ceiling. 60 FPS is not slow here, it is *arithmetically
impossible*: 16.6 ms is less than the push alone.

That is why `SPI_FREQUENCY` is 40 MHz and this example targets 30 FPS. Raising
the clock to 80 MHz halves the floor — many ST7789 panels manage it, and the
ones this was developed against do not.

### The trap: lowering the logical resolution does not help the floor

`RES_160x160` and friends shrink the **logical** framebuffer, so the clear and
every blit get cheaper. They do **not** reduce the DMA bytes, because the push
happens at *physical* resolution. The ~30% quoted in
[resolution-scaling.md](../../docs/architecture/resolution-scaling.md) is
CPU-side work, not wire time. Worth doing; it will not buy you 60 FPS.

### What the scroll actually costs

Here is the counter-intuitive part. A moving camera means
`StaticTilemapLayerCache` misses **100% of frames** — it replays its snapshot
only while the sampled camera position is unchanged, and here it changes every
frame.

That sounds expensive. It is not what dominates. The scroll costs **CPU redraw
time and zero wire time**, and the wire is already 55% of the frame. The cache
is left wired up in `MidwayScene::draw()` — the same three calls metroidvania
makes — specifically so this can be measured rather than argued about.

### Measuring it yourself

Uncomment the three profiling lines in the `esp32dev` environment of
[`platformio.ini`](platformio.ini), then:

```bash
pio device monitor -e esp32dev
```

Once a second, from `TFT_eSPI_Drawer.cpp:520`:

```
[TFT sendBufferScaled avg/N fr] total ... | setup ... | scale ... | dmaWait ... | pushDMA ... | endWrite ... | N FPS
```

That `total` splits the frame in two — transmit versus everything else — and
tells you which half to attack.

They ship **commented out** on the device and enabled on native, because the
instrumentation is not free: `PIXELROOT32_ENABLE_PROFILING` timestamps inside
the DMA loop and the debug overlay renders text every frame. On a target whose
budget is already 55% spent, measuring the frame changes the frame.

### One measured data point

| | |
| --- | --- |
| Board | esp32dev + ST7789 240×240, SPI 40 MHz |
| Tiles | 8×8, 30×200 map |
| Instrumentation | overlay + profiler **on** |
| **Result** | **~24 FPS average** (41.7 ms/frame) |

Of those 41.7 ms, 23.0 ms is the computed wire floor; the remaining ~18.7 ms is
clear, terrain, sprites, logic — **and the instrumentation measuring it**.

The tiles have since been doubled to 16×16 (below) and the flags turned off.
**That combination has not been re-measured.** When it is, this table gets a
second row rather than an edited first one.

## Why the tiles are 16×16

The map was 8×8 first, to match the machine being imitated, and **8×8 looked
better** — a finer grid draws a coastline that does not read as a staircase. It
was given up for frame rate, and the reason is worth recording because it is
not about pixels at all:

> The renderer pays a per-tile cost on every tile — a call into
> `drawSpriteInternal`, an index fetch, bounds arithmetic — and that cost does
> not shrink when the tile does.

| | 8×8 | 16×16 |
| --- | --- | --- |
| Map | 30 × 200 | 15 × 100 |
| World | 240 × 1600 px | **240 × 1600 px** |
| Blits per frame | 900 | **225** |
| Pixels per frame | 57,600 | 57,600 |
| Tileset + map in flash | 6,256 B | **2,524 B** |

The world is deliberately identical. `kCameraStartY`, the stage length and all
twelve wave triggers are in **world pixels**, so halving the grid while doubling
the tile left every one of them untouched.

What it cost: an island is now a 5-tile blob rather than a 10-tile ellipse, and
the foam shoreline ring went from 8 to 16 px.

Two things that turned out **not** to be the problem, checked before making the
change: the 4bpp tilemap path already caches its palette LUT across tiles
(`Renderer.cpp:1005-1062`, rebuilt only when the palette pointer changes), and
`packRgb565ToTftSprite8` is an inline bit-shuffle.

## Scrolling, and the two coordinate spaces

The camera counts **down** from `kCameraStartY` (1392) to 0 at 32 px/s — 43.5
seconds of stage. Row 0 is north, so the player meets the highest row numbers
first and the carrier last.

### Camera2D silently pins to the origin without bounds

The single worst trap in this example, and it cost a debugging session:

```cpp
camera_.setBounds(math::toScalar(0), math::toScalar(0));
camera_.setVerticalBounds(math::toScalar(0), math::toScalar(kCameraStartY));
camera_.setPosition(...);   // ← clamped by the two lines above
```

`Camera2D` constructs with `minX/maxX/minY/maxY` all **zero**
(`Camera2D.cpp:15-22`) and `setPosition()` clamps against them with no
diagnostic (`Camera2D.cpp:34-41`). A camera never given bounds is pinned to the
origin no matter what it is told.

It fails with no error: the world renders row 0 forever while the game logic
runs correctly somewhere off screen. The symptom reads as *"every sprite in my
game vanished"*, which points nowhere near the camera.

`legend_of_clone` and `metroidvania` never hit this because
`RoomGraph::enterRoom()` sets the bounds for them, once per room. There is no
room graph in a scrolling shooter.

### The player lives in screen space

The world scrolls underneath a shmup's player; the player does not travel
through it. So `PlayerActor` stores a **screen** position and converts to world
only at draw and hit-test time.

Storing a world position would mean adding the scroll delta back every frame
just to stand still — and any frame that missed the addition would drag the
aircraft off the bottom of the screen. Screen space makes standing still the
default and costs one addition at draw time.

### Sub-pixel motion without floats

Speeds are px/s, frames arrive in ms, so a frame is worth a fraction of a pixel.
Dropping that fraction every frame makes everything measurably slower than its
stated speed — at 30 FPS the scroll would lose about 3% of its travel.
`advancePixels()` carries the remainder in an integer, which stays exact on the
non-FPU targets the engine also builds for.

It is correct for negative rates too: C++ integer division truncates toward
zero, so the remainder keeps its sign and the climbing camera loses nothing.

## Waves are keyed to the camera, not to a clock

```c
const Wave kWaves[] = {
    { 1300, 3, 0,  6,   0, 26 },   // trigger camera y, count, weaves, col, xstep, gap
    { 1200, 3, 1, 14,   0, 26 },
    ...
```

A wave fires when the camera has climbed past its trigger. That way a formation
always arrives over the same stretch of water, which is what makes a stage
learnable. A timer would decouple the two and the same wave would meet the
player somewhere different every run.

An enemy's on-screen speed is its world speed **plus the scroll**: 60 + 32 = 92
px/s. Tuning that number while forgetting the scroll is added to it is the
easiest way to end up with enemies that flash past unhittably.

## Pools, not entities

The player is an `Entity` and goes through `Scene::draw`. Bullets, enemies and
explosions do not — they live in `ObjectPool`s the scene owns and draws
directly.

`Scene::draw` sorts its entity list and viewport-culls each member every frame.
That is worth paying for a handful of long-lived objects and not for thirty
projectiles already known to be on screen.

Pools are reset from `init()` **after** `Scene::init()` has run, which is the
ordering the [`ObjectPool`](../../include/gameplay/ObjectPool.h) header
requires: `resetState()` must clear the entity list before any pooled object is
destructed.

Sizes are derived, not guessed. A player bullet crosses the 208 px playfield in
208/300 s = 693 ms and one leaves every 170 ms, so at most 5 are ever in flight;
the pool holds 8.

## Collision without a physics engine

The example builds with `PIXELROOT32_ENABLE_PHYSICS=0`. Aircraft do not fall,
do not push each other and never touch terrain — every collision is one AABB
overlap between two sprites.

That uses a local integer `Box`, not `core::Rect`. `Rect` carries `Scalar`
coordinates every test would have to convert through, and its `intersects()`
compares with `<` rather than `<=`, so two boxes that merely touch along an edge
are reported as overlapping.

The player's hitbox is inset 4 px per side from its sprite. 16×16 of P-38 is
mostly wing, and a shmup that kills you for a wingtip graze feels broken rather
than hard. Every arcade game of this kind cheats here.

## The asset pipeline

Everything in `src/assets/` is **generated**. The art source and the exporter
live outside the repository, in `docs/audits/_dev_tools/midway_clone/`, which is
gitignored — the same split `legend_of_clone` and `metroidvania` use, where the
output is what the repo carries and what reviewers read.

```bash
python generate_assets.py             # after editing art_source.py
python generate_assets.py --check     # validate + round-trip, write nothing
python generate_assets.py --preview   # print the art back as ASCII
```

A tool exists rather than hand-written headers because the 4bpp format puts the
**left** pixel in the **low** nibble (`Renderer.cpp:605-606`) while sprite rows
are declared `uint16_t`. On a little-endian target the hex digits of a literal
run in the opposite order to the pixels they draw: `0x2220` is the pixel run
`0,2,2,2`. Hand-authoring that is not difficult, merely impossible to do
reliably. Every grid is packed, unpacked again and diffed against its source
before anything is written.

### Two things the art had to work around

**`.` means transparent, so sand is `a`.** `legend_of_clone` spells sand `.` in
its tilemap art; here `.` is the hole in every grid. A symbol meaning *nothing*
in one file and *beach* in another is a trap worth avoiding.

**The carrier deck is wood, not steel.** It was gray `#BCBCBC` first — exactly
the player's bare-aluminium airframe — and the aircraft vanished the moment it
crossed one. Wartime flight decks really were planked, so the colour that reads
is also the one that is correct.

## Build flags

| Flag | Required | Why |
| --- | --- | --- |
| `PIXELROOT32_ENABLE_4BPP_SPRITES=1` | **Yes** | The engine gates 4bpp draw paths with `if constexpr`. Without it this builds clean and renders **nothing** — `Scenes.h` `#error`s instead. |
| `PIXELROOT32_ENABLE_GAMEPLAY_OBJECT_POOL=1` | **Yes** | `ObjectPool.h` is wrapped in this and defaults to **0** (`PlatformDefaults.h:101-103`). Without it the whole `pixelroot32::gameplay` namespace is absent, and the error points at the namespace rather than at the missing flag. |
| `PIXELROOT32_ENABLE_GAMEPLAY_ROOM=0` | — | No rooms to enter. |
| `PIXELROOT32_ENABLE_PHYSICS=0` | — | No bodies to integrate. |
| `PIXELROOT32_ENABLE_PROFILING` | No | See [Measuring it yourself](#measuring-it-yourself). |

## Memory

| | Flash | SRAM |
| --- | --- | --- |
| Tileset, 8 tiles × 16×16 4bpp | 1,024 B | 0 |
| Map indices, 15 × 100 | 1,500 B | 0 |
| Sprites, 11 frames | 1,216 B | 0 |
| **Assets total** | **3,740 B** | **0 B** |
| Object pools (bullets, enemies, explosions) | — | ~780 B |
| `StaticTilemapLayerCache` snapshot | — | **57,600 B** |

That last row deserves a hard look. `allocateForRenderer()` reserves a full
logical framebuffer — 240 × 240 bytes — and in this example the cache **never
hits**, because the camera moves every frame. It is 57.6 KB of SRAM buying one
`memcpy` on the stage-complete freeze.

It is kept because measuring the miss is the point of the example. **A shipping
game with a continuously scrolling camera should call
`setFramebufferCacheEnabled(false)` and skip the allocation entirely.**

## Not in this iteration

- No audio. `PIXELROOT32_ENABLE_AUDIO=1` and a backend is wired up; nothing
  plays yet.
- One stage, one enemy type. No boss, no POW power-ups, no loop-the-loop, no
  refuelling — all of which the original has.
- Enemies do not react to the player. They descend, they weave, they shoot on a
  timer.
- Terrain is decoration. `TILE_IS_LAND` is exported for ground targets that do
  not exist yet; nothing collides with the map.
- The stage ends by holding on the last frame. No score tally, no next stage.
