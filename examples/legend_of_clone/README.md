# The Legend of Clone — screen transitions


> **⚠️ Demonstration example** — This project is provided **as an example** to showcase the capabilities of the PixelRoot32 Game Engine and what you can build with it. It may **not be 100% functional or finished**; some features can be incomplete, experimental, or work in progress.


An 8-bit-style overworld and the dungeon under it. Two scenes, four rooms each,
and one player who walks between them.

The name is the honest label. This is a **clone** of The Legend of Zelda, built to exercise engine
features — room graphs, scrolling screen transitions, scene fades, 4bpp
tilemaps — against a layout everyone already knows, so the machinery is the
thing under review and not the level design. It is not a reproduction: the maps
are hand-authored rather than ripped, the hero carries nothing and is nobody in
particular, and there are no enemies, items or combat.

It started as an answer to one question — *does a player walking off the edge
of one screen land correctly on the next?* — and the dungeon is the same
question asked again somewhere the answer had better not be different.

```
        OVERWORLD                        DUNGEON
+----------------+----------------+   +----------------+----------------+
| 0  north-west  | 1  north-east  |   | 0  north-west  | 1  north-east  |
+----------------+----------------+   +----------------+----------------+
| 2  START  [C]  | 3  south-east  |   | 2  ENTRANCE [S]| 3  south-east  |
+----------------+----------------+   +----------------+----------------+

           [C] cave mouth  <----------------->  [S] stairs
```

## Running it

```bash
pio run -e native -t exec     # SDL2 desktop
pio run -e esp32dev -t upload # ESP32 + ST7789 240x240
```

Arrow keys walk. Walk into a gap in the border and the screen scrolls. Walk
into the black cave mouth on the start screen and the picture fades into the
dungeon; walk onto the staircase you arrive beside and it fades back out,
putting you below the cave rather than at the start of the game.

## What it demonstrates

| Engine capability | Where |
| --- | --- |
| `RoomGraph<N>` + `buildRoomGraph()` | `TopDownScene::init()` |
| Exported room-layer data contract | `assets/OverworldRooms.h` |
| Camera pinned per room, not following | `TopDownScene::snapCameraToRoom()` |
| `onEnter` hook bridged to `Scene::onRoomEnter` | `TopDownScene::onRoomEnterCallback()` |
| `Engine::triggerTransition()` fade between scenes | `OverworldScene::onPlayerSettled()` |
| Exported 4bpp tilemaps and sprites, flash-resident | `assets/OverworldTileMap.cpp` |
| Dual palette mode — world and player, 16 slots each | `TopDownScene::init()` |
| `StaticTilemapLayerCache` on a pinned camera | `TopDownScene::draw()` |
| Sprite flipping as animation, NES-style | `PlayerActor::draw()` |
| Offset bypass for a non-scrolling HUD strip | `drawStatusBar()` |

The asset shapes here follow **[metroidvania](../metroidvania/)**, which is the
reference for how this engine expects tilemaps and sprites to be fed to it:
tileset pools and map indices in flash behind `PIXELROOT32_SCENE_FLASH_ATTR`,
read through `PIXELROOT32_READ_BYTE_P`, drawn through `StaticTilemapLayerCache`,
with every header gated on `PIXELROOT32_ENABLE_4BPP_SPRITES`.

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

## Two scenes, one set of machinery

The overworld and the dungeon are separate `Scene`s. What they have in common —
a room grid, a camera pinned per room, the scrolling change between rooms,
collision, the player — lives in `TopDownScene`, and each concrete scene is
about eighty lines: its map, its status readout, and the one tile that changes
scene.

That split is not tidiness. The room transition below contains one step that is
easy to leave out and impossible to spot afterwards, and a second copy of it in
the dungeon would have been a second chance to get it wrong.

`TopDownScene::Setup` is what a scene hands over: two palettes (world and
player), a room layer, a `TileWorld` already attached to its export, and where
to put the player. Two hooks are optional — `drawStatusBar()` and
`onPlayerSettled()`, the latter being where a scene reacts to the tile the
player is standing on.

`onPlayerSettled()` is deliberately not called mid-slide. During a room change
the player is being interpolated across a seam and passes over tiles they never
stepped on; firing a cave entrance from one of those would be a teleport nobody
asked for.

### init() runs again every time

`SceneManager::setCurrentScene()` calls `init()` on the scene it swaps to. Every
time. A scene you have already visited is re-initialised from scratch when you
come back to it, and two things in this example exist only because of that:

**The spawn is remembered.** `OverworldScene` records where the player
should reappear *before* it starts the fade into the dungeon. Without it,
climbing the stairs back out would drop you at the start of the game.

**The room graph is reset.** `buildRoomGraph()` **appends** — that is how
several exported layers are stitched into one graph — so a second call on a
graph that is already full adds nothing and returns 0. The scene therefore does
`rooms_ = RoomGraph<kMaxRooms>{}` before every build.

**The tilemap snapshot is dropped.** A framebuffer cached under the previous
scene's palette and tiles is worthless to this one, so `init()` calls `clear()`
and `invalidate()` before taking the buffer back with `allocateForRenderer()`.
Allocating at init rather than in `draw()` is what keeps the game loop off the
heap.

That second one shipped broken: leaving the dungeon rebuilt no rooms,
`worldReady_` went false, and the scene stopped drawing. It presented as a black
screen rather than as the "MAP DATA REJECTED" message that was supposed to catch
exactly this, because the message was being drawn in **world space** — with the
dungeon camera's leftover offset still in the renderer, it landed sixty pixels
above the top of the screen. The check was right and its output was invisible.
Failure paths draw in screen space now.

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
   The player slides rather than teleports because an 8-bit top-down walks you the last
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

## The asset pipeline

`src/assets/` is **generated code**, in the shape the PixelRoot32 Tilemap Editor
and Sprite Compiler produce. The runtime is written against that shape rather
than against anything this example invented — which is the point of the whole
folder.

```
character art  ──exporter──►  src/assets/*.h|.cpp   packed 4bpp in flash
(not versioned)               (versioned, what builds)
```

**Only the right-hand side is in the repository.** The exporter and its
character-art source are development tooling and live outside the tree, under
the gitignored `docs/audits/_dev_tools/legend_of_clone/`, next to the other
asset scripts. That is the same arrangement metroidvania has: its files say
`// Generated by PixelRoot32 Sprite Compiler` and the compiler is not in the
repository either. Cloning this repo gets you an example that builds; it does
not get you the art pipeline, and it does not need to.

So treat `src/assets/*` as build output. The header on every file says as much,
and a hand edit there is lost on the next export.

### What the exporter is, for the record

Art is authored as **characters, one per pixel** — `art_source.py` — rather than
as hex. A bush you can see in a diff is worth more during review than 128 bytes
of hex, and eight rooms of level design are only reviewable as a grid. But the
authoring format **stops at the exporter**: what ships is packed bytes in flash,
identical to an editor export, with no packer, no `.bss` and no startup cost.

That distinction is the correction this example needed. An earlier version
shipped the character art *as the runtime format* and unpacked it into RAM at
`init()`. Readable, and wrong: it spent 4,864 bytes of RAM on a convenience that
belongs at build time.

A second script, `check_maps.py`, validates both maps against their room tables
and the doorway constants — a border declaring a connection the tiles wall off,
an edge opening the room graph knows nothing about, a spawn cell sitting on the
very tile that triggers a scene change. Each of those produces a game that runs
and misbehaves, which no compiler will catch.

## Map data

Both maps are authored as **22 rows of 30 characters**, one per tile, and
exported to a `TERRAIN_INDICES` array. The legend:

```
OVERWORLD                        DUNGEON
'.'  sand         walkable       '.'  floor    walkable
','  grass patch  walkable       '#'  wall     blocking
'B'  bush         blocking       'S'  stairs   walkable, leaves the dungeon
'T'  forest       blocking
'#'  mountain     blocking
'C'  cave mouth   walkable, enters the dungeon
```

Alongside the indices, each tileset exports a `TILE_SOLID` table. Collision is a
property of the **tile**, not of a second layer — a bush blocks wherever it is —
so the two come out of one source and cannot drift apart. `TileWorld` is what
pairs them at runtime: three pointers, no data of its own.

> This is where this example diverges from metroidvania, which derives collision
> from a separate `platforms` layer. That is the right answer *there*, because a
> platform tile and a background tile can share art. Here tile type **is**
> collision, so a 7-byte table beats a 660-byte layer.

The cave mouth and the stairs are **walkable**. They have to be — you enter a
cave in a game like this by walking into it, not by bumping into it — which is why both are
placed inside a room rather than in its border. A walkable cell in a border
would be an opening the room graph knows nothing about.

The overworld screens are hand-authored to read like the start area of the NES
first quest — mountains along the north, forest walling in the south, the cave
in a rock face. **They are not tile-exact rips of the original ROM.** Dungeon
rooms are a 2-tile-thick wall around an 11x7 interior, which is the NES dungeon
room proportioned to a 15-wide screen. Doorways are two tiles wide so the
16-pixel player walks through without being tile-aligned; the original gets away
with one-tile doors only because it nudges you onto the grid as you pass.

Room seams have to line up by hand: the open cells on one side of a border must
face open cells on the other, or the connection is decorative and the player
walks into a wall. The seams are marked in each map's comments.

## Colors

A `Color` in this engine is a **palette slot, not an RGB value** — the same
enumerator resolves differently under each palette. So the NES look is not a
set of tinted draws; it is a custom 16-entry RGB565 table installed at `init()`,
after which `Color::Yellow` *is* sand.

The scene runs in **dual palette mode**, which is the metroidvania arrangement:

```cpp
enableDualPaletteMode(true);
setBackgroundCustomPalette(TILEMAP_PALETTE_DATA);      // the world
setSpriteCustomPalette(PLAYER_SPRITE_PALETTE_RGB565);  // the hero
```

Two tables of sixteen instead of one. The previous version shared a single
table, which meant three of the world's slots were spent on the hero's skin, tunic
and brown. They are no longer competing.

**The dungeon shares the world's table.** Not a compromise — it already carried
a navy, a blue and a grey, so blue walls and stone stairs needed no new entries.
A dungeon that *did* want its own would pass a different pointer;
`TopDownScene::Setup` takes one per scene precisely so that stays a one-line
change.

### Two indices, and why they are not the same

Each palette is exported as **two** arrays, and conflating them is the mistake
the split exists to prevent:

| | Indexed by | What it holds |
| --- | --- | --- |
| `..._PALETTE_MAPPING` | 4bpp **pixel value** | a `Color` slot |
| `..._PALETTE_DATA` | `Color` **slot** | RGB565 |

The blitter treats pixel value 0 as transparent and never reads entry 0, so
black — which every outline, the cave mouth and the whole dungeon floor need —
cannot live at pixel value 0. It lives at 1 and maps to `Color::Black`.

The mapping is therefore **not** the identity, and it must not be forced to be.
Metroidvania's happens to be identity because nothing in its art collides with
that rule; compacting this one the same way put `Color::White` at black and
would have drawn the status bar text invisibly. Colors keep their conventional
slots, so anything that names a color instead of indexing art — status text,
the `MAP DATA REJECTED` message — still means what it says.

## Art

Every 16x16 image is authored as **16 strings of 16 characters** and exported to
packed 4bpp. The characters mean:

```
'k' black   'g' green        'o' rock, lit
'w' white   'l' tunic green  'r' rock, shaded
'n' shadow  '.' sand         's' skin
'd' dark green               'h' hero brown
' ' transparent
```

Nibble order matters and is easy to get wrong: within a byte the **low** nibble
is the left pixel of the pair. Reversing it mirrors every pair of pixels, which
looks almost right — harder to spot than art that looks broken. The exporter
owns that detail now, so it is decided once rather than per asset.

Adding a color costs nothing until art references it: the exporter builds each
palette from the characters that group actually uses.

## The player

The hero is **three colors and a hole** — tunic green, skin, and one brown doing
outline, hair, boots and belt at once. That is one NES sprite palette exactly,
which was the whole budget an 8-bit cartridge had for a protagonist.

He has **no black in him**. Outlining a sprite in black is the fastest way to
make it stop reading as an 8-bit character; it flattens into a sticker.

**He carries nothing in either hand,** and that is a design constraint rather
than an omission. It keeps clear daylight between this hero and the obvious
inspiration — and it pays for itself a second time in the walk cycle below.

### The walk cycle

**Two frames, always** — a toggle, not a sequence. `PlayerActor::walkFrame_` is
one bit for that reason.

**A frame lasts 100 ms**, which is 6 frames at ~60 Hz — the cadence 8-bit
top-down walks are built on. `kWalkFrameMs`.

**The cycle is keyed on input, not on movement.** Pressed against a wall the
hero keeps walking on the spot, and releasing the D-pad freezes him on whatever
frame he was on rather than snapping to a neutral pose. Gating on *movement*
instead would make him stutter to a halt against every bush.

**Which frames mirror, measured rather than assumed:**

| Direction | Frame 0 | Frame 1 |
| --- | --- | --- |
| South | `PLAYER_DOWN` | `PLAYER_DOWN` mirrored |
| North | `PLAYER_UP` | `PLAYER_UP` mirrored |
| East | `PLAYER_SIDE_A` | `PLAYER_SIDE_B` — a real bitmap |
| West | east, mirrored | |

Four bitmaps, not five, and the empty hands are why. A held object cannot be
mirrored — flipping the sprite would teleport it to the other hand — so a
character carrying a shield needs a second front-facing bitmap. This one does
not: the head and torso are left-right symmetric while the arms and legs are
not, so the mirror swings the far arm forward and the near one back, which *is*
the animation. The second front-facing bitmap measured **0 differing pixels**
against `mirror(PLAYER_DOWN)`; dropping it saved 168 bytes of flash.

The side pair cannot play the same trick, because there the flip bit is already
spending itself on the facing. Mirroring a side-on pose turns him around instead
of animating him — 146 differing pixels, against the front pair's 0.

One trap worth knowing, because it has already cost an afternoon here: a sprite
that is **perfectly** symmetric makes its own mirrored walk frame invisible, and
that reads as a broken timer rather than as broken art. `check_sprites.py`
measures every bitmap against its own mirror and fails below a floor:

| Bitmap | Differing pixels vs its own mirror |
| --- | --- |
| `PLAYER_DOWN` | 44 |
| `PLAYER_UP` | 26 |
| `PLAYER_SIDE_A` | 146 |
| `PLAYER_SIDE_B` | 142 |

The two vertical numbers are the ones that matter, because those are the only
frames whose animation *is* the mirror. An earlier `PLAYER_UP` sat at four
pixels — enough to pass, not enough to see.

Note that the flip bit ends up doing two different jobs: facing north it
carries the animation *frame*, facing east or west it carries the *facing*.
`PlayerActor::draw()` has that same split.

Five bitmaps cover four directions and a two-frame cycle.

## Drawing the terrain

The terrain goes through `StaticTilemapLayerCache`, the same path metroidvania
uses. A pinned camera is the case that cache exists for:

```cpp
const TileMap4bppDrawSpec staticLayers[] = { terrainLayer() };
tilemapLayerCache_.draw(renderer, camX, camY, staticLayers, 1, nullptr, 0);
```

While the player walks around a room the camera does not move, so the terrain is
byte-identical frame after frame and the cache replays it with one memcpy
instead of blitting 165 tiles. During a slide the camera sample changes every
frame and it rebuilds — which is exactly when a rebuild is correct. Nothing had
to be told when to invalidate.

One detail is not obvious and is worth copying. The cache keys its snapshot on a
camera sample, and its documentation suggests `-renderer.getXOffset()`. That
does not work here: the offset is only set inside `draw()`, so
`adviseFramebufferBeforeBeginFrame` — which the engine runs *before*
`beginFrame` — would read the previous frame's value and disagree with `draw()`
on the exact frame the camera moves. Both callers read `Camera2D` directly
instead, so they always agree.

## The build flag

```ini
-D PIXELROOT32_ENABLE_4BPP_SPRITES=1
```

The engine gates its 4bpp paths with `if constexpr`, so building without this is
**not a compile error — it is a black screen with no diagnostic.** Every header
here is gated on it the way metroidvania's are, and `Scenes.h` turns the missing
flag into an `#error` that says so. Failing at build time is the whole point:
this example cannot function without it, so it should not pretend to build.

## Memory

Almost everything is `const` in the scene flash section
(`PIXELROOT32_SCENE_FLASH_ATTR`) and read through `PIXELROOT32_READ_BYTE_P`, so
it costs flash rather than RAM:

| Item | Bytes | Where |
| --- | ---: | --- |
| Tileset pixel data | 1,408 | flash — 11 tiles x 128 B across both maps |
| Player pixel data | 640 | flash — 5 sprites x 128 B |
| Map indices | 1,320 | flash — 660 per map |
| Collision tables | 11 | flash — one `bool` per tile, not per cell |
| `TileWorld` x2 | ~40 | RAM — three pointers each, no data |
| `RoomGraph<4>` x2 | ~240 | RAM — fixed capacity, no allocation |
| Framebuffer snapshot | 57,600 | heap, `allocateForRenderer()` at init, ESP32 only |

Measured on `esp32dev`, whole example: **24,800 B RAM (7.6%)** and 351,073 B
flash (26.8%).

The previous version of this example expanded character maps into `.bss` at
startup and packed art into RAM buffers. Moving to the exported format took RAM
from 29,664 B to 24,800 B — **4,864 bytes returned** — while flash stayed flat,
because the art that arrived was offset by the packer and character maps that
left.

`TileMapGeneric::indices` is a non-const `uint8_t*`, so the exporter casts the
const array on assignment, exactly as the editor's own output does. Nothing
writes through it.

## Not in this iteration

- The dungeon has no keys, no locked doors, no enemies and nothing to find. Four
  rooms and a way out.
- The status bar is a placeholder. `UISpriteRow` is what the heart row is for,
  once the player has something to lose.
- No enemies, items, sword or combat anywhere.
- No persistence — leaving the dungeon forgets everything about it.
