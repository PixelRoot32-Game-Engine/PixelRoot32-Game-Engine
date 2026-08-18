# iso_dungeon — an isometric room on a generic projection

A single dungeon chamber drawn in 2:1 isometric, with a hero that walks it
tile by tile. Two stone walls along the back edges, a doorway in each, an altar
on a ritual square, and two pillars flanking it.

The point of the example is not the dungeon. It is that **PixelRoot32 has no
isometric mode**. The view here is one `gameplay::ProjectionSpec` — six
integers — and every other system in the example is the same
projection-blind code an axis-aligned game would use.

## The whole isometric view

```cpp
inline constexpr gameplay::ProjectionSpec kTileProjection{
    120, 88,      // screen position of tile (0,0)'s diamond centre
     16,  8,      // +1 tileX -> right and down
    -16,  8};     // +1 tileY -> left  and down
```

That is it. Orthogonal, isometric 2:1, isometric 1:1, oblique and mirrored
layouts are all *values* of this one type; there is deliberately no isometric
function, enum or template parameter anywhere in the engine. Point
`kTileProjection` at `{0, 0, 16, 0, 0, 16}` and this example becomes a
top-down board game with no other edit.

## Requirements (build flags)

| Flag | Why |
|---|---|
| `PIXELROOT32_ENABLE_GAMEPLAY_PROJECTION=1` | `Projection.h`. Without it the header compiles to nothing and the room has no geometry at all. |
| `PIXELROOT32_ENABLE_DEPTH_SORT=1` | `Entity::depthKey` and `gameplay::compareByDepthKey`. Off, entities draw in insertion order and the occlusion is simply wrong half the time. |
| `PIXELROOT32_ENABLE_GAMEPLAY_GRID_SPACE=1` | `gameplay::GridMotion`. This example declares no `GridSpec` at all, but `GridMotion` shares the grid flag rather than taking one of its own — see below. |
| `PIXELROOT32_ENABLE_4BPP_SPRITES=1` | The art is 4bpp on a custom 16-colour palette. Not cosmetic: the engine gates its 4bpp draw paths with `if constexpr`, so building without it is not an error, it is a black screen. |

## What the example actually demonstrates

### Movement is projection-blind

The hero walks **exact tile to exact tile** — always either at rest on a cell
or travelling between two named cells, never anywhere else.
`gameplay::GridMotion` holds that state and `gameplay::tickStep` advances it,
on a fixed 16 ms logic clock so a slow frame stretches nothing.

`HeroActor` supplies only the two policies the engine deliberately does not
own: which cell may be entered, and where the direction comes from. The
isometric view enters in exactly **one line**:

```cpp
position = gameplay::interpolatedWorld(motion_, kStepsPerCell, kTileProjection);
```

That overload projects both endpoints through the spec and lerps between them.
Under a non-identity basis a step along one cell axis moves **both** screen
axes, which the `GridSpec` overload cannot express — and it is the whole
reason an isometric game does not reimplement cell-to-cell navigation.

### The D-pad needs no remapping

Under `kTileProjection` the cell axes project to the four screen **diagonals**:
`+cellX` goes down-right, `+cellY` down-left. So the four buttons already cover
the four directions a player can see, and the input code is byte-identical to
an orthogonal game's.

This is a property of *this* basis, not a general truth. A game with a
different basis has to decide it again, and the engine takes no position.

### Row-major iteration IS the painter's order

`RoomRenderer` draws the floor and walls in one plain `for y / for x` sweep,
with no sort. Screen depth under this spec is `8 * (x + y)`, so a tile is always
drawn after both `(x-1, y)` and `(x, y-1)` — exactly the two neighbours whose
extruded blocks can overlap it from behind.

The back walls are drawn in that same static pass rather than as sorted
entities, and that is not a shortcut: both walls hug the two back edges, so
their depth is lower than any reachable tile's. Nothing the player can stand on
is ever behind them.

### Occlusion is real, and it is the engine's depth sort

The altar and the two pillars are **entities**, on the same render layer as the
hero, for exactly one reason: the hero can reach tiles on both sides of them,
so whether a prop draws over or under the hero changes from frame to frame.

Each entity writes its own `depthKey` from its projected anchor and the scene
uses `gameplay::compareByDepthKey`. `compareByBottomY` would be **wrong** here:
it orders by world Y, which is the correct paint order only while screen depth
is a monotone function of world Y — true for an axis-aligned room, false the
moment a projection shears the grid. Cells `(2,0)` and `(0,2)` sit on the same
screen row at completely different cell-space Ys.

Verified both ways against the same altar (`depthKey` 136):

| Hero tile | `depthKey` | Result |
|---|---|---|
| `(3, 2)` | 128 < 136 | the altar draws over the hero |
| `(3, 4)` | 144 > 136 | the hero draws over the altar |

Screen Y happens to **be** the isometric depth for this spec, because
`cellToScreenY` reduces to `originY + 8 * (x + y)` when both cell axes share a
vertical component. That identity is a property of this basis, not of
projections in general — which is exactly why the engine takes the key as data
instead of deriving it.

## Decisions worth knowing about

**The tile size was not free.** 32x16 is the largest 2:1 tile that keeps a 7x7
room inside a 240 px display without scrolling: `(7 + 7) * 16 = 224`. A 48x24
tile would need 336 px. A `static_assert` also pins the basis determinant at
256 — a power of two, so `screenToCell`'s single division strength-reduces to a
shift. Change the tile size carelessly and that assert is what stops you.

**`GAMEPLAY_GRID_SPACE` is on for a capability it does not name.** `GridMotion`
shares the grid flag rather than taking a `..._GAMEPLAY_GRID_MOTION` of its
own, so an isometric game that wants cell-to-cell navigation must enable the
grid flag even though it never declares a `GridSpec`. `GridMotion.h` states
this outright. The cost is one unused header, not one unused byte.




**Standing still costs nothing.** `shouldRedrawFramebuffer()` reports whether
the hero would draw differently from the frame already on the panel, and the
engine skips both `draw()` and `present()` when it would not. That is not a CPU
optimisation: `present()` pushes 240×240 RGB565 over SPI, about 23 ms at
40 MHz, which is the frame budget. The hero is the only entity that can move,
so one flag answers for the whole room — a scene with several movers would
need to OR their answers together.

**`graphics::Color` is a palette INDEX here, and its name lies.** This example
installs a custom 16-colour palette, so `Color::Black` is index 0, which the
renderer treats as transparent, and index 1 — spelled `Color::White` — is where
the palette puts pure black. `kVoidColor` names that index so the trap lives in
exactly one place. Nothing paints it today: it is already the colour
`beginFrame()` clears to, so `RoomRenderer` skips the backdrop fill rather than
writing 57,600 identical bytes over 57,600 identical bytes. Repalette the room
and that fill comes back.

## Art

Isometric art is geometry, not draftsmanship. Every block in `src/assets/` is
the same diamond extruded downward by a different amount — a wall by 24 px, an
altar by 14, and a pillar is a narrower 16×8 diamond extruded 26 — which is why
they stack without seams.

Two rules the art keeps, both learned the hard way:

1. **No face may reuse the outline colour index.** A cube whose lit face equals
   its outline loses its silhouette and flattens into a hexagon.
2. **The floor is a checkerboard, not a flat tone.** A single tone gives the eye
   nothing to judge which cell the hero is standing on, which is the one thing a
   player of an isometric game constantly needs to know.

Every sprite ships a `*_FOOT_Y`: the bitmap row that must land on the target
cell's diamond centre. One anchoring rule (`IsoDraw.h`) for the 16px floor, the
40px wall, the 34px pillar and the 24px hero alike — the alternative, an ad-hoc
Y offset at each call site, is how isometric art quietly goes crooked.

## Controls

| Button | Cell direction | On screen |
|---|---|---|
| Up | `-cellY` | up-right |
| Down | `+cellY` | down-left |
| Left | `-cellX` | up-left |
| Right | `+cellX` | down-right |

One axis at a time, fixed priority Up > Down > Left > Right. Diagonals are
unrepresentable by construction, and a step in flight cannot be redirected —
that refusal is what makes the movement read as a board rather than a walk.

## Documentation links

- [`gameplay/Projection.h`](../../include/gameplay/Projection.h) — the projection math
- [`gameplay/GridMotion.h`](../../include/gameplay/GridMotion.h) — cell-to-cell step state
- [`gameplay/DepthCompare.h`](../../include/gameplay/DepthCompare.h) — `compareByDepthKey`
- [Memory system](../../docs/architecture/memory-system.md) — per-capability budgets

## Build

```bash
cd examples/iso_dungeon
pio run -e native        # SDL2 desktop
pio run -e esp32dev      # ST7789 240x240
```
