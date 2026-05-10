# TileAttributeEntry

<Badge type="info" text="Struct" />

**Source:** `Renderer.h`

## Description

All attributes for a single tile at a specific position.

TileAttributeEntry associates a tile position (x, y) with its metadata attributes.
Only tiles that have attributes are included in the exported data (sparse
representation), minimizing memory usage for large tilemaps.

Attribute Resolution:
- Editor merges tileset default attributes with instance overrides
- Only final resolved attributes are exported (no inheritance logic at runtime)
- Empty tiles (no attributes) are not included in the exported data

Position Encoding:
- X and Y are tile coordinates (not pixel coordinates)
- Coordinates are relative to the layer's origin (0, 0)
- Maximum coordinate value: 65535 (uint16_t range)

Query Pattern:
```cpp
// Find tile at position (10, 5)
for (uint16_t i = 0; i < layer.num_tiles_with_attributes; i++) {
    if (layer.tiles[i].x == 10 && layer.tiles[i].y == 5) {
        // Found tile, search attributes
        for (uint8_t j = 0; j < layer.tiles[i].num_attributes; j++) {
            if (strcmp_P(layer.tiles[i].attributes[j].key, "solid") == 0) {
                // Found "solid" attribute
            }
        }
    }
}
```

Attributes array is stored in PROGMEM (flash memory)
Maximum 255 attributes per tile (uint8_t limit)
Use helper functions like get_tile_attribute() for easier queries

TileAttribute for individual key-value pairs
LayerAttributes for layer-level organization

::: tip
Attributes array is stored in PROGMEM (flash memory)
:::

::: tip
Maximum 255 attributes per tile (uint8_t limit)
:::

::: tip
Use helper functions like get_tile_attribute() for easier queries
:::

## Properties

| Name | Type | Description |
|------|------|-------------|
| `x` | `uint16_t` | Tile X coordinate in layer space |
| `y` | `uint16_t` | Tile Y coordinate in layer space |
| `num_attributes` | `uint8_t` | Number of attributes for this tile |
| `TileAttribute` | `const` | PROGMEM array of attribute key-value pairs |
