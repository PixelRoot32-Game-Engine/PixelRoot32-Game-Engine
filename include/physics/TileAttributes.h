/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 *
 * Tile collision behaviors and helpers for encoding tile metadata in PhysicsActor::userData.
 * Used by tilemap collision builders and game logic to drive collision response.
 */
#pragma once
#include <cstdint>
#include <cstddef>

namespace pixelroot32::physics {

/**
 * @enum TileFlags
 * @brief Bit flags for tile behavior attributes (8-bit, 1 byte per tile).
 * Optimized for ESP32 runtime with bit operations only.
 */
enum TileFlags : uint8_t {
    TILE_NONE        = 0,        ///< No behavior flags set
    TILE_SOLID       = 1u << 0,   ///< Solid collision (walls, blocks)
    TILE_SENSOR      = 1u << 1,   ///< Sensor-only collision (triggers, collectibles)
    TILE_DAMAGE      = 1u << 2,   ///< Damage on contact (lava, spikes)
    TILE_COLLECTIBLE = 1u << 3,   ///< Can be collected/removed (coins, items)
    TILE_ONEWAY      = 1u << 4,   ///< One-way platform (blocks from above only)
    TILE_TRIGGER     = 1u << 5,   ///< Trigger event on contact
    // Reserved bits 6-7 for future use
};

/**
 * @enum TileCollisionBehavior
 * @brief Defines how a tile collider behaves in the physics system.
 * @deprecated Use TileFlags for new implementations. Kept for backward compatibility.
 */
enum class TileCollisionBehavior : uint8_t {
    SOLID       = 0,  ///< Normal collision with full physical response (walls, blocks).
    SENSOR      = 1,  ///< Trigger only: events, no impulse or penetration (collectibles, checkpoints).
    ONE_WAY_UP  = 2,  ///< Platform: only blocks from above (player can jump through from below).
    DAMAGE      = 3,  ///< Sensor that typically causes damage (e.g. lava, spikes).
    DESTRUCTIBLE = 4  ///< Solid that can be destroyed on hit (e.g. breakable blocks).
};

/**
 * @brief Pack tile coordinates and TileFlags into a single value for userData.
 * Encoding: bits [0-9]=x, [10-19]=y, [20-27]=flags. Max 1024x1024 tiles.
 * @param x Tile X (0..1023).
 * @param y Tile Y (0..1023).
 * @param flags TileFlags combination.
 * @return Packed value to store via setUserData(reinterpret_cast<void*>(packed)).
 */
inline uintptr_t packTileData(uint16_t x, uint16_t y, TileFlags flags) {
    return (static_cast<uintptr_t>(y) << 10) | static_cast<uintptr_t>(x)
         | (static_cast<uintptr_t>(flags) << 20);
}

/**
 * @brief Unpack tile data from userData with TileFlags.
 * @param packed Packed value from userData.
 * @param x Output tile X coordinate.
 * @param y Output tile Y coordinate.
 * @param flags Output TileFlags combination.
 */
inline void unpackTileData(uintptr_t packed, uint16_t& x, uint16_t& y, TileFlags& flags) {
    x = static_cast<uint16_t>(packed & 0x3FF);
    y = static_cast<uint16_t>((packed >> 10) & 0x3FF);
    flags = static_cast<TileFlags>((packed >> 20) & 0xFF);
}

/**
 * @brief Pack tile coordinates and TileCollisionBehavior into a single value for userData.
 * Encoding: bits [0-9]=x, [10-19]=y, [20-23]=behavior. Max 1024x1024 tiles.
 * @deprecated Use packTileData(x, y, TileFlags) for new implementations.
 * @param x Tile X (0..1023).
 * @param y Tile Y (0..1023).
 * @param behavior TileCollisionBehavior.
 * @return Packed value to store via setUserData(reinterpret_cast<void*>(packed)).
 */
inline uintptr_t packTileData(uint16_t x, uint16_t y, TileCollisionBehavior behavior) {
    return (static_cast<uintptr_t>(y) << 10) | static_cast<uintptr_t>(x)
         | (static_cast<uintptr_t>(static_cast<uint8_t>(behavior)) << 20);
}

/**
 * @brief Unpack tile data from userData with TileCollisionBehavior.
 * @deprecated Use unpackTileData(packed, x, y, TileFlags) for new implementations.
 * @param packed Packed value from userData.
 * @param x Output tile X coordinate.
 * @param y Output tile Y coordinate.
 * @param behavior Output TileCollisionBehavior.
 */
inline void unpackTileData(uintptr_t packed, uint16_t& x, uint16_t& y, TileCollisionBehavior& behavior) {
    x = static_cast<uint16_t>(packed & 0x3FF);
    y = static_cast<uint16_t>((packed >> 10) & 0x3FF);
    behavior = static_cast<TileCollisionBehavior>((packed >> 20) & 0xF);
}

/**
 * @brief Helper to derive sensor flag from TileFlags combination.
 * @param flags TileFlags combination.
 * @return true if tile should be configured as sensor.
 */
inline bool isSensorTile(TileFlags flags) {
    return (flags & (TILE_SENSOR | TILE_DAMAGE | TILE_COLLECTIBLE | TILE_TRIGGER)) != 0;
}

/**
 * @brief Helper to derive one-way flag from TileFlags combination.
 * @param flags TileFlags combination.
 * @return true if tile should be configured as one-way platform.
 */
inline bool isOneWayTile(TileFlags flags) {
    return (flags & TILE_ONEWAY) != 0;
}

/**
 * @brief Helper to derive solid flag from TileFlags combination.
 * @param flags TileFlags combination.
 * @return true if tile should be configured as solid body.
 */
inline bool isSolidTile(TileFlags flags) {
    return (flags & TILE_SOLID) != 0;
}

/**
 * @brief Legacy: pack only coordinates (16+16 bits). Compatible with existing userData usage.
 * Use when TileCollisionBehavior is not needed (max 65535x65535 tiles).
 */
inline uintptr_t packCoord(uint16_t x, uint16_t y) {
    return (static_cast<uintptr_t>(y) << 16) | x;
}

/**
 * @brief Legacy: unpack coordinates from legacy encoding.
 */
inline void unpackCoord(uintptr_t packed, uint16_t& x, uint16_t& y) {
    x = static_cast<uint16_t>(packed & 0xFFFF);
    y = static_cast<uintptr_t>(packed >> 16);
}

/**
 * @struct TileBehaviorLayer
 * @brief Runtime representation of exported behavior layer for O(1) flag lookup.
 * 
 * This structure matches the format exported by the Tilemap Editor
 * and provides efficient access to tile behavior flags without runtime strings.
 */
struct TileBehaviorLayer {
    const uint8_t* data;  ///< Pointer to dense uint8_t array (1 byte per tile)
    uint16_t width;       ///< Layer width in tiles
    uint16_t height;      ///< Layer height in tiles
};

/**
 * @brief Get TileFlags for a specific tile position in a behavior layer.
 * 
 * Provides O(1) lookup with bounds checking for safe access.
 * Returns TILE_NONE (0) for out-of-bounds coordinates.
 * 
 * @param layer Behavior layer structure containing the data
 * @param x X coordinate in tiles
 * @param y Y coordinate in tiles
 * @return TileFlags combination (0 = TILE_NONE if out of bounds)
 */
inline uint8_t getTileFlags(const TileBehaviorLayer& layer, int x, int y) {
    if (x < 0 || x >= (int)layer.width || y < 0 || y >= (int)layer.height) {
        return 0; // TILE_NONE
    }
    return layer.data[y * layer.width + x];
}

} // namespace pixelroot32::physics
