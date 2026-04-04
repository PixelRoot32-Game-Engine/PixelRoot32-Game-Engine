/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#pragma once

#include <cstdint>
#include <cstddef>
#include "../platforms/EngineConfig.h"

namespace pixelroot32::graphics {

/**
 * @class ChunkManager
 * @brief Manages chunk-based viewport culling for tilemaps.
 * 
 * Divides tilemap into chunks and tracks which chunks are visible.
 * Only visible chunks are rendered, reducing draw calls significantly.
 * 
 * For a 30x30 tilemap with 8x8 chunk size:
 *   - 4x4 = 16 chunks total
 *   - If viewport shows only center, ~9 chunks visible
 *   - Reduces render area by ~75%
 * 
 * Usage:
 *   ChunkManager chunks;
 *   chunks.init(mapWidth, mapHeight, tileWidth, tileHeight);
 *   
 *   // In render loop:
 *   chunks.update(viewportX, viewportY, viewportW, viewportH);
 *   for (int chunkY = 0; chunkY < chunks.getChunkCountY(); chunkY++) {
 *       for (int chunkX = 0; chunkX < chunks.getChunkCountX(); chunkX++) {
 *           if (!chunks.isChunkVisible(chunkX, chunkY)) continue;
 *           // Render chunk
 *       }
 *   }
 */
class ChunkManager {
public:
    struct Chunk {
        int16_t startTileX;     // First tile X in chunk
        int16_t startTileY;    // First tile Y in chunk
        int16_t tileCountX;    // Number of tiles in X
        int16_t tileCountY;    // Number of tiles in Y
    };

    ChunkManager()
        : chunks_(nullptr)
        , chunkCountX_(0)
        , chunkCountY_(0)
        , visibleChunks_(nullptr)
        , visibleCount_(0)
        , mapWidth_(0)
        , mapHeight_(0)
        , tileWidth_(0)
        , tileHeight_(0) {
    }

    ~ChunkManager() {
        if (chunks_) delete[] chunks_;
        if (visibleChunks_) delete[] visibleChunks_;
    }

    /**
     * @brief Initialize chunk manager
     * @param mapWidth Width of tilemap in tiles
     * @param mapHeight Height of tilemap in tiles
     * @param tileWidth Width of each tile in pixels
     * @param tileHeight Height of each tile in pixels
     */
    void init(uint8_t mapWidth, uint8_t mapHeight, uint8_t tileWidth, uint8_t tileHeight) {
        mapWidth_ = mapWidth;
        mapHeight_ = mapHeight;
        tileWidth_ = tileWidth;
        tileHeight_ = tileHeight;
        
        // Calculate chunk grid size
        chunkCountX_ = (mapWidth + PIXELROOT32_CHUNK_SIZE - 1) / PIXELROOT32_CHUNK_SIZE;
        chunkCountY_ = (mapHeight + PIXELROOT32_CHUNK_SIZE - 1) / PIXELROOT32_CHUNK_SIZE;
        
        // Allocate chunks
        size_t chunkCount = chunkCountX_ * chunkCountY_;
        chunks_ = new Chunk[chunkCount];
        
        // Initialize chunk definitions
        for (uint8_t cy = 0; cy < chunkCountY_; cy++) {
            for (uint8_t cx = 0; cx < chunkCountX_; cx++) {
                size_t idx = cy * chunkCountX_ + cx;
                chunks_[idx].startTileX = cx * PIXELROOT32_CHUNK_SIZE;
                chunks_[idx].startTileY = cy * PIXELROOT32_CHUNK_SIZE;
                chunks_[idx].tileCountX = (cx == chunkCountX_ - 1) 
                    ? mapWidth_ - chunks_[idx].startTileX 
                    : PIXELROOT32_CHUNK_SIZE;
                chunks_[idx].tileCountY = (cy == chunkCountY_ - 1) 
                    ? mapHeight_ - chunks_[idx].startTileY 
                    : PIXELROOT32_CHUNK_SIZE;
            }
        }
        
        // Allocate visible chunk tracking
        visibleChunks_ = new bool[chunkCount];
        for (size_t i = 0; i < chunkCount; i++) {
            visibleChunks_[i] = false;
        }
        visibleCount_ = 0;
    }

    /**
     * @brief Update visible chunks based on viewport
     * @param viewportX Viewport X position in pixels
     * @param viewportY Viewport Y position in pixels
     * @param viewportW Viewport width in pixels
     * @param viewportH Viewport height in pixels
     * @return Number of visible chunks
     */
    size_t update(int viewportX, int viewportY, int viewportW, int viewportH) {
        visibleCount_ = 0;
        
        if (!chunks_ || !visibleChunks_) return 0;
        
        // Convert viewport to tile coordinates
        int viewTileStartX = viewportX / tileWidth_;
        int viewTileStartY = viewportY / tileHeight_;
        int viewTileEndX = (viewportX + viewportW + tileWidth_ - 1) / tileWidth_;
        int viewTileEndY = (viewportY + viewportH + tileHeight_ - 1) / tileHeight_;
        
        // Clamp to map bounds
        if (viewTileStartX < 0) viewTileStartX = 0;
        if (viewTileStartY < 0) viewTileStartY = 0;
        if (viewTileEndX > mapWidth_) viewTileEndX = mapWidth_;
        if (viewTileEndY > mapHeight_) viewTileEndY = mapHeight_;
        
        // Check each chunk
        for (uint8_t cy = 0; cy < chunkCountY_; cy++) {
            for (uint8_t cx = 0; cx < chunkCountX_; cx++) {
                size_t idx = cy * chunkCountX_ + cx;
                Chunk& chunk = chunks_[idx];
                
                // Check if chunk intersects viewport
                bool chunkVisible = !(chunk.startTileX >= viewTileEndX || 
                                   chunk.startTileX + chunk.tileCountX <= viewTileStartX ||
                                   chunk.startTileY >= viewTileEndY ||
                                   chunk.startTileY + chunk.tileCountY <= viewTileStartY);
                
                visibleChunks_[idx] = chunkVisible;
                if (chunkVisible) visibleCount_++;
            }
        }
        
        return visibleCount_;
    }

    /**
     * @brief Check if a specific chunk is visible
     * @param chunkX Chunk X index
     * @param chunkY Chunk Y index
     * @return true if chunk is visible
     */
    bool isChunkVisible(uint8_t chunkX, uint8_t chunkY) const {
        if (!visibleChunks_ || chunkX >= chunkCountX_ || chunkY >= chunkCountY_) {
            return false;
        }
        return visibleChunks_[chunkY * chunkCountX_ + chunkX];
    }

    /**
     * @brief Get chunk info
     * @param chunkX Chunk X index
     * @param chunkY Chunk Y index
     * @return Pointer to chunk info, or nullptr if invalid
     */
    const Chunk* getChunk(uint8_t chunkX, uint8_t chunkY) const {
        if (!chunks_ || chunkX >= chunkCountX_ || chunkY >= chunkCountY_) {
            return nullptr;
        }
        return &chunks_[chunkY * chunkCountX_ + chunkX];
    }

    /**
     * @brief Get number of chunks in X direction
     * @return Chunk count X
     */
    uint8_t getChunkCountX() const { return chunkCountX_; }

    /**
     * @brief Get number of chunks in Y direction
     * @return Chunk count Y
     */
    uint8_t getChunkCountY() const { return chunkCountY_; }

    /**
     * @brief Get total number of visible chunks
     * @return Visible chunk count (call update() first)
     */
    size_t getVisibleCount() const { return visibleCount_; }

    /**
     * @brief Check if initialized
     * @return true if ready to use
     */
    bool isInitialized() const { return chunks_ != nullptr; }

private:
    Chunk* chunks_;
    uint8_t chunkCountX_;
    uint8_t chunkCountY_;
    bool* visibleChunks_;
    size_t visibleCount_;
    uint8_t mapWidth_;
    uint8_t mapHeight_;
    uint8_t tileWidth_;
    uint8_t tileHeight_;
};

} // namespace pixelroot32::graphics
