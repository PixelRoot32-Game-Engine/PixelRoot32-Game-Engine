/*
 * Original work:
 * Copyright (c) nbourre
 * Licensed under the MIT License
 *
 * Modifications:
 * Copyright (c) 2026 PixelRoot32
 *
 * This file remains licensed under the MIT License.
 */
#pragma once
#include "platforms/PlatformMemory.h"

#include "DrawSurface.h"
#include "DisplayConfig.h"
#include "Color.h"
#include "Font.h"
#include <memory>
#include <string_view>

namespace pixelroot32::graphics {

/**
 * @brief Compact sprite descriptor for monochrome bitmapped sprites.
 *
 * Sprites are stored as an array of 16-bit rows. Each row packs horizontal
 * pixels into bits, using the following convention:
 *
 * - Bit 0 represents the leftmost pixel of the row.
 * - Bit (width - 1) represents the rightmost pixel of the row.
 *
 * Only the lowest (width) bits of each row are used. A bit value of 1 means
 * "pixel on", 0 means "pixel off".
 *
 * This format is optimized for small microcontroller displays (NES/GameBoy
 * style assets) and keeps data in flash-friendly, constexpr-friendly form.
 */
struct Sprite {
    const uint16_t* data;   ///< Pointer to packed row data (size = height).
    uint8_t         width;  ///< Sprite width in pixels (<= 16).
    uint8_t         height; ///< Sprite height in pixels.
};

struct Sprite2bpp {
    const uint8_t*  data;
    const Color*    palette;
    uint8_t         width;
    uint8_t         height;
    uint8_t         paletteSize;
};

struct Sprite4bpp {
    const uint8_t*  data;
    const Color*    palette;
    uint8_t         width;
    uint8_t         height;
    uint8_t         paletteSize;
};

/**
 * @brief Single monochrome layer used by layered sprites.
 *
 * Each layer uses the same width/height as its owning MultiSprite but can
 * provide its own bitmap and color.
 */
struct SpriteLayer {
    const uint16_t* data;   ///< Pointer to packed row data for this layer.
    Color           color;  ///< Color used for "on" pixels in this layer.
};

/**
 * @brief Multi-layer, multi-color sprite built from 1bpp layers.
 *
 * A MultiSprite combines several SpriteLayer entries that share the same
 * width and height. Layers are drawn in array order, allowing more complex
 * visuals (highlights, outlines) while keeping each layer 1bpp.
 *
 * This design keeps compatibility with the existing Sprite format while
 * enabling NES/GameBoy-style layered sprites.
 */
struct MultiSprite {
    uint8_t              width;      ///< Sprite width in pixels (<= 16).
    uint8_t              height;     ///< Sprite height in pixels.
    const SpriteLayer*   layers;     ///< Pointer to array of layers.
    uint8_t              layerCount; ///< Number of layers in the array.
};

template<typename T>
struct TileMapGeneric {
    uint8_t*        indices;
    uint8_t         width;
    uint8_t         height;
    const T*        tiles;
    uint8_t         tileWidth;
    uint8_t         tileHeight;
    uint16_t        tileCount;
};

using TileMap = TileMapGeneric<Sprite>;

using TileMap2bpp = TileMapGeneric<Sprite2bpp>;

using TileMap4bpp = TileMapGeneric<Sprite4bpp>;

/**
 * @brief Single attribute key-value pair for tile metadata.
 *
 * TileAttribute represents a single metadata entry attached to a tile, such as
 * collision properties, interaction types, or game-specific data. Both key and
 * value are stored as PROGMEM strings to minimize RAM usage on ESP32.
 *
 * Attributes are exported from the PixelRoot32 Tilemap Editor with final resolved
 * values (tileset defaults merged with instance overrides). The editor's two-level
 * attribute system (default + instance) is collapsed at export time, so runtime
 * code only sees the final merged result.
 *
 * Common Use Cases:
 * - Collision detection: {"solid", "true"}, {"walkable", "false"}
 * - Interaction: {"type", "door"}, {"interactable", "true"}
 * - Game logic: {"damage", "10"}, {"health", "50"}
 * - Tile behavior: {"animated", "true"}, {"speed", "2"}
 *
 * Memory Layout:
 * - Both pointers reference flash memory (PROGMEM/PIXELROOT32_SCENE_FLASH_ATTR)
 * - No RAM overhead for string storage
 * - Suitable for ESP32 with limited RAM
 *
 * @note All strings are null-terminated C strings stored in flash memory
 * @note Use strcmp_P() or similar PROGMEM-aware functions to compare keys
 * @note Values are always strings; convert to int/bool as needed in game code
 *
 * @see TileAttributeEntry for tile position association
 * @see LayerAttributes for layer-level attribute organization
 */
struct TileAttribute {
    const char* key;      ///< Attribute key (PROGMEM string, e.g., "type", "solid")
    const char* value;    ///< Attribute value (PROGMEM string, e.g., "door", "true")
};

/**
 * @brief All attributes for a single tile at a specific position.
 *
 * TileAttributeEntry associates a tile position (x, y) with its metadata attributes.
 * Only tiles that have attributes are included in the exported data (sparse
 * representation), minimizing memory usage for large tilemaps.
 *
 * Attribute Resolution:
 * - Editor merges tileset default attributes with instance overrides
 * - Only final resolved attributes are exported (no inheritance logic at runtime)
 * - Empty tiles (no attributes) are not included in the exported data
 *
 * Position Encoding:
 * - X and Y are tile coordinates (not pixel coordinates)
 * - Coordinates are relative to the layer's origin (0, 0)
 * - Maximum coordinate value: 65535 (uint16_t range)
 *
 * Query Pattern:
 * ```cpp
 * // Find tile at position (10, 5)
 * for (uint16_t i = 0; i < layer.num_tiles_with_attributes; i++) {
 *     if (layer.tiles[i].x == 10 && layer.tiles[i].y == 5) {
 *         // Found tile, search attributes
 *         for (uint8_t j = 0; j < layer.tiles[i].num_attributes; j++) {
 *             if (strcmp_P(layer.tiles[i].attributes[j].key, "solid") == 0) {
 *                 // Found "solid" attribute
 *             }
 *         }
 *     }
 * }
 * ```
 *
 * @note Attributes array is stored in PROGMEM (flash memory)
 * @note Maximum 255 attributes per tile (uint8_t limit)
 * @note Use helper functions like get_tile_attribute() for easier queries
 *
 * @see TileAttribute for individual key-value pairs
 * @see LayerAttributes for layer-level organization
 */
struct TileAttributeEntry {
    uint16_t x;                           ///< Tile X coordinate in layer space
    uint16_t y;                           ///< Tile Y coordinate in layer space
    uint8_t num_attributes;               ///< Number of attributes for this tile
    const TileAttribute* attributes;      ///< PROGMEM array of attribute key-value pairs
};

/**
 * @brief All tiles with attributes in a single tilemap layer.
 *
 * LayerAttributes organizes all tile metadata for a single layer, providing
 * efficient lookup of attributes by tile position. Only tiles with non-empty
 * attributes are included, using a sparse representation to minimize memory.
 *
 * Layer Organization:
 * - Each layer in a scene has its own LayerAttributes structure
 * - Layers are typically organized as: Background, Midground, Foreground, etc.
 * - Layer name matches the name defined in the Tilemap Editor
 *
 * Memory Efficiency:
 * - Sparse representation: only tiles with attributes are stored
 * - All data stored in PROGMEM (flash memory) on ESP32
 * - No RAM overhead for attribute storage
 * - Typical size: ~40 bytes per tile with attributes (depends on key/value lengths)
 *
 * Query Workflow:
 * 1. Identify layer by index or name
 * 2. Search tiles array for matching (x, y) position
 * 3. If found, iterate through tile's attributes array
 * 4. Compare keys using strcmp_P() for PROGMEM strings
 *
 * Example Usage:
 * ```cpp
 * // Query attribute for tile at (10, 5) in layer 0
 * const char* value = get_tile_attribute(0, 10, 5, "solid");
 * if (value && strcmp_P(value, "true") == 0) {
 *     // Tile is solid
 * }
 * ```
 *
 * @note Layer name is a PROGMEM string (use strcmp_P() for comparison)
 * @note Tiles array is sorted by position for potential binary search optimization
 * @note Maximum 65535 tiles with attributes per layer (uint16_t limit)
 *
 * @see TileAttributeEntry for individual tile attributes
 * @see TileAttribute for key-value pairs
 * @see Generated scene headers for query helper functions
 */
struct LayerAttributes {
    const char* layer_name;               ///< Layer name (PROGMEM string, e.g., "Background")
    uint16_t num_tiles_with_attributes;   ///< Number of tiles with attributes in this layer
    const TileAttributeEntry* tiles;      ///< PROGMEM array of tiles with attributes (sparse)
};

/**
 * @brief Query a tile attribute value by position and key.
 * 
 * Searches the layer attributes for a tile at the specified position and
 * returns the value associated with the given key. All data is read from
 * PROGMEM (flash memory) to minimize RAM usage.
 * 
 * This function performs O(n) linear search through tiles and attributes.
 * For querying multiple attributes from the same tile, consider caching
 * the tile lookup or using a more optimized approach.
 * 
 * @param layer_attributes Pointer to PROGMEM array of LayerAttributes
 * @param num_layers Number of layers in the array
 * @param layer_idx Index of the layer to query (0-based)
 * @param x Tile X coordinate
 * @param y Tile Y coordinate
 * @param key Attribute key to search for (may be RAM or PROGMEM string)
 * @return Pointer to PROGMEM attribute value string, or nullptr if not found
 * 
 * @note Returns nullptr if:
 *       - layer_idx >= num_layers (out of bounds)
 *       - Layer is empty (num_tiles_with_attributes == 0)
 *       - Tile at (x, y) does not exist
 *       - Tile exists but does not have the specified key
 * 
 * @note The returned pointer references PROGMEM. Use strcmp_P() or similar
 *       functions to compare values.
 * 
 * Example usage:
 * ```cpp
 * // Query "solid" attribute for tile at (10, 5) in layer 0
 * const char* value = pixelroot32::graphics::get_tile_attribute(
 *     layer_attributes, NUM_LAYERS_WITH_ATTRIBUTES, 0, 10, 5, "solid"
 * );
 * if (value && strcmp_P(value, "true") == 0) {
 *     // Tile is solid
 * }
 * ```
 * 
 * @see tile_has_attributes() for checking if a tile has any attributes
 * @see LayerAttributes for layer-level organization
 * @see TileAttributeEntry for tile position association
 * @see TileAttribute for individual key-value pairs
 */
inline const char* get_tile_attribute(
    const LayerAttributes* layer_attributes,
    uint8_t num_layers,
    uint8_t layer_idx,
    uint16_t x,
    uint16_t y,
    const char* key
) {
    // Check if layer exists
    if (layer_idx >= num_layers) {
        return nullptr;
    }
    
    // Get layer from PROGMEM
    LayerAttributes layer;
    PIXELROOT32_MEMCPY_P(&layer, &layer_attributes[layer_idx], sizeof(LayerAttributes));
    
    // Search for tile at position (x, y)
    for (uint16_t i = 0; i < layer.num_tiles_with_attributes; i++) {
        TileAttributeEntry tile;
        PIXELROOT32_MEMCPY_P(&tile, &layer.tiles[i], sizeof(TileAttributeEntry));
        
        if (tile.x == x && tile.y == y) {
            // Found tile, search for attribute key
            for (uint8_t j = 0; j < tile.num_attributes; j++) {
                TileAttribute attr;
                PIXELROOT32_MEMCPY_P(&attr, &tile.attributes[j], sizeof(TileAttribute));
                
                // Compare keys using PROGMEM-safe function
                if (PIXELROOT32_STRCMP_P(key, attr.key) == 0) {
                    return attr.value;
                }
            }
            break;  // Tile found but key not present
        }
    }
    
    return nullptr;  // Tile not found or attribute not present
}

/**
 * @brief Check if a tile at the given position has any attributes.
 * 
 * This function searches for a tile at the specified coordinates in the given
 * layer and returns true if the tile exists (has any attributes), false otherwise.
 * All data is read from PROGMEM (flash memory) to minimize RAM usage.
 * 
 * This function performs O(n) linear search through tiles. It is more efficient
 * than calling get_tile_attribute() when you only need to check for tile existence
 * without querying specific attribute values.
 * 
 * @param layer_attributes Pointer to PROGMEM array of LayerAttributes
 * @param num_layers Number of layers in the array
 * @param layer_idx Index of the layer to query (0-based)
 * @param x Tile X coordinate
 * @param y Tile Y coordinate
 * @return true if tile has attributes, false otherwise
 * 
 * @note Returns false if:
 *       - layer_idx >= num_layers (out of bounds)
 *       - Layer is empty (num_tiles_with_attributes == 0)
 *       - Tile at (x, y) does not exist in the layer
 * 
 * @note This function only checks for tile existence. To query specific
 *       attribute values, use get_tile_attribute().
 * 
 * Example usage:
 * ```cpp
 * // Check if tile at (10, 5) in layer 0 has any attributes
 * if (pixelroot32::graphics::tile_has_attributes(
 *     layer_attributes, NUM_LAYERS_WITH_ATTRIBUTES, 0, 10, 5
 * )) {
 *     // Tile has attributes, query specific values
 *     const char* solid = pixelroot32::graphics::get_tile_attribute(
 *         layer_attributes, NUM_LAYERS_WITH_ATTRIBUTES, 0, 10, 5, "solid"
 *     );
 * }
 * ```
 * 
 * @see get_tile_attribute() for querying specific attribute values
 * @see LayerAttributes for layer-level organization
 * @see TileAttributeEntry for tile position association
 */
inline bool tile_has_attributes(
    const LayerAttributes* layer_attributes,
    uint8_t num_layers,
    uint8_t layer_idx,
    uint16_t x,
    uint16_t y
) {
    // Check if layer exists
    if (layer_idx >= num_layers) {
        return false;
    }
    
    // Get layer from PROGMEM
    LayerAttributes layer;
    PIXELROOT32_MEMCPY_P(&layer, &layer_attributes[layer_idx], sizeof(LayerAttributes));
    
    // Search for tile at position (x, y)
    for (uint16_t i = 0; i < layer.num_tiles_with_attributes; i++) {
        TileAttributeEntry tile;
        PIXELROOT32_MEMCPY_P(&tile, &layer.tiles[i], sizeof(TileAttributeEntry));
        
        if (tile.x == x && tile.y == y) {
            return true;  // Tile found
        }
    }
    
    return false;  // Tile not found
}

/**
 * @brief Get the TileAttributeEntry for a tile at the given position.
 * 
 * This function is optimized for cases where multiple attributes need to be
 * queried from the same tile. Instead of searching for each attribute
 * individually, get the entry once and iterate through its attributes.
 * 
 * This function performs O(n) linear search through tiles to find the matching
 * position. Once found, it returns a pointer to the TileAttributeEntry in
 * PROGMEM, allowing the caller to read and iterate through all attributes
 * efficiently.
 * 
 * @param layer_attributes Pointer to PROGMEM array of LayerAttributes
 * @param num_layers Number of layers in the array
 * @param layer_idx Index of the layer to query (0-based)
 * @param x Tile X coordinate
 * @param y Tile Y coordinate
 * @return Pointer to TileAttributeEntry in PROGMEM, or nullptr if not found
 * 
 * @note Returns nullptr if:
 *       - layer_idx >= num_layers (out of bounds)
 *       - Layer is empty (num_tiles_with_attributes == 0)
 *       - Tile at (x, y) does not exist in the layer
 * 
 * @note The returned pointer references PROGMEM. Use PIXELROOT32_MEMCPY_P
 *       to read the entry into RAM before accessing its fields.
 * 
 * @note This function is more efficient than calling get_tile_attribute()
 *       multiple times for the same tile, as it performs the position lookup
 *       only once.
 * 
 * Example usage:
 * ```cpp
 * // Query multiple attributes from tile at (10, 5) in layer 0
 * const TileAttributeEntry* entry_ptr = pixelroot32::graphics::get_tile_entry(
 *     layer_attributes, NUM_LAYERS_WITH_ATTRIBUTES, 0, 10, 5
 * );
 * 
 * if (entry_ptr) {
 *     // Read entry from PROGMEM into RAM
 *     TileAttributeEntry entry;
 *     PIXELROOT32_MEMCPY_P(&entry, entry_ptr, sizeof(TileAttributeEntry));
 *     
 *     // Iterate through all attributes
 *     for (uint8_t i = 0; i < entry.num_attributes; i++) {
 *         TileAttribute attr;
 *         PIXELROOT32_MEMCPY_P(&attr, &entry.attributes[i], sizeof(TileAttribute));
 *         
 *         // Process attribute key and value
 *         // attr.key and attr.value are PROGMEM pointers
 *         // Use strcmp_P() or similar functions to compare
 *     }
 * }
 * ```
 * 
 * @see get_tile_attribute() for querying a single attribute value
 * @see tile_has_attributes() for checking tile existence
 * @see LayerAttributes for layer-level organization
 * @see TileAttributeEntry for tile attribute structure
 */
inline const TileAttributeEntry* get_tile_entry(
    const LayerAttributes* layer_attributes,
    uint8_t num_layers,
    uint8_t layer_idx,
    uint16_t x,
    uint16_t y
) {
    // Check if layer exists
    if (layer_idx >= num_layers) {
        return nullptr;
    }
    
    // Get layer from PROGMEM
    LayerAttributes layer;
    PIXELROOT32_MEMCPY_P(&layer, &layer_attributes[layer_idx], sizeof(LayerAttributes));
    
    // Search for tile at position (x, y)
    for (uint16_t i = 0; i < layer.num_tiles_with_attributes; i++) {
        TileAttributeEntry tile;
        PIXELROOT32_MEMCPY_P(&tile, &layer.tiles[i], sizeof(TileAttributeEntry));
        
        if (tile.x == x && tile.y == y) {
            // Return pointer to entry in PROGMEM (not the copied tile)
            return &layer.tiles[i];
        }
    }
    
    return nullptr;  // Tile not found
}

/**
 * @brief Single animation frame that can reference either a Sprite or a MultiSprite.
 *
 * Exactly one of the pointers is expected to be non-null for a valid frame.
 * This allows the same animation system to drive both simple and layered
 * sprites without exposing bit-level details to game code.
 */
struct SpriteAnimationFrame {
    const Sprite*      sprite;      ///< Optional pointer to a simple 1bpp sprite frame.
    const MultiSprite* multiSprite; ///< Optional pointer to a layered sprite frame.
};

/**
 * @brief Lightweight, step-based sprite animation controller.
 *
 * SpriteAnimation owns no memory. It references a compile-time array of
 * SpriteAnimationFrame entries and exposes simple integer-based control:
 *
 * - step(): advance to the next frame (wrapping at frameCount)
 * - reset(): go back to frame 0
 * - getCurrentSprite()/getCurrentMultiSprite(): query current frame data
 *
 * The animation object never draws anything; Actors remain responsible for
 * asking which frame to render and calling Renderer accordingly.
 *
 * Initially this struct is used for "step-based" animation (advance once per
 * logical event, such as a horde movement). The design can be extended later
 * with time-based advancement without changing Renderer.
 */
struct SpriteAnimation {
    const SpriteAnimationFrame* frames;    ///< Pointer to immutable frame table.
    uint8_t                     frameCount;///< Number of frames in the table.
    uint8_t                     current;   ///< Current frame index [0, frameCount).

    /// Reset the animation to the first frame.
    void reset() {
        current = 0;
    }

    /// Advance to the next frame in a loop (step-based advancement).
    void step() {
        if (!frames || frameCount == 0) {
            return;
        }
        ++current;
        if (current >= frameCount) {
            current = 0;
        }
    }

    /// Get the current frame descriptor (may contain either type of sprite).
    const SpriteAnimationFrame& getCurrentFrame() const {
        return frames[current];
    }

    /// Convenience helper: returns the current simple Sprite, if any.
    const Sprite* getCurrentSprite() const {
        if (!frames || frameCount == 0) {
            return nullptr;
        }
        return frames[current].sprite;
    }

    /// Convenience helper: returns the current MultiSprite, if any.
    const MultiSprite* getCurrentMultiSprite() const {
        if (!frames || frameCount == 0) {
            return nullptr;
        }
        return frames[current].multiSprite;
    }
};

/**
 * @class Renderer
 * @brief High-level graphics rendering system.
 *
 * The Renderer class provides a unified API for drawing shapes, text, and images.
 * It abstracts the underlying hardware implementation (DrawSurface) and manages
 * display configuration, including rotation and offsets.
 */
class Renderer {
public:
    /**
     * @brief Constructs the Renderer with a specific display configuration.
     * @param config The display configuration settings.
     */
    Renderer(const DisplayConfig& config);
    Renderer(DisplayConfig&& config);
    Renderer(Renderer&& other) noexcept
        : drawer(std::move(other.drawer)),
          config(std::move(other.config)),
          logicalWidth(other.logicalWidth),
          logicalHeight(other.logicalHeight),
          xOffset(other.xOffset),
          yOffset(other.yOffset),
          offsetBypass(other.offsetBypass),
          currentRenderContext(other.currentRenderContext)
    {}

    Renderer& operator=(Renderer&& other) noexcept {
        if (this != &other) {
            config = std::move(other.config);
            drawer = std::move(other.drawer);
            logicalWidth = other.logicalWidth;
            logicalHeight = other.logicalHeight;
            xOffset = other.xOffset;
            yOffset = other.yOffset;
            offsetBypass = other.offsetBypass;
            currentRenderContext = other.currentRenderContext;
        }
        return *this;
    }
    ~Renderer() = default;

    /**
     * @brief Initializes the renderer and the underlying draw surface.
     */
    void init();

    /**
     * @brief Prepares the buffer for a new frame (clears screen).
     */
    void beginFrame();

    /**
     * @brief Finalizes the frame and sends the buffer to the display.
     */
    void endFrame();

    /**
     * @brief Gets the underlying DrawSurface implementation.
     * @return Reference to the DrawSurface.
     */
    DrawSurface& getDrawSurface() { return *drawer; }

    /**
     * @brief Draws a string of text using the default font.
     * @param text The text to draw.
     * @param x X coordinate.
     * @param y Y coordinate.
     * @param color Text color.
     * @param size Text size multiplier.
     */
    void drawText(std::string_view text, int16_t x, int16_t y, Color color, uint8_t size);

    /**
     * @brief Draws a string of text using a specific font.
     * @param text The text to draw.
     * @param x X coordinate.
     * @param y Y coordinate.
     * @param color Text color.
     * @param size Text size multiplier.
     * @param font Pointer to the font to use. If nullptr, uses the default font.
     */
    void drawText(std::string_view text, int16_t x, int16_t y, Color color, uint8_t size, const Font* font);

    /**
     * @brief Draws text centered horizontally at a given Y coordinate using the default font.
     * @param text The text to draw.
     * @param y Y coordinate.
     * @param color Text color.
     * @param size Text size.
     */
    void drawTextCentered(std::string_view text, int16_t y, Color color, uint8_t size);

    /**
     * @brief Draws text centered horizontally at a given Y coordinate using a specific font.
     * @param text The text to draw.
     * @param y Y coordinate.
     * @param color Text color.
     * @param size Text size.
     * @param font Pointer to the font to use. If nullptr, uses the default font.
     */
    void drawTextCentered(std::string_view text, int16_t y, Color color, uint8_t size, const Font* font);

    /**
     * @brief Draws a filled circle.
     * @param x Center X coordinate.
     * @param y Center Y coordinate.
     * @param radius Radius of the circle.
     * @param color Fill color.
     */
    void drawFilledCircle(int x, int y, int radius, Color color);   

    /**
     * @brief Draws a circle outline.
     * @param x Center X coordinate.
     * @param y Center Y coordinate.
     * @param radius Radius of the circle.
     * @param color Outline color.
     */
    void drawCircle(int x, int y, int radius, Color color);
    
    /**
     * @brief Draws a rectangle outline.
     * @param x Top-left X coordinate.
     * @param y Top-left Y coordinate.
     * @param width Width of the rectangle.
     * @param height Height of the rectangle.
     * @param color Outline color.
     */
    void drawRectangle(int x, int y, int width, int height, Color color);

    /**
     * @brief Draws a filled rectangle.
     * @param x Top-left X coordinate.
     * @param y Top-left Y coordinate.
     * @param width Width of the rectangle.
     * @param height Height of the rectangle.
     * @param color Fill color.
     */
    void drawFilledRectangle(int x, int y, int width, int height, Color color);

    /**
     * @brief Draws a filled rectangle with a 16-bit color.
     * @param x Top-left X coordinate.
     * @param y Top-left Y coordinate.
     * @param width Width of the rectangle.
     * @param height Height of the rectangle.
     * @param color Fill color (RGB565).
     */
    void drawFilledRectangleW(int x, int y, int width, int height, uint16_t color);

    /**
     * @brief Draws a line between two points.
     * @param x1 Start X.
     * @param y1 Start Y.
     * @param x2 End X.
     * @param y2 End Y.
     * @param color Line color.
     */
    void drawLine(int x1, int y1, int x2, int y2, Color color); 

    /**
     * @brief Draws a bitmap image.
     * @param x Top-left X coordinate.
     * @param y Top-left Y coordinate.
     * @param width Width of the bitmap.
     * @param height Height of the bitmap.
     * @param bitmap Pointer to the bitmap data.
     * @param color Color to draw the bitmap pixels (if monochrome) or ignored.
     */
    void drawBitmap(int x, int y, int width, int height, const uint8_t *bitmap, Color color);

    /**
     * @brief Draws a single pixel.
     * @param x X coordinate.
     * @param y Y coordinate.
     * @param color Pixel color.
     */
    void drawPixel(int x, int y, Color color);

    /**
     * @brief Sets the logical display size (rendering resolution).
     * @param w Logical width.
     * @param h Logical height.
     */
    void setDisplaySize(int w, int h) {
        logicalWidth = w;
        logicalHeight = h;
    }

    /// @brief Gets the logical rendering width.
    int getLogicalWidth() const { return logicalWidth; }
    
    /// @brief Gets the logical rendering height.
    int getLogicalHeight() const { return logicalHeight; }

    /**
     * @brief Sets a global offset for all drawing operations.
     * @param x X offset.
     * @param y Y offset.
     */
    void setDisplayOffset(int x, int y) {
        xOffset = x;
        yOffset = y;
    }

    /**
     * @brief Sets the display contrast (brightness).
     * @param level Contrast level (0-255).
     */
    void setContrast(uint8_t level) {
        getDrawSurface().setContrast(level);
    }

    /**
     * @brief Sets the font for text rendering.
     * @param font Pointer to the font data.
     */
    void setFont(const uint8_t* font);

    int getXOffset() const { return xOffset; }
    int getYOffset() const { return yOffset; }

    /**
     * @brief Sets the render context for palette selection.
     * 
     * This method allows the renderer to use the appropriate palette based on
     * the current render layer. When set, primitives will use this context
     * instead of their default (Sprite).
     * 
     * @param context The palette context to use (Background or Sprite).
     *                 Pass nullptr to use method-specific defaults.
     */
    void setRenderContext(PaletteContext* context) {
        currentRenderContext = context;
    }

    /**
     * @brief Gets the current render context.
     * @return Pointer to the current context, or nullptr if using defaults.
     */
    PaletteContext* getRenderContext() const {
        return currentRenderContext;
    }

    /**
     * @brief Draws a 1bpp monochrome sprite using the Sprite descriptor.
     *
     * Sprite data is interpreted bit-by-bit using the Sprite convention:
     * bit 0 = leftmost pixel, bit (width - 1) = rightmost pixel.
     *
     * This API intentionally hides all bit-level details from game code.
     *
     * @param sprite Sprite descriptor (data, width, height).
     * @param x      Top-left X coordinate in logical screen space.
     * @param y      Top-left Y coordinate in logical screen space.
     * @param color  Color used for "on" pixels.
     * @param flipX  If true, sprite is mirrored horizontally.
     */
    void drawSprite(const Sprite& sprite, int x, int y, Color color, bool flipX = false);

    /**
     * @brief Draws a scaled 1bpp monochrome sprite.
     *
     * Similar to drawSprite but applies nearest-neighbor scaling.
     * The destination size is calculated as ceil(width * scaleX) x ceil(height * scaleY).
     *
     * @param sprite Sprite descriptor.
     * @param x      Top-left X coordinate.
     * @param y      Top-left Y coordinate.
     * @param scaleX Horizontal scaling factor (e.g., 1.25).
     * @param scaleY Vertical scaling factor (e.g., 1.25).
     * @param color  Color used for "on" pixels.
     * @param flipX  If true, sprite is mirrored horizontally before scaling.
     */
    void drawSprite(const Sprite& sprite, int x, int y, float scaleX, float scaleY, Color color, bool flipX = false);

    void drawSprite(const Sprite2bpp& sprite, int x, int y, bool flipX = false);

    void drawSprite(const Sprite4bpp& sprite, int x, int y, bool flipX = false);

    /**
     * @brief Draws a multi-layer sprite composed of several 1bpp layers.
     *
     * Each layer is rendered in array order using the existing drawSprite()
     * implementation, avoiding duplicated bit iteration logic.
     *
     * @param sprite Multi-layer sprite descriptor.
     * @param x      Top-left X coordinate in logical screen space.
     * @param y      Top-left Y coordinate in logical screen space.
     */
    void drawMultiSprite(const MultiSprite& sprite, int x, int y);

    /**
     * @brief Draws a scaled multi-layer sprite.
     *
     * Reuses the scaled drawSprite implementation for each layer.
     *
     * @param sprite Multi-layer sprite descriptor.
     * @param x      Top-left X coordinate.
     * @param y      Top-left Y coordinate.
     * @param scaleX Horizontal scaling factor.
     * @param scaleY Vertical scaling factor.
     */
    void drawMultiSprite(const MultiSprite& sprite, int x, int y, float scaleX, float scaleY);

    /**
     * @brief Draws a tilemap of 1bpp sprites.
     */
    void drawTileMap(const TileMap& map, int originX, int originY, Color color = Color::White);

    /**
     * @brief Draws a tilemap of 2bpp sprites.
     */
    void drawTileMap(const TileMap2bpp& map, int originX, int originY);

    /**
     * @brief Draws a tilemap of 4bpp sprites.
     */
    void drawTileMap(const TileMap4bpp& map, int originX, int originY);

    /**
     * @brief Enables or disables ignoring global offsets for subsequent draw calls.
     * 
     * When bypass is enabled, xOffset and yOffset are ignored, and drawing
     * occurs at absolute logical screen coordinates.
     * 
     * @param bypass True to ignore offsets, false to apply them (default).
     */
    void setOffsetBypass(bool bypass) {
        offsetBypass = bypass;
    }

    /**
     * @brief Checks if offset bypass is currently enabled.
     * @return True if offsets are being ignored.
     */
    bool isOffsetBypassEnabled() const {
        return offsetBypass;
    }

private:
    std::unique_ptr<DrawSurface> drawer; ///< Pointer to the platform-specific implementation.

    DisplayConfig config;

    int logicalWidth = 240;  ///< Logical rendering width (used for clipping)
    int logicalHeight = 240; ///< Logical rendering height (used for clipping)

    int xOffset = 0;
    int yOffset = 0;

    bool offsetBypass = false; ///< When true, xOffset and yOffset are ignored

    PaletteContext* currentRenderContext = nullptr; ///< Current render context for palette selection (nullptr = use method defaults)

    void drawSpriteInternal(const Sprite2bpp& sprite, int x, int y, const uint16_t* paletteLUT, bool flipX);
    void drawSpriteInternal(const Sprite4bpp& sprite, int x, int y, const uint16_t* paletteLUT, bool flipX);
};

}
