# Sprite Compiler Advanced Features

Advanced behaviour of the PixelRoot32 Sprite Compiler: palettes, prefixes, and export modes.

## Automatic palette detection

The compiler can detect when all colors in a sprite match one of the engine’s built-in palettes, which keeps headers smaller and matches runtime palette setup.

### Predefined engine palettes

The engine exposes five built-in palette types (see [Color (API)](/api/graphics/color)):

- `PR32` — default PixelRoot32 palette
- `NES`
- `GB`
- `GBC`
- `PICO8`

### How it works

1. **Detection**: Unique colors in the image are compared against those palette tables.
2. **Match**: If every color fits a built-in palette, the tool may **omit** a custom RGB mapping array and assume you use `Color::setPalette` / engine palettes at runtime.
3. **Custom colors**: If any color is outside those sets, the header can include a `{PREFIX}_PALETTE_MAPPING[16]` (or similar) for your indices.

Exact behaviour depends on the tool version — use `python main.py --help` on your checkout.

## Naming with prefixes

Use **`--prefix`** (or the GUI **Prefix** field) to namespace generated symbols and avoid collisions.

```bash
python main.py sheet.png --grid 16x16 --sprite 0,0,1,1 --prefix PLAYER_JUM
```

Typical generated identifiers:

- `PLAYER_JUM_SPRITE_0_LAYER_0`
- `PLAYER_JUM_SPRITE_0_2BPP`
- `PLAYER_JUM_SPRITE_0_4BPP`
- `PLAYER_JUM_PALETTE_MAPPING` (when a custom mapping is emitted)

## Export modes

### Layered (1bpp layers)

Best match for classic PixelRoot32 drawing: one bitmask per color layer, composited by the renderer.

### Packed 2bpp / 4bpp

Single array, multiple bits per pixel:

- **2bpp**: up to 4 colors (index 0 often treated as transparent in engine conventions).
- **4bpp**: up to 16 colors.

Packed modes can reduce draw setup when a sprite uses several colors in one array.

## Next steps

- [Usage guide](/tools/sprite-compiler/usage-guide) — CLI workflow and build integration
- [Installation](/tools/sprite-compiler/installation)
- [Overview](/tools/sprite-compiler/overview)

## See also

- [Rendering](/guide/rendering)
- [Graphics Techniques](/guide/graphics-techniques.md)
