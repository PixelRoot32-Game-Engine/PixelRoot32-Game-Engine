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
 * @enum TileCollisionBehavior
 * @brief Defines how a tile collider behaves in the physics system.
 */
enum class TileCollisionBehavior : uint8_t {
    SOLID       = 0,  ///< Normal collision with full physical response (walls, blocks).
    SENSOR      = 1,  ///< Trigger only: events, no impulse or penetration (collectibles, checkpoints).
    ONE_WAY_UP  = 2,  ///< Platform: only blocks from above (player can jump through from below).
    DAMAGE      = 3,  ///< Sensor that typically causes damage (e.g. lava, spikes).
    DESTRUCTIBLE = 4  ///< Solid that can be destroyed on hit (e.g. breakable blocks).
};

/**
 * @brief Pack tile coordinates and behavior into a single value for userData.
 * Encoding: bits [0-9]=x, [10-19]=y, [20-23]=behavior. Max 1024x1024 tiles.
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
 * @brief Unpack tile data from userData.
 */
inline void unpackTileData(uintptr_t packed, uint16_t& x, uint16_t& y, TileCollisionBehavior& behavior) {
    x = static_cast<uint16_t>(packed & 0x3FF);
    y = static_cast<uint16_t>((packed >> 10) & 0x3FF);
    behavior = static_cast<TileCollisionBehavior>((packed >> 20) & 0xF);
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
    y = static_cast<uint16_t>(packed >> 16);
}

} // namespace pixelroot32::physics
