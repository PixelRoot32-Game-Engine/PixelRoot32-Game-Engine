# Bomberman Sprite Assets — Regeneration Guide

## How to regenerate headers from the local PNG

1. Open `docs/audits/NES - Bomberman - Miscellaneous - General Sprites.png` in
   your image editor.
2. For each asset category below, slice the corresponding 16×16 pixel region.
3. Convert each pixel to RGB565 using the standard formula:
   `(R>>3)<<11 | (G>>2)<<5 | (B>>3)`
4. Pack 4bpp (2 pixels per byte) into `uint16_t` arrays of 64 entries per frame.
5. Update the `static const uint16_t ARRAY_NAME_4BPP[64]` arrays in the
   corresponding `.h` file.

## Slice regions (human terms)

The NES Bomberman general sprite sheet at
`docs/audits/NES - Bomberman - Miscellaneous - General Sprites.png` contains
the following regions:

| Region          | Files                                | Description                                     |
|-----------------|--------------------------------------|-------------------------------------------------|
| Top rows        | PlayerSprites.h                      | Bomberman walk cycles (3 frames × 3 directions: Down, Up, Right) + 5-frame death animation |
| Bomb row        | BombSprites.h                        | 3-frame bomb pulse cycle (idle + 2 flash states) |
| Middle crosses  | ExplosionSprites.h                   | 7 directional segments (Center, H arm, V arm, 4 tips) × 2 flicker frames each |
| Upper right     | BoardTiles.h                         | Hard wall, soft wall, exit door |
| Power-up row    | BoardTiles.h                         | Fire and Bomb power-up icons |
| Enemy rows      | EnemySprites.h                       | Ballom: 2 walk frames + 3 death frames |

Exact pixel coordinates are left to the developer — the NES sheet layout
varies, and you should verify slices visually.

## Palette

The shared palette is in `BombermanPalette.h`:
- 10 entries (indices 0-9) covering transparent, black, white, gray, orange,
  red, yellow, blue, purple, and light green.
- Slots 10-15 are unused (0x0000).
- If your sliced sprites reference a color not in the palette, adjust the
  RGB565 values in `BombermanPalette.h` or remap the sprite data.

## .gitignore status

The generated `.h` files under `examples/bomberman/src/assets/` are tracked
in git. The `.gitignore` in the example root excludes build artifacts only.

## Licensing warning

The NES Bomberman sprite sheet is © Hudson Soft / Konami. These generated
headers are for personal/educational use only. Do NOT distribute the PNG
or the generated headers publicly. Commission CC0 replacement art before
any public sample release.
