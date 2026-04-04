/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */

#pragma once
#include <cstdint>
#include "../platforms/EngineConfig.h"

namespace pixelroot32::graphics {

/**
 * @brief Tracks which tile indices have changed and need re-rendering.
 * 
 * Uses a bitmap for memory efficiency (1 bit per tile). When a tile
 * animation changes frame, the tile is marked dirty. The renderer can
 * check this and only re-render dirty tiles, skipping unchanged ones.
 * 
 * Memory: (PIXELROOT32_DIRTY_TRACKER_SIZE + 7) / 8 bytes (default: 32 bytes)
 * 
 * Usage:
 *   DirtyTileTracker dirty;
 *   dirty.init();
 *   
 *   // Mark tile as dirty (call when animation frame changes)
 *   dirty.markDirty(42);
 *   
 *   // Check if tile needs re-rendering
 *   if (dirty.isDirty(42)) {
 *       // Re-render and cache tile
 *       dirty.clearDirty(42);
 *   }
 */
class DirtyTileTracker {
public:
    DirtyTileTracker() : bitmap_(nullptr), size_(0) {}

    ~DirtyTileTracker() {
        if (bitmap_) {
            delete[] bitmap_;
        }
    }

    /**
     * @brief Initialize the dirty tracker
     * @param size Number of tiles to track (default: PIXELROOT32_DIRTY_TRACKER_SIZE)
     * @return true if initialization successful
     */
    bool init(size_t size = PIXELROOT32_DIRTY_TRACKER_SIZE) {
        if (bitmap_) {
            return true;  // Already initialized
        }
        
        size_ = size;
        size_t bytes = (size_ + 7) / 8;
        bitmap_ = new uint8_t[bytes];
        if (!bitmap_) {
            size_ = 0;
            return false;
        }
        
        // Clear all dirty flags
        memset(bitmap_, 0, bytes);
        return true;
    }

    /**
     * @brief Mark a tile as dirty (needs re-rendering)
     * @param tileIndex Tile index to mark
     */
    void markDirty(uint8_t tileIndex) {
        if (!bitmap_ || tileIndex >= size_) return;
        
        size_t byteIdx = tileIndex >> 3;
        uint8_t bitIdx = tileIndex & 7;
        bitmap_[byteIdx] |= (1 << bitIdx);
    }

    /**
     * @brief Check if a tile is dirty
     * @param tileIndex Tile index to check
     * @return true if tile is dirty and needs re-rendering
     */
    bool isDirty(uint8_t tileIndex) const {
        if (!bitmap_ || tileIndex >= size_) return false;
        
        size_t byteIdx = tileIndex >> 3;
        uint8_t bitIdx = tileIndex & 7;
        return (bitmap_[byteIdx] & (1 << bitIdx)) != 0;
    }

    /**
     * @brief Clear dirty flag for a tile
     * @param tileIndex Tile index to clear
     */
    void clearDirty(uint8_t tileIndex) {
        if (!bitmap_ || tileIndex >= size_) return;
        
        size_t byteIdx = tileIndex >> 3;
        uint8_t bitIdx = tileIndex & 7;
        bitmap_[byteIdx] &= ~(1 << bitIdx);
    }

    /**
     * @brief Clear all dirty flags
     */
    void clearAll() {
        if (!bitmap_) return;
        
        size_t bytes = (size_ + 7) / 8;
        memset(bitmap_, 0, bytes);
    }

    /**
     * @brief Check if tracker is initialized
     * @return true if ready to use
     */
    bool isInitialized() const {
        return bitmap_ != nullptr;
    }

    /**
     * @brief Get tracker size
     * @return Number of tiles tracked
     */
    size_t size() const {
        return size_;
    }

private:
    uint8_t* bitmap_;
    size_t size_;
};

/**
 * @brief Single tile animation definition (compile-time constant).
 * 
 * Defines a sequence of tile indices that form an animation loop.
 * All data stored in PROGMEM/flash to minimize RAM usage.
 * 
 * Memory Layout: 4 bytes total (POD structure)
 * - baseTileIndex: First tile in animation sequence (0-255)
 * - frameCount: Number of frames in animation (1-255) 
 * - frameDuration: Ticks to display each frame (1-255)
 * - reserved: Padding for alignment (future use)
 * 
 * Example: Water animation with 4 frames, 8 ticks per frame
 * { baseTileIndex: 42, frameCount: 4, frameDuration: 8, reserved: 0 }
 * 
 * @note This structure is stored in PROGMEM (flash memory)
 * @note sizeof(TileAnimation) == 4 bytes
 */
struct TileAnimation {
    uint8_t baseTileIndex;   ///< First tile in sequence (e.g., 42)
    uint8_t frameCount;      ///< Number of frames (e.g., 4)
    uint8_t frameDuration;   ///< Ticks per frame (e.g., 8)
    uint8_t reserved;        ///< Padding for alignment/future use
};

/**
 * @brief Manages tile animations for a tilemap.
 * 
 * Provides O(1) frame resolution via lookup table. All animation
 * definitions stored in PROGMEM. Zero dynamic allocations.
 * 
 * CRITICAL: Uses fixed-size arrays (no new/delete) to comply with
 * PixelRoot32's strict "zero allocation" policy for ESP32.
 */
class TileAnimationManager {
public:
    /**
     * @brief Construct animation manager with compile-time animation table.
     * 
     * @param animations PROGMEM array of TileAnimation definitions
     * @param animCount Number of animations in the array
     * @param tileCount Number of tiles in tileset (from TileMapGeneric)
     */
    TileAnimationManager(const TileAnimation* animations, uint8_t animCount, uint16_t tileCount);

    /**
     * @brief Resolve tile index to current animated frame.
     * 
     * @param tileIndex Base tile index from tilemap
     * @return Current frame index (may be same as input if not animated)
     * 
     * PERFORMANCE: O(1) array lookup, IRAM-friendly, no branches in hot path
     */
    uint8_t resolveFrame(uint8_t tileIndex);
    
    /**
     * @brief Advance all animations by one step.
     * Call once per frame in Scene::update().
     * 
     * Complexity: O(animations × frameCount) - typically 4-32 operations
     */
    void step();
    
    /**
     * @brief Reset all animations to frame 0.
     */
    void reset();
    
private:
    const TileAnimation* animations;  ///< PROGMEM animation definitions
    uint8_t animCount;               ///< Number of animations
    uint16_t tileCount;               ///< Number of tiles in tileset (uint16_t para 512+)
    uint16_t globalFrameCounter;      ///< Global animation timer
    uint8_t lookupTable[MAX_TILESET_SIZE];  ///< tileIndex → currentFrame
};

} // namespace pixelroot32::graphics
