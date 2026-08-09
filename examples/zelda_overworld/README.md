# Zelda Overworld — screen transitions

A four-screen slice of an NES-style overworld, built to answer one question:
**does a player walking off the edge of one screen land correctly on the next?**

Everything that would obscure that answer is absent. No enemies, no items, no
caves, no combat. One player, four screens, and the scrolling transition
between them.

```
+----------------+----------------+
| 0  north-west  | 1  north-east  |
+----------------+----------------+
| 2  START       | 3  south-east  |
+----------------+----------------+
```

## Running it

```bash
pio run -e native -t exec     # SDL2 desktop
pio run -e esp32dev -t upload # ESP32 + ST7789 240x240
```

Arrow keys walk. Walk into a gap in the border and the screen scrolls.

## What it demonstrates

| Engine capability | Where |
| --- | --- |
| `RoomGraph<N>` + `buildRoomGraph()` | `ZeldaOverworldScene::init()` |
| Exported room-layer data contract | `assets/OverworldRooms.h` |
| Camera pinned per room, not following | `snapCameraToRoom()` |
| `onEnter` hook bridged to `Scene::onRoomEnter` | `onRoomEnterCallback()` |
| Multi-layer 1bpp tilemaps | `assets/OverworldMap.cpp` |
| Offset bypass for a non-scrolling HUD strip | `drawStatusBar()` |

## Screen layout

The NES splits its 256x240 output into a 256x176 playfield and a 64 px status
bar. This example keeps that split on the engine's reference 240x240 panel: a
**240x176 playfield** (15x11 tiles at 16 px) with the status bar **below** it.

The bar sits at the bottom rather than the top on purpose. With the playfield
anchored at screen y = 0, world y maps straight to screen y and the camera
offset is the only transform in play. A top bar would push a constant +64 into
every world coordinate, tilemap origin and hit test in the example.

The camera viewport is the playfield (240x176), not the panel. Because the
renderer's logical surface is still 240 px tall, world rows below the current
room would bleed into the strip — so the bar is drawn last, and opaque.

## How a screen change works

The camera never follows the player. It sits pinned at the current room's
origin, which is what makes the overworld read as a grid of fixed screens
rather than a scrolling field.

When the player's box crosses a room edge and that edge has a connection:

1. **Camera bounds widen to the union of both rooms.** `Camera2D::setPosition`
   clamps to its bounds, and those bounds are still the room being left. Skip
   this and the slide pins to the old room's edge — the transition runs, the
   timer expires, and nothing appears to move.
2. **The player is disabled.** `Scene::update` skips disabled entities, so this
   is the whole of the input lockout.
3. **Camera and player interpolate together** over `kTransitionDurationMs`.
   The player slides rather than teleports because the NES walks Link the last
   few pixels across the seam while the screen scrolls. Teleporting looks like
   a cut.
4. **`RoomGraph::enterRoom()` on arrival** resets the bounds to the new room
   and fires the `onEnter` hook.

The crossing test fires on the *leading edge* — the frame any pixel of the
player enters the neighbouring screen, not once they are halfway across.

### Why not CameraTween

`CameraTween` interpolates a camera and nothing else. This transition has to
move the player in lockstep with it, which needs a timer the scene owns
anyway — at which point the camera lerp is two lines and a second timer inside
`CameraTween` would only be state to keep in sync. If a later iteration adds a
transition that moves the camera alone, `CameraTween` is the right tool for it.

## Map data

`assets/OverworldMap.cpp` holds a **character map** — 22 rows of 30 characters,
one per world tile — and expands it at `init()` into layer index arrays and a
collision map.

```
'.'  sand        walkable
','  grass tuft  walkable
'T'  tree        blocking
'#'  mountain    blocking
'C'  cave mouth  blocking
```

One authoritative grid, several derived views. A hand-maintained collision
table next to hand-maintained tile indices is a table that drifts; deriving
both from the same characters makes drift impossible. It is also the only form
in which four screens of level design are reviewable in a diff.

The screens are hand-authored to read like the start area of the NES first
quest — mountains along the north, the cave in the rock face on the start
screen, woods to the east. **They are not tile-exact rips of the original ROM.**

Room seams have to line up by hand: the open cells on one side of a border must
face open cells on the other, or the connection is decorative and the player
walks into a wall. The seams are marked in the map's comments.

## Migrating the art to 4bpp

Iteration 1 ships 1bpp art. The migration is meant to cost no structural
change, and there are exactly two places it touches.

**1. `src/TileFormat.h`** — the type aliases and the two draw wrappers:

```cpp
using SceneSprite  = gfx::Sprite4bpp;    // was gfx::Sprite
using SceneTileMap = gfx::TileMap4bpp;   // was gfx::TileMap
```

The engine's `drawSprite`/`drawTileMap` overloads do not share a signature
across bit depths — the 1bpp pair takes a `Color`, the 4bpp pair reads a
palette out of the descriptor. The wrappers absorb that: their own signature
keeps the `color` parameter, which simply goes unused. **No call site changes.**

**2. The asset files** — `OverworldTiles.h` and `LinkSprites.h` grow palettes
and 4bpp pixel data. The tile *ids* and the character map do not change.

Two things are deliberately structured so they do not become a third step:

- **The layer list is data.** `OVERWORLD_LAYERS` is an array the scene iterates
  without asking what is in it. 1bpp needs three layers because a layer can
  only be one color; 4bpp needs one because the palette is per cell. Collapsing
  three entries into one is an edit to `OverworldMap.cpp`, not to the scene.
- **Collision is derived from tile ids, not from pixels.** `isSolidCell()`
  keeps working across any bit depth.

Then flip the build flags in `lib/platformio.ini`:

```ini
-D PIXELROOT32_ENABLE_4BPP_SPRITES=1
```

That flag is not cosmetic — it gates the 4bpp paths in `Renderer.cpp` via
`if constexpr`, and leaving it at 0 compiles them out. The symptom of
forgetting it is a **silent black screen**, not a build error.

## Why the world looks like this

A 1bpp tilemap renders in a single color: `drawTileMap` takes one `Color` for
the whole layer. Three layers is the minimum that keeps the world legible —
sand, foliage, rock — and each one is a category a game would want separately
anyway, not a workaround.

Within a layer the tiles carry their identity in their **pattern** rather than
their hue: rock is masonry hatch, trees are a round canopy on a trunk, grass is
a pair of tufts, sand is a dense dither. Those shapes survive the 4bpp
migration unchanged; only the palette arrives.

## Memory

| Item | Bytes | Notes |
| --- | ---: | --- |
| 3 layer index arrays | 1,980 | 660 cells each, RAM — `TileMapGeneric::indices` is non-const |
| Collision map | 660 | `bool` per cell |
| `RoomGraph<4>` | ~120 | Fixed capacity, no allocation |
| Tileset + player sprites | ~200 | Flash (`constexpr`) |

~2.8 KB of RAM, which fits the tightest supported variant (ESP32-C3, 400 KB
SRAM) with room to spare.

## Not in this iteration

- Caves are drawn but not enterable.
- The status bar is a placeholder. `UISpriteRow` is what the heart row is for,
  once the player has something to lose.
- No enemies, items, sword or combat.
- No persistence.
