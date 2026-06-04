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

## Methods

### `inline const char* get_tile_attribute( const LayerAttributes* layer_attributes, uint8_t num_layers, uint8_t layer_idx, uint16_t x, uint16_t y, const char* key )`

**Description:**

Query a tile attribute value by position and key.

**Parameters:**

- `layer_attributes`: Pointer to PROGMEM array of LayerAttributes
- `num_layers`: Number of layers in the array
- `layer_idx`: Index of the layer to query (0-based)
- `x`: Tile X coordinate
- `y`: Tile Y coordinate
- `key`: Attribute key to search for (may be RAM or PROGMEM string)

**Returns:** Pointer to PROGMEM attribute value string, or nullptr if not found

::: tip
Returns nullptr if:
      - layer_idx >= num_layers (out of bounds)
      - Layer is empty (num_tiles_with_attributes == 0)
      - Tile at (x, y) does not exist
      - Tile exists but does not have the specified key
:::

::: tip
The returned pointer references PROGMEM. Use strcmp_P() or similar
      functions to compare values.

Example usage:
```cpp
// Query "solid" attribute for tile at (10, 5) in layer 0
const char* value = pixelroot32::graphics::get_tile_attribute(
    layer_attributes, NUM_LAYERS_WITH_ATTRIBUTES, 0, 10, 5, "solid"
);
if (value && strcmp_P(value, "true") == 0) {
    // Tile is solid
}
```
:::

### `inline bool tile_has_attributes( const LayerAttributes* layer_attributes, uint8_t num_layers, uint8_t layer_idx, uint16_t x, uint16_t y )`

**Description:**

Check if a tile at the given position has any attributes.

**Parameters:**

- `layer_attributes`: Pointer to PROGMEM array of LayerAttributes
- `num_layers`: Number of layers in the array
- `layer_idx`: Index of the layer to query (0-based)
- `x`: Tile X coordinate
- `y`: Tile Y coordinate

**Returns:** true if tile has attributes, false otherwise

::: tip
Returns false if:
      - layer_idx >= num_layers (out of bounds)
      - Layer is empty (num_tiles_with_attributes == 0)
      - Tile at (x, y) does not exist in the layer
:::

::: tip
This function only checks for tile existence. To query specific
      attribute values, use get_tile_attribute().

Example usage:
```cpp
// Check if tile at (10, 5) in layer 0 has any attributes
if (pixelroot32::graphics::tile_has_attributes(
    layer_attributes, NUM_LAYERS_WITH_ATTRIBUTES, 0, 10, 5
)) {
    // Tile has attributes, query specific values
    const char* solid = pixelroot32::graphics::get_tile_attribute(
        layer_attributes, NUM_LAYERS_WITH_ATTRIBUTES, 0, 10, 5, "solid"
    );
}
```
:::

### `inline const TileAttributeEntry* get_tile_entry( const LayerAttributes* layer_attributes, uint8_t num_layers, uint8_t layer_idx, uint16_t x, uint16_t y )`

**Description:**

Get the TileAttributeEntry for a tile at the given position.

**Parameters:**

- `layer_attributes`: Pointer to PROGMEM array of LayerAttributes
- `num_layers`: Number of layers in the array
- `layer_idx`: Index of the layer to query (0-based)
- `x`: Tile X coordinate
- `y`: Tile Y coordinate

**Returns:** Pointer to TileAttributeEntry in PROGMEM, or nullptr if not found

::: tip
Returns nullptr if:
      - layer_idx >= num_layers (out of bounds)
      - Layer is empty (num_tiles_with_attributes == 0)
      - Tile at (x, y) does not exist in the layer
:::

::: tip
The returned pointer references PROGMEM. Use PIXELROOT32_MEMCPY_P
      to read the entry into RAM before accessing its fields.
:::

::: tip
This function is more efficient than calling get_tile_attribute()
      multiple times for the same tile, as it performs the position lookup
      only once.

Example usage:
```cpp
// Query multiple attributes from tile at (10, 5) in layer 0
const TileAttributeEntry* entry_ptr = pixelroot32::graphics::get_tile_entry(
    layer_attributes, NUM_LAYERS_WITH_ATTRIBUTES, 0, 10, 5
);

if (entry_ptr) {
    // Read entry from PROGMEM into RAM
    TileAttributeEntry entry;
    PIXELROOT32_MEMCPY_P(&entry, entry_ptr, sizeof(TileAttributeEntry));
    
    // Iterate through all attributes
    for (uint8_t i = 0; i < entry.num_attributes; i++) {
        TileAttribute attr;
        PIXELROOT32_MEMCPY_P(&attr, &entry.attributes[i], sizeof(TileAttribute));
        
        // Process attribute key and value
        // attr.key and attr.value are PROGMEM pointers
        // Use strcmp_P() or similar functions to compare
    }
}
```
:::
