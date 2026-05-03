# LayerAttributes

<Badge type="info" text="Struct" />

**Source:** `Renderer.h`

## Description

All tiles with attributes in a single tilemap layer.

LayerAttributes organizes all tile metadata for a single layer, providing
efficient lookup of attributes by tile position. Only tiles with non-empty
attributes are included, using a sparse representation to minimize memory.

Layer Organization:
- Each layer in a scene has its own LayerAttributes structure
- Layers are typically organized as: Background, Midground, Foreground, etc.
- Layer name matches the name defined in the Tilemap Editor

Memory Efficiency:
- Sparse representation: only tiles with attributes are stored
- All data stored in PROGMEM (flash memory) on ESP32
- No RAM overhead for attribute storage
- Typical size: ~40 bytes per tile with attributes (depends on key/value lengths)

Query Workflow:
1. Identify layer by index or name
2. Search tiles array for matching (x, y) position
3. If found, iterate through tile's attributes array
4. Compare keys using strcmp_P() for PROGMEM strings

Example Usage:
```cpp
// Query attribute for tile at (10, 5) in layer 0
const char* value = get_tile_attribute(0, 10, 5, "solid");
if (value && strcmp_P(value, "true") == 0) {
    // Tile is solid
}
```

Layer name is a PROGMEM string (use strcmp_P() for comparison)
Tiles array is sorted by position for potential binary search optimization
Maximum 65535 tiles with attributes per layer (uint16_t limit)

TileAttributeEntry for individual tile attributes
TileAttribute for key-value pairs
Generated scene headers for query helper functions

::: tip
Layer name is a PROGMEM string (use strcmp_P() for comparison)
:::

::: tip
Tiles array is sorted by position for potential binary search optimization
:::

::: tip
Maximum 65535 tiles with attributes per layer (uint16_t limit)
:::

## Properties

| Name | Type | Description |
|------|------|-------------|
| `char` | `const` | Layer name (PROGMEM string, e.g., "Background") |
| `num_tiles_with_attributes` | `uint16_t` | Number of tiles with attributes in this layer |
| `TileAttributeEntry` | `const` | PROGMEM array of tiles with attributes (sparse) |
