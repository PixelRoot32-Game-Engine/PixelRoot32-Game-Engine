# Multi-palette and indexed color

Sprites and tile layers use **indexed** pixel data plus **palette** tables. You can switch palettes per draw call or layer to get more visual variety without wider BPP.

## Modes

- **1bpp** — smallest footprint; one bit per pixel selects between two colors from a palette.
- **2bpp / 4bpp** — optional; enable with `PIXELROOT32_ENABLE_2BPP_SPRITES` / `PIXELROOT32_ENABLE_4BPP_SPRITES` (see [Configuration](../api/config.md)).

Built-in palette presets (PR32, NES, Game Boy, PICO-8, etc.) are described in the [Color / graphics API](../api/graphics.md#color-and-palettes).

## Workflow

1. Store art as indices (and optional per-tile attributes).
2. Assign a `PaletteType` or custom colors when drawing sprites or tile layers.
3. Use multi-palette tile attributes where supported (see tile attribute section in [TileMap API](../api/graphics.md#tilemaps)).

## Example project

The `examples/dual_palette` sample in the engine repository demonstrates switching palettes in a real scene.

## Related

- [Rendering](./rendering.md)
- [Tilemaps](./tilemaps.md)
