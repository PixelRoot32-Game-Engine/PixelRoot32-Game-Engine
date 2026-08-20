# CameraBounds

<Badge type="info" text="Struct" />

**Source:** `ProjectedMapBounds.h`

## Description

Closed-interval camera-position range produced by `cameraRangeFor`.

**Deliberately NOT `ScreenBounds`.** Every field here -- `minX`, `maxX`,
`minY`, `maxY` -- is a camera position the camera may actually occupy, so
the interval is CLOSED: `[minX, maxX]`, both ends inclusive. `ScreenBounds`
is HALF-OPEN (`right`/`bottom` are one past the last covered pixel)
because it measures a run of covered pixels, not a set of legal camera
positions. Reusing `ScreenBounds` as this return type -- feeding a
`right` that means "one past the edge" into code that reads it as "the
maximum legal position" -- is an off-by-one that stays invisible until the
camera scrolls all the way to a map edge, which is exactly the kind of bug
a short play session will not surface. The two structs share a field
count and nothing else; do not conflate them.

`valid` mirrors `ScreenBounds::valid`: `false` when the source
`ScreenBounds` passed to `cameraRangeFor` was itself never seeded (see
`expandProjectedMapBounds`'s union-across-calls contract), in which case
no camera range exists and `minX`/`maxX`/`minY`/`maxY` are left at their
defaults rather than computed from garbage input.

## Properties

| Name | Type | Description |
|------|------|-------------|
| `minX` | `int` | Minimum camera X position the camera may occupy. |
| `maxX` | `int` | Maximum camera X position the camera may occupy. |
| `minY` | `int` | Minimum camera Y position the camera may occupy. |
| `maxY` | `int` | Maximum camera Y position the camera may occupy. |
| `valid` | `bool` | `false` when `world.valid` was `false`; see above. |
