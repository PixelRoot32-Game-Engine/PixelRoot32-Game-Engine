# Fixtures — `test_iso_dungeon_projected_conversion`

Input data for the differential test one directory up. Seven files: the room
catalogue, the isometric constants, and the exported tilemap with its palette,
tileset and props.

## Where they came from

They were read directly out of `examples/iso_dungeon/src/` until that example
moved to
[PixelRoot32-Demo-Projects](https://github.com/PixelRoot32-Game-Engine/PixelRoot32-Demo-Projects/tree/main/graphics/iso_dungeon).
The engine's `platformio.ini` pointed `[env:native_test_gameplay]` at the
example's source directory; when the directory left, the suite stopped
compiling, and because it reports `ERRORED` rather than `FAILED` the run's
summary line still read as green. The suite owns its inputs now, so it cannot
break that way again.

## This is a frozen input, not a mirror

**Do not "sync" these files with the Demo-Projects copy.** They are not a
duplicate that has to be kept in step.

The test renders the exported tilemap through the projected `drawTileMap` path
and compares it, pixel for pixel, against a frozen copy of the hand-rolled
`drawSprite` loop the conversion replaced — an oracle that lives inside the test
itself. What is under test is the **engine's** projected path. The fixture is
only the input both sides are fed.

So the demo is free to change its art, its rooms, or its export, and this test
stays just as valid. Conversely, changing a fixture here changes the input to
both sides of the comparison, which weakens the test without failing it — so
change these files only with a reason that is about the engine.

If the engine's export *contract* changes — the shape of what the Tilemap
Editor emits — then these files need to be re-exported to match, and that is
the one case where re-copying from the demo is the right move. See
[Projected Tilemap: Producer Obligations](../../../../docs/architecture/projected-tilemap-producer-obligations.md).

## Why one file ends in `.cpp.inc`

`IsoDungeonRoomTileMap.cpp.inc` is the export's single translation unit, holding
`TILESET_SPRITES`, `TILESET_FOOT_Y`, the room index arrays and `init()`. Its
contents are the export verbatim; only the name differs, and it has to.

PlatformIO globs `.cpp` files under a test directory and compiles each as its
own translation unit. That breaks this suite in two directions at once:

- In `[env:native_test]`, `PIXELROOT32_ENABLE_TILEMAP_PROJECTION` is off, so
  `math::ProjectionSpec` does not exist and the export cannot compile at all —
  the suite errors instead of reporting its "capability disabled" placeholder.
- In `[env:native_test_gameplay]`, the test also `#include`s the file (that is
  the shipped shape it is meant to exercise), so every symbol is defined twice
  and the link fails.

The `.inc` suffix keeps it out of the glob. The test includes it, which puts it
inside the `#if PIXELROOT32_ENABLE_TILEMAP_PROJECTION` guard where it belongs.

## Assets

The tile and prop art is the demo's own, CC0. It is duplicated here as test
data.
