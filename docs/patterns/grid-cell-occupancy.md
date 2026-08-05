# Pattern: Cell Occupancy ("Can I Enter This Cell?")

> **Zero new engine code.** This page composes two primitives that already
> ship — `pixelroot32::gameplay::GridSpace` (`include/gameplay/GridSpace.h`)
> and `pixelroot32::physics::TileAttributes` (`include/physics/TileAttributes.h`)
> — into the answer to a question every grid-based game eventually asks:
> given a cell (or a world position), is it safe to move there?

Neither primitive answers this question on its own:

- `GridSpace` converts between cell indices and world positions
  (`cellToWorld*` / `worldToCell*`) and bounds-checks a cell index against a
  grid's declared extent (`containsCell`). It has no concept of what is
  *in* a cell.
- `TileAttributes` looks up per-tile behavior flags (`getTileFlags`) and
  interprets them (`isSolidTile`, `isOneWayTile`, `isSensorTile`). It has no
  concept of a game's own grid origin or extent — it operates on tile
  indices directly.

Composing them is the whole recipe.

## Requirements

`GridSpace.h` is gated behind `PIXELROOT32_ENABLE_GAMEPLAY_GRID_SPACE`
(default `0`) — see
[Memory system — gameplay flags and byte budgets](../architecture/memory-system.md)
for its byte cost (0 B SRAM for a `constexpr` grid). `TileAttributes.h` has
no gating flag; it is always available.

## The composition

```cpp
#include "gameplay/GridSpace.h"
#include "physics/TileAttributes.h"

using pixelroot32::gameplay::GridSpec;
using pixelroot32::gameplay::containsCell;
using pixelroot32::gameplay::worldToCellX;
using pixelroot32::gameplay::worldToCellY;
using pixelroot32::physics::TileBehaviorLayer;
using pixelroot32::physics::TileFlags;
using pixelroot32::physics::getTileFlags;
using pixelroot32::physics::isSolidTile;

/// True if (cellX, cellY) is inside the grid's declared extent AND the tile
/// there is not solid.
bool isCellWalkable(int cellX, int cellY,
                     const GridSpec& grid,
                     const TileBehaviorLayer& layer) {
    if (!containsCell(cellX, cellY, grid)) {
        return false;
    }
    const uint8_t flags = getTileFlags(layer, cellX, cellY);
    return !isSolidTile(static_cast<TileFlags>(flags));
}

/// Same question, asked from a world-space position (e.g. where an actor
/// wants to move next) instead of an already-known cell index. Works for
/// both target coordinate systems: pass ints for a raw pixel position, or
/// a math::Scalar for an Actor::position-style value (the Scalar overloads
/// of worldToCellX/Y floor first, then reuse the same int primitive).
template <typename Coord>
bool isPositionWalkable(Coord worldX, Coord worldY,
                         const GridSpec& grid,
                         const TileBehaviorLayer& layer) {
    return isCellWalkable(worldToCellX(worldX, grid),
                           worldToCellY(worldY, grid),
                           grid, layer);
}
```

## Why the bounds check comes first

`getTileFlags` already returns `TILE_NONE` (0, i.e. not solid) for an
out-of-bounds `(x, y)` — see its doc comment in `TileAttributes.h`. That
alone is not enough to say "walkable": it means *no attribute data exists
there*, not *nothing is there*. A cell past the edge of a level is absent
from the tile data (so `getTileFlags` reports it as clear) but is still not
a place gameplay should let anything stand. `containsCell` catches that case
against the *game's own declared grid extent* — which is not required to be
the same as the tile layer's `width`/`height`. A game may want a smaller
walkable area than its tile data covers (a UI band reserved at the top of
the screen, for example — see the note below), and `GridSpec`'s `cols`/`rows`
is where that narrower extent lives.

## Worked example: a UI band narrower than the tile data

Suppose a game reserves the top two rows of the screen for a score display
and does not want gameplay to ever place anything there, even though the
underlying tile layer has attribute data for every row:

```cpp
// The tile layer covers the full screen height...
TileBehaviorLayer levelLayer{levelData, /*width=*/20, /*height=*/24};

// ...but the game's own grid only considers rows below the UI band walkable.
inline constexpr GridSpec kPlayGrid{0, /*originY=*/2 * CELL_SIZE,
                                     CELL_SIZE, CELL_SIZE,
                                     /*cols=*/20, /*rows=*/22};
static_assert(gameplay::gridSpecIsValid(kPlayGrid),
              "kPlayGrid exceeds Scalar's range or has an invalid cell size.");
```

`containsCell(cellX, cellY, kPlayGrid)` now rejects the UI rows regardless of
what `levelLayer` says about them, without `TileAttributes` needing any
concept of "UI band" at all — the two primitives stay decoupled, and the
game-specific rule (which rows are playable) lives in the grid declaration,
not in a third helper.

## What this pattern deliberately does not ship

No `isCellWalkable` / `isPositionWalkable` function exists in the engine
itself. Whether "walkable" means `!isSolidTile(...)`, or also excludes
`isOneWayTile`/`isSensorTile` results, or folds in a per-game rule (an
occupied-by-another-actor check, say) is a decision specific to each game's
movement rules — exactly the same reasoning that keeps `snapToGrid` out of
`GridSpace` itself. Copy the composition above and adapt the flag check to
what your game actually needs.
