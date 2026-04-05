/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#pragma once

#include <cstdint>
#include <cstddef>
#include "../platforms/EngineConfig.h"

// =============================================================================
// Platform-specific attributes
// =============================================================================
#ifndef IRAM_ATTR
#define IRAM_ATTR __attribute__((always_inline))
#endif

namespace pixelroot32::graphics {

// Tile size: 8x8 pixels at 16bpp = 128 bytes
static constexpr size_t TILE_CACHE_TILE_WIDTH = 8;
static constexpr size_t TILE_CACHE_TILE_HEIGHT = 8;
static constexpr size_t TILE_CACHE_TILE_BYTES = TILE_CACHE_TILE_WIDTH * TILE_CACHE_TILE_HEIGHT * sizeof(uint16_t);

/**
 * @class TileCache
 * @brief LRU cache for pre-rendered tile bitmaps.
 * 
 * Stores pre-rendered tiles in RAM to avoid pixel-by-pixel rendering
 * on every frame. When the cache is full, evicts least recently used tiles.
 * 
 * Memory: PIXELROOT32_TILE_CACHE_SIZE * 128 bytes (default: 2KB for 16 tiles)
 * 
 * Usage:
 *   TileCache cache;
 *   cache.init();
 *   
 *   // On tile render request:
 *   uint16_t* cached = cache.get(tileIndex, paletteHash);
 *   if (cached) {
 *       // Draw cached tile directly
 *   } else {
 *       // Render and cache
 *       cache.put(tileIndex, paletteHash, renderedData);
 *   }
 */
class TileCache {
public:
    struct CacheEntry {
        uint8_t tileIndex;      // Tile index in tileset
        uint16_t paletteHash;   // Hash of palette combination
        uint16_t* data;        // Pre-rendered tile data (or nullptr if empty)
        uint8_t accessCount;   // LRU counter (higher = more recently used)
    };

    TileCache()
        : entries_(nullptr)
        , capacity_(0)
        , accessCounter_(0) {
    }

    ~TileCache() {
        if (entries_) {
            // Free cached tile data
            for (size_t i = 0; i < capacity_; i++) {
                if (entries_[i].data) {
                    delete[] entries_[i].data;
                }
            }
            delete[] entries_;
        }
    }

    /**
     * @brief Initialize the cache with specified capacity
     * @param capacity Number of tiles to cache (default: PIXELROOT32_TILE_CACHE_SIZE)
     * @return true if initialization successful
     */
    bool init(size_t capacity = PIXELROOT32_TILE_CACHE_SIZE) {
        if (entries_) {
            return true;  // Already initialized
        }
        
        capacity_ = capacity;
        entries_ = new CacheEntry[capacity_];
        if (!entries_) {
            capacity_ = 0;
            return false;
        }
        
        // Initialize entries
        for (size_t i = 0; i < capacity_; i++) {
            entries_[i].tileIndex = 0xFF;
            entries_[i].paletteHash = 0;
            entries_[i].data = nullptr;
            entries_[i].accessCount = 0;
        }
        
        accessCounter_ = 0;
        return true;
    }

    /**
     * @brief Get a cached tile
     * @param tileIndex Tile index in tileset
     * @param paletteHash Hash of palette combination
     * @return Pointer to cached data, or nullptr if not found
     */
    IRAM_ATTR uint16_t* get(uint8_t tileIndex, uint16_t paletteHash) {
        if (!entries_) return nullptr;
        
        for (size_t i = 0; i < capacity_; i++) {
            if (entries_[i].tileIndex == tileIndex && 
                entries_[i].paletteHash == paletteHash &&
                entries_[i].data != nullptr) {
                // Update LRU
                entries_[i].accessCount = ++accessCounter_;
                return entries_[i].data;
            }
        }
        return nullptr;
    }

    /**
     * @brief Put a rendered tile into cache
     * @param tileIndex Tile index in tileset
     * @param paletteHash Hash of palette combination
     * @param data Pre-rendered tile data (caller transfers ownership)
     * @return true if successfully cached
     */
    bool put(uint8_t tileIndex, uint16_t paletteHash, uint16_t* data) {
        if (!entries_ || !data) return false;
        
        // Find empty slot or evict LRU
        size_t evictIndex = 0;
        uint8_t minAccess = 0xFF;
        
        for (size_t i = 0; i < capacity_; i++) {
            if (entries_[i].data == nullptr) {
                evictIndex = i;
                break;
            }
            if (entries_[i].accessCount < minAccess) {
                minAccess = entries_[i].accessCount;
                evictIndex = i;
            }
        }
        
        // Evict if needed
        if (entries_[evictIndex].data != nullptr) {
            delete[] entries_[evictIndex].data;
        }
        
        // Store new entry
        entries_[evictIndex].tileIndex = tileIndex;
        entries_[evictIndex].paletteHash = paletteHash;
        entries_[evictIndex].data = data;
        entries_[evictIndex].accessCount = ++accessCounter_;
        
        return true;
    }

    /**
     * @brief Invalidate all cached tiles for a given tile index
     * Call when tile animation changes
     * @param tileIndex Tile index to invalidate
     */
    void invalidate(uint8_t tileIndex) {
        if (!entries_) return;
        
        for (size_t i = 0; i < capacity_; i++) {
            if (entries_[i].tileIndex == tileIndex && entries_[i].data != nullptr) {
                delete[] entries_[i].data;
                entries_[i].data = nullptr;
                entries_[i].tileIndex = 0xFF;
            }
        }
    }

    /**
     * @brief Invalidate entire cache
     * Call when any animation changes
     */
    void invalidateAll() {
        if (!entries_) return;
        
        for (size_t i = 0; i < capacity_; i++) {
            if (entries_[i].data != nullptr) {
                delete[] entries_[i].data;
                entries_[i].data = nullptr;
                entries_[i].tileIndex = 0xFF;
            }
        }
    }

    /**
     * @brief Check if cache is initialized
     * @return true if cache is ready
     */
    bool isInitialized() const {
        return entries_ != nullptr;
    }

    /**
     * @brief Get cache capacity
     * @return Number of tiles that can be cached
     */
    size_t capacity() const {
        return capacity_;
    }

private:
    CacheEntry* entries_;
    size_t capacity_;
    uint8_t accessCounter_;
};

} // namespace pixelroot32::graphics
