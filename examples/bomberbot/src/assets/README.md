# Bomberbot Sprite Assets — Regeneration Guide

> **✅ ORIGINAL CC0 ARTWORK**
>
> The sprite data in this directory (`.h` headers) is **original pixel art
> designed from scratch** for the PixelRoot32 demo. It is not derived from
> any copyrighted game artwork, and is free to use and redistribute under
> the CC0 / public-domain intent of this project.
>
> - **Design**: robot player, green slime enemy, round bomb, fire-cross
>   explosion, and board tiles — all original compositions.
> - **Technical parity**: 4bpp, 16×16 grid, shared 16-color palette, and the
>   same symbol names / frame counts as before, so the game code is
>   unaffected by regeneration.
> - **License**: engine code is MIT; these assets are original and carry no
>   third-party restriction.

## How to regenerate headers

Run the generator script from the repo root:

```bash
python examples/bomberbot/tools/generate_cc0_assets.py
```

It writes every header in this directory. The art itself lives in the
script as 16×16 character grids (one character per palette index); edit the
grids there, then re-run the script.

## Header contents

| Header              | Contents                                                          |
|---------------------|-------------------------------------------------------------------|
| `PlayerSprites.h`   | Robot walk cycles (3 frames × 3 directions: Down, Up, Right; Left mirrors Right via `flipX`) + 6-frame death animation |
| `BombSprites.h`     | 3-frame bomb pulse cycle (idle + 2 flash states)                  |
| `ExplosionSprites.h`| Fire-cross segments: Center, H/V arms (base + ext), 4 tips + `_BASE` variants, × 4 growth frames each |
| `BoardTiles.h`      | Hard wall, soft wall, exit door, Fire + Bomb power-up icons, and the 14-slot `kPowerUpsAll` sheet |
| `EnemySprites.h`    | Green slime enemy: 7 walk frames (3 right + 3 left + 1 filler) + 4 death frames |

## Packing format

Matches the engine's `Sprite4bpp` (`include/graphics/Renderer.h`):

- 16×16 @ 4bpp → 8 bytes/row → 4 `uint16_t`/row → 64 entries per frame.
- Byte low nibble = left pixel, high nibble = right pixel.
- `uint16 = p0 | p1<<4 | p2<<8 | p3<<12` (little-endian byte order).
- Palette index 0 in sprite data means **transparent** (never drawn).

## Palette

The shared palette is in `BomberbotPalette.h` (16 RGB565 entries). If you
change a grid in the generator to use a palette index not covered by that
sprite's palette mapping, adjust `BOMBERBOT_PALETTE_MAPPING` /
`PLAYER_PALETTE_MAPPING` accordingly.

## .gitignore status

The generated `.h` files under `examples/bomberbot/src/assets/` are tracked
in git. The `.gitignore` in the example root excludes build artifacts only.
