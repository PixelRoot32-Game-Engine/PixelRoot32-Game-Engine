# TileAttribute

<Badge type="info" text="Struct" />

**Source:** `Renderer.h`

## Description

Single attribute key-value pair for tile metadata.

TileAttribute represents a single metadata entry attached to a tile, such as
collision properties, interaction types, or game-specific data. Both key and
value are stored as PROGMEM strings to minimize RAM usage on ESP32.

Attributes are exported from the PixelRoot32 Tilemap Editor with final resolved
values (tileset defaults merged with instance overrides). The editor's two-level
attribute system (default + instance) is collapsed at export time, so runtime
code only sees the final merged result.

Common Use Cases:
- Collision detection: {"solid", "true"}, {"walkable", "false"}
- Interaction: {"type", "door"}, {"interactable", "true"}
- Game logic: {"damage", "10"}, {"health", "50"}
- Tile behavior: {"animated", "true"}, {"speed", "2"}

Memory Layout:
- Both pointers reference flash memory (PROGMEM/PIXELROOT32_SCENE_FLASH_ATTR)
- No RAM overhead for string storage
- Suitable for ESP32 with limited RAM

All strings are null-terminated C strings stored in flash memory
Use strcmp_P() or similar PROGMEM-aware functions to compare keys
Values are always strings; convert to int/bool as needed in game code

TileAttributeEntry for tile position association
LayerAttributes for layer-level attribute organization

::: tip
All strings are null-terminated C strings stored in flash memory
:::

::: tip
Use strcmp_P() or similar PROGMEM-aware functions to compare keys
:::

::: tip
Values are always strings; convert to int/bool as needed in game code
:::

## Properties

| Name | Type | Description |
|------|------|-------------|
| `char` | `const` | Attribute key (PROGMEM string, e.g., "type", "solid") |
