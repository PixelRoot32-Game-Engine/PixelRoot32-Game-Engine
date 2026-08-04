# Sprite Compiler Usage Guide

Complete guide to using the PixelRoot32 Sprite Compiler. The compiler is available in two modes:

- **Tool Suite module** — native ImGui GUI inside the Tool Suite application.
- **Standalone CLI** — `python main.py` from the open-source repo (see [installation](/tools/sprite-compiler/installation)).

Both produce identical C headers. The Tool Suite module delegates compilation to the `pr32-sprite-compiler` binary (bundled or on `PATH`). The standalone CLI runs directly from Python — ideal for CI and build automation.

> **Note:** There is **no** official Node/npm global package; ignore any older docs that referenced `npm install -g pr32-sprite-compiler`.

## GUI Usage (Tool Suite)

### Opening the module

1. Launch the **PixelRoot32 Tool Suite**
2. Click **"Sprite Compiler"** on the launcher card

The module opens a 3-panel docked window:

- **Settings** (left) — Input image, Tile Settings (grid W×H, offset), Sprite Selection, Export (mode, prefix, output path)
- **Preview** (center) — Real-time visual preview of the selected sprite
- **Log** (bottom) — Colour-coded output (Info / Warning / Error / Success)

### Module workflow

1. **Open or create a project** (File menu: Ctrl+N / Ctrl+O)
2. **Select an input image** — browse or type the PNG path in the Settings panel
3. **Configure grid** — set tile Width × Height, offset, and limit
4. **Select sprites** — auto-detect or manually add regions
5. **Export** — choose mode (Layered / 2bpp / 4bpp), prefix, output file, and click **"Export Sprites"**

Projects are saved as `.pr32sprite` files so you can resume work later. The module owns its **File** menu (a toolbar popup): New, Open, Save, Save As, Close.

### Keyboard shortcuts

| Shortcut | Action |
|----------|--------|
| `Ctrl+N` | New project |
| `Ctrl+O` | Open project |
| `Ctrl+S` | Save project |
| `Ctrl+Shift+S` | Save project as |
| `Ctrl+W` | Close project |

## Standalone CLI

For automation, build scripts, and CI environments. Install via [Python source](/tools/sprite-compiler/installation#method-2-standalone-cli-python-source).

```bash
python main.py [input] [options]
```

**Required:**

- `input`: Input PNG image file
- `--grid WxH`: Grid cell size (e.g., `16x16`)
- `--sprite gx,gy,gw,gh`: Sprite definition (can be repeated)

**Optional:**

- `--prefix NAME`: Prefix for generated arrays (e.g., `PLAYER_JUM`)
- `--out FILE`: Output header file (default: `sprites.h`)
- `--offset X,Y`: Initial offset in pixels (default: `0,0`)
- `--mode MODE`: Export mode (`layered`, `2bpp`, `4bpp`)

## CLI Examples (Standalone)

### Simple Conversion

Convert a single 16x16 sprite located at the top-left corner:

```bash
python main.py player.png --grid 16x16 --sprite 0,0,1,1 --out player.h
```

### Multiple Sprites

Convert multiple sprites from a single sheet:

```bash
python main.py sheet.png --grid 16x16 \
  --sprite 0,0,1,1 \
  --sprite 1,0,1,1 \
  --sprite 2,0,1,1 \
  --out animations.h
```

### Export Modes

**Layered (Default):**
Generates multiple `uint16_t` arrays, one for each color layer. Best for standard PixelRoot32 rendering.

```bash
python main.py icon.png --grid 16x16 --sprite 0,0,1,1 --mode layered
```

**Packed 2bpp:**
Generates a single array with 2 bits per pixel (4 colors max).

```bash
python main.py icon.png --grid 16x16 --sprite 0,0,1,1 --mode 2bpp
```

## Step-by-Step Examples

### Example 1: Simple Player Sprite

**Step 1: Create Image**

- Create an 8x8 pixel PNG image
- Use black and white colors
- Save as `player.png`

**Step 2: Compile**

```bash
python main.py player.png --grid 8x8 --sprite 0,0,1,1 --prefix PLAYER --out player_sprite.h
```

**Step 3: Use in Code**

```cpp
#include "player_sprite.h"

void draw() {
    // PLAYER_SPRITE_0_LAYER_0 is generated automatically
    renderer.drawSprite(PLAYER_SPRITE_0_LAYER_0, 100, 100, Color::White);
}
```

### Example 2: Multiple Animation Frames

**Step 1: Prepare Images**

- Create frames: `walk_0.png`, `walk_1.png`, `walk_2.png`
- All same size (e.g., 16x16)

**Step 2: Compile each frame**

```bash
mkdir -p animations
for f in walk_*.png; do
  python main.py "$f" --grid 16x16 --sprite 0,0,1,1 --prefix WALK_ --out "animations/$(basename "$f" .png).h"
done
```

**Step 3: Use in Animation**

```cpp
#include "animations/walk_0.h"
#include "animations/walk_1.h"
#include "animations/walk_2.h"

const Sprite* WALK_FRAMES[] = {
    &WALK_0_SPRITE,
    &WALK_1_SPRITE,
    &WALK_2_SPRITE
};
```

### Example 3: Sprite Sheet

**Step 1: Create Sprite Sheet**

- Create a 64x64 image with 4x4 grid of 16x16 sprites
- Save as `characters.png`

**Step 2: Export cells from the sheet**

Use **`--grid`** for cell size and one **`--sprite gx,gy,gw,gh`** per cell (grid coordinates). Example for the **top row** of a 4×4 grid of 16×16 tiles:

```bash
python main.py characters.png --grid 16x16 \
  --sprite 0,0,1,1 --sprite 1,0,1,1 --sprite 2,0,1,1 --sprite 3,0,1,1 \
  --out characters.h
```

Add `--sprite` lines for `y = 1…3` for the remaining rows (or pick cells in the GUI).

**Step 3: Use Individual Sprites**

```cpp
#include "characters.h"

// Sprites named {PREFIX}_SPRITE_{N}_LAYER_{layer}, e.g. CHARACTER_SPRITE_0_LAYER_0
renderer.drawSprite(CHARACTER_SPRITE_0_LAYER_0, 50, 50, Color::White);
renderer.drawSprite(CHARACTER_SPRITE_1_LAYER_0, 70, 50, Color::White);
```

> With `--mode layered`, each sprite emits one array per color layer (`..._SPRITE_N_LAYER_{i}`). Packed modes emit `..._SPRITE_N_2BPP` / `..._SPRITE_N_4BPP`.

## Batch Processing

### Process Multiple Files

Process all PNG files in a directory:

```bash
mkdir -p generated
for f in sprites/*.png; do
  python main.py "$f" --grid 16x16 --sprite 0,0,1,1 --out "generated/$(basename "$f" .png).h"
done
```

### With Options

Apply the same options to each file (example: **`--mode`** / **`--prefix`** as supported by `python main.py --help`):

```bash
mkdir -p src/sprites
for f in assets/*.png; do
  python main.py "$f" --grid 16x16 --sprite 0,0,1,1 --mode layered --prefix SPRITE_ --out "src/sprites/$(basename "$f" .png).h"
done
```

### Recursive Processing

Process nested paths (shell-dependent; example with `find`):

```bash
mkdir -p generated
find assets -name '*.png' -print0 | while IFS= read -r -d '' f; do
  base=$(basename "$f" .png)
  python main.py "$f" --grid 16x16 --sprite 0,0,1,1 --out "generated/${base}.h"
done
```

## Sprite sheets (grid + cells)

There is **no** separate `sheet`/`count` subcommand in the documented flow: set **`--grid WxH`** and add one **`--sprite gx,gy,gw,gh`** per cell (grid units). Example — four 8×8 tiles along the top row:

```bash
python main.py sheet.png --grid 8x8 \
  --sprite 0,0,1,1 --sprite 1,0,1,1 --sprite 2,0,1,1 --sprite 3,0,1,1 \
  --prefix TILE_ \
  --out output.h
```

Use **`--prefix`** to keep generated symbol names readable.

## Palettes, thresholds, and other switches

Extra options (custom palette files, built-in palette names, dithering, header guard, includes, etc.) **depend on the version of the tool**. From the repository root, run:

```bash
python main.py --help
```

and match your CLI to that output. Do **not** assume a globally installed `pr32-sprite-compiler` from npm.

## Build-system Automation (Standalone CLI)

### PlatformIO

Add to `platformio.ini`:

```ini
[env:esp32dev]
extra_scripts = 
    pre:scripts/compile_sprites.py
```

**compile_sprites.py** (adjust `SPRITE_COMPILER_ROOT` to your clone):

```python
Import("env")
import subprocess
from pathlib import Path

SPRITE_COMPILER_ROOT = Path("path/to/PixelRoot32-Sprite-Compiler").resolve()

def compile_one(png: Path, out_h: Path) -> None:
    subprocess.run(
        [
            "python",
            str(SPRITE_COMPILER_ROOT / "main.py"),
            str(png),
            "--grid", "16x16",
            "--sprite", "0,0,1,1",
            "--out",
            str(out_h),
        ],
        check=True,
    )

out_dir = Path("src/sprites")
out_dir.mkdir(parents=True, exist_ok=True)
for png in Path("assets/sprites").glob("*.png"):
    compile_one(png, out_dir / f"{png.stem}.h")
```

### Makefile

```makefile
SPRITES = $(wildcard assets/sprites/*.png)
SPRITE_HEADERS = $(SPRITES:assets/sprites/%.png=src/sprites/%.h)

SPRITE_COMPILER_ROOT := path/to/PixelRoot32-Sprite-Compiler

src/sprites/%.h: assets/sprites/%.png
	python $(SPRITE_COMPILER_ROOT)/main.py $< --grid 16x16 --sprite 0,0,1,1 --out $@

sprites: $(SPRITE_HEADERS)
```

### CMake

```cmake
# Set SPRITE_COMPILER_ROOT to your PixelRoot32-Sprite-Compiler clone
set(SPRITE_COMPILER_ROOT "/absolute/path/to/PixelRoot32-Sprite-Compiler")

file(GLOB SPRITE_FILES "assets/sprites/*.png")

foreach(SPRITE ${SPRITE_FILES})
    get_filename_component(SPRITE_NAME ${SPRITE} NAME_WE)
    add_custom_command(
        OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/src/sprites/${SPRITE_NAME}.h
        COMMAND python
        ARGS ${SPRITE_COMPILER_ROOT}/main.py
             ${SPRITE}
             --grid 16x16 --sprite 0,0,1,1
             --out ${CMAKE_CURRENT_SOURCE_DIR}/src/sprites/${SPRITE_NAME}.h
        DEPENDS ${SPRITE}
    )
endforeach()
```

## Build Integration (Standalone CLI)

### Image Preparation

- **Use indexed color PNG** for best results
- **Keep sprites small** (8x8, 16x16, 32x32)
- **Use black and white** for 1bpp
- **Limit colors** for 2bpp/4bpp formats

### File Organization

```
project/
├── assets/
│   └── sprites/
│       ├── player.png
│       ├── enemy.png
│       └── items.png
├── src/
│   └── sprites/          # Generated headers
│       ├── player.h
│       ├── enemy.h
│       └── items.h
└── platformio.ini
```

### Naming Conventions

- Use descriptive names: `player_walk_0.png` → `PLAYER_WALK_0_SPRITE`
- Be consistent: All caps for sprite names
- Use prefixes: `ENEMY_`, `PLAYER_`, `ITEM_`

### Version Control

- **Commit generated headers** (they're part of the build)
- **Or** add to `.gitignore` and regenerate on build
- **Keep source images** in version control

## Troubleshooting

### Common Issues

**"Image too large":**

- Sprites must be ≤ 16 pixels wide for 1bpp
- Resize image or split into multiple sprites

**"Colors not converting correctly":**

- Use indexed color PNG
- For 1bpp: Use only black and white
- For 2bpp: Use exactly 4 colors
- For 4bpp: Use up to 16 colors

**"Output file not found":**

- Check write permissions
- Verify output directory exists
- Use absolute paths if needed

**"Invalid format":**

- Ensure input is PNG format
- Check file is not corrupted
- Try re-saving image in image editor

**Tool Suite: Export fails silently or warns about missing CLI**

- The `pr32-sprite-compiler` binary must be available alongside the Tool Suite executable or on `PATH`.
- Re-run the Tool Suite installer, or download the binary from the [releases](https://github.com/PixelRoot32-Game-Engine/PixelRoot32-Sprite-Sheet-Compiler/releases) page and place it next to the Tool Suite.
- Check the Log panel at the bottom of the Sprite Compiler module for the exact error message.

### Getting Help (Standalone CLI)

```bash
python main.py --help
```

Shows all available options and usage.

## Next Steps

- [Advanced Features](/tools/sprite-compiler/advanced-features) - Explore advanced options
- [Overview](/tools/sprite-compiler/overview) - Learn more about the compiler
- [Manual - Sprites](/guide/rendering) - Using sprites in games

## See Also

- [Code Examples - Sprites](/examples/sprite-animation) - Sprite usage examples
- [Testing](/guide/testing) — engine test / dev workflow
