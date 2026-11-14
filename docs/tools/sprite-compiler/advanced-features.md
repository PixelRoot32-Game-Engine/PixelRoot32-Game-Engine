# Sprite Compiler Advanced Features

Advanced behaviour of the PixelRoot32 Sprite Compiler: palettes, prefixes, and export modes. These apply to both the Tool Suite module and the standalone CLI — only the invocation differs.

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

Exact behaviour depends on the tool version — use `python main.py --help` on your checkout (standalone CLI), or check the tooltip / Log panel in the Tool Suite module.

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

### Storage: one copy in flash

Since CLI v0.4.0, every generated header starts with `#pragma once`, in every mode, with or without a prefix.

A prefix also changes how the arrays are stored:

- **With a prefix**, the palette mapping and every sprite array are emitted as C++17 `inline constexpr` instead of `static const`. The data is stored once in flash, no matter how many `.cpp` files include the header. Names, types, values and order are the same as before.
- **Without a prefix**, the arrays stay `static const`, so each `.cpp` file that includes the header gets its own copy. This keeps generic names such as `SPRITE_0_4BPP` from being merged across different sprite sheets.

```cpp
// --prefix PLAYER
inline constexpr uint16_t PLAYER_SPRITE_0_4BPP[] = { /* ... */ };

// no prefix
static const uint16_t SPRITE_0_4BPP[] = { /* ... */ };
```

Rules for prefixed exports:

- **Use a unique prefix per sprite sheet** within a game. Two `inline` arrays with the same name are treated as one definition and merged at link time, so two sheets exported with the same prefix would silently share data.
- **Compile as C++17 or newer.** `inline` variables are a C++17 feature.

The Tool Suite module shows the same hint next to the **Prefix** field.

## Naming sprites

Namespaces keep two sheets apart; **`--sprite-name`** (CLI v0.5.0 and newer) also makes each sprite's symbols say what they contain. Give it **once per `--sprite`, in the same order**:

```bash
python main.py player.png --grid 16x32 \
  --prefix PLAYER --sprite 0,0,1,1 --sprite 1,0,1,1 \
  --sprite-name idle --sprite-name walk \
  --mode 4bpp --out sprites_player.h
```

```cpp
// --prefix PLAYER --sprite-name idle / walk
inline constexpr uint16_t PLAYER_idle_4BPP[] = { /* ... */ };
inline constexpr uint16_t PLAYER_walk_4BPP[] = { /* ... */ };
```

The stem is `{prefix}{name}`; the suffix still comes from the mode (`_LAYER_k`, `_2BPP`, `_4BPP`), and `_PALETTE_MAPPING` keeps its prefix-only name. Without `--prefix` the named arrays stay `static const`, exactly like the index-named ones.

Rules, checked before anything is written (a rejected run creates no header and exits with status 1):

- **One name per sprite.** `--sprite-name must be given once per --sprite (got N name(s) for M sprite(s))`.
- **A valid C identifier.** `[A-Za-z_][A-Za-z0-9_]*`; anything else is reported as `not a valid C identifier`.
- **Unique within the header.** Two sprites sharing a name would share a symbol.
- **Opt-in.** Without the flag the header keeps the index stems (`PLAYER_SPRITE_0_4BPP`) and is byte-identical to the output of the previous release.

The prefix rules above still apply to named arrays: `inline constexpr` with a prefix, `static const` without it.

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
