/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */

#pragma once
#include <cstdint>
#include "../platforms/EngineConfig.h"

namespace pixelroot32::graphics {

/**
 * @struct TileAnimation
 * @brief Single tile animation definition (compile-time constant).
 * 
 * Defines a sequence of tile indices that form an animation loop.
 * All data stored in PROGMEM/flash to minimize RAM usage.
 * 
 * Memory Layout: 4 bytes total (POD structure)
 * - baseTileIndex: First tile in animation sequence (0-255)
 * - frameCount: Number of frames in animation (1-255) 
 * - frameDuration: Hold each animation cell for this many **60 Hz ticks** (1-255).
 *   Logical tick rate is capped at 60/s (wall clock), independent of Engine loop speed.
 * - reserved: Padding for alignment (future use)
 * 
 * Example: Water animation with 4 frames, 8 ticks per frame (~133 ms per cell at 60 Hz)
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
 * @class TileAnimationManager
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
     * @brief Advance animations from elapsed wall time (60 Hz logical ticks max).
     * Uses high-resolution time between calls (micros); deltaTimeMs is a fallback when
     * the clock does not advance between calls (e.g. unit tests without real time).
     *
     * Complexity: O(animations × frameCount) when at least one tick elapses; else O(1)
     * @param deltaTimeMs Fallback elapsed time in milliseconds.
     */
    void step(unsigned long deltaTimeMs);
    
    /**
     * @brief Reset all animations to frame 0.
     */
    void reset();


    /**
     * @brief Fingerprint of the current resolved animation state (O(animCount)).
     * @return The visual signature of the current resolved animation state.
     *
     * Stable across `step(deltaTimeMs)` calls until a visible frame advances (same contract as
     * `resolveFrame` / lookup table). Used by scenes to skip draw+present when output
     * would be identical (e.g. ESP32 SPI budget).
     */
    uint32_t getVisualSignature() const;

    /**
     * @brief Checks if the resolved graphic for storedTileIndex changed since the snapshot taken at the
     * start of last step(); invalid indices return true so callers repaint conservatively.
     * @param storedTileIndex The tile index to check.
     * @return True if the resolved graphic for storedTileIndex changed since the snapshot taken at the
     * start of last step(); invalid indices return true so callers repaint conservatively.
     */
    bool animatedTileAppearanceChanged(uint8_t storedTileIndex) const;

private:
    void rebuildLookupTable();
    void snapshotLookupForDirtyCompare();

    const TileAnimation* animations;  ///< PROGMEM animation definitions
    uint8_t animCount;               ///< Number of animations
    uint16_t tileCount;               ///< Number of tiles in tileset (uint16_t para 512+)
    uint32_t globalFrameCounter;      ///< 60 Hz–paced animation time (ticks since start / reset)
    uint32_t tickAccumUs;             ///< Fraction of 1/60 s wall time in microseconds
    uint32_t lastStepMicros;          ///< Previous micros() sample for pacing
    uint8_t lookupTable[MAX_TILESET_SIZE];  ///< tileIndex → currentFrame
    uint8_t prevLookupSnapshot[MAX_TILESET_SIZE]; ///< copy at step() entry for dirty diff
};

} // namespace pixelroot32::graphics
