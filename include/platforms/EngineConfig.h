/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#pragma once

#include "platforms/PlatformDefaults.h"
#include "core/Log.h"

// =============================================================================
// Logging (must be defined before including Log.h)
// =============================================================================
// Uncomment to enable engine logging; log() calls print to platform output.
// If not defined, log() is a no-op. Can also be set via build flags: -DPIXELROOT32_DEBUG_MODE
// #define PIXELROOT32_DEBUG_MODE

#ifdef PLATFORM_NATIVE
    #include "platforms/mock/MockArduino.h"
#else
    #include <Arduino.h>
#endif

// =============================================================================
// Physical Display Configuration (Hardware)
// =============================================================================
// These define the actual hardware display resolution.
// Override these before including this header if using a different display.

#ifndef PHYSICAL_DISPLAY_WIDTH
#define PHYSICAL_DISPLAY_WIDTH  240
#endif

#ifndef PHYSICAL_DISPLAY_HEIGHT
#define PHYSICAL_DISPLAY_HEIGHT 240
#endif

// =============================================================================
// Logical Resolution Configuration (Rendering)
// =============================================================================
// These define the resolution at which the game renders internally.
// The engine will scale from logical to physical resolution automatically.
// Set to 0 or omit to use physical resolution (no scaling).
//
// Recommended presets for ESP32:
//   128x128 - Best performance, 72% memory savings
//   160x160 - Balanced, 44% memory savings  
//   240x240 - Full resolution (no scaling)

#ifndef LOGICAL_WIDTH
#define LOGICAL_WIDTH  PHYSICAL_DISPLAY_WIDTH
#endif

#ifndef LOGICAL_HEIGHT
#define LOGICAL_HEIGHT PHYSICAL_DISPLAY_HEIGHT
#endif

// =============================================================================
// Display Settings
// =============================================================================
#ifndef DISPLAY_ROTATION
#define DISPLAY_ROTATION 0
#endif

#ifndef X_OFF_SET
#define X_OFF_SET 0
#endif

#ifndef Y_OFF_SET
#define Y_OFF_SET 0
#endif

// =============================================================================
// Deprecated Macros (for backward compatibility)
// =============================================================================
// These are kept for compatibility with existing code.
// New code should use LOGICAL_WIDTH/HEIGHT or PHYSICAL_DISPLAY_WIDTH/HEIGHT.
#define DISPLAY_WIDTH  LOGICAL_WIDTH
#define DISPLAY_HEIGHT LOGICAL_HEIGHT

// =============================================================================
// Debug & Profiling
// =============================================================================
// Uncomment to enable performance profiling in Serial monitor
// #define PIXELROOT32_ENABLE_PROFILING

#ifndef PIXELROOT32_PROFILE_BEGIN
#define PIXELROOT32_PROFILE_BEGIN(name) (void)0
#endif
#ifndef PIXELROOT32_PROFILE_END
#define PIXELROOT32_PROFILE_END(name) (void)0
#endif

// Enable a discrete debug overlay with FPS, RAM and CPU metrics.
// Replaces the old PIXELROOT32_ENABLE_FPS_DISPLAY.
// #define PIXELROOT32_ENABLE_DEBUG_OVERLAY

// =============================================================================
// Scene Limits
// =============================================================================
#ifndef MAX_SCENES
    #define MAX_SCENES 8
#endif

#ifndef MAX_LAYERS
    #define MAX_LAYERS 4
#endif
#ifndef MAX_ENTITIES
    #define MAX_ENTITIES 64
#endif

// =============================================================================
// Tile Animation Limits
// =============================================================================
#ifndef MAX_TILESET_SIZE
    #define MAX_TILESET_SIZE 256
#endif

// =============================================================================
// Palette Limits
// =============================================================================

/** Number of background palette slots for multi-palette tilemaps (2bpp/4bpp).
 *  Each tilemap cell can select a slot 0..(MAX_BACKGROUND_PALETTE_SLOTS-1) via paletteIndices.
 *  Override before including this header to change the slot count (e.g. -DMAX_BACKGROUND_PALETTE_SLOTS=4). */
#ifndef MAX_BACKGROUND_PALETTE_SLOTS
    #define MAX_BACKGROUND_PALETTE_SLOTS 8
#endif

/** Number of sprite palette slots for multi-palette sprites (2bpp/4bpp).
 *  Each sprite draw call can select a slot 0..(MAX_SPRITE_PALETTE_SLOTS-1) via paletteSlot parameter.
 *  Override before including this header to change the slot count (e.g. -DMAX_SPRITE_PALETTE_SLOTS=4). */
#ifndef MAX_SPRITE_PALETTE_SLOTS
    #define MAX_SPRITE_PALETTE_SLOTS 8
#endif

#ifndef SPATIAL_GRID_CELL_SIZE
    #define SPATIAL_GRID_CELL_SIZE 32
#endif

#ifndef SPATIAL_GRID_MAX_ENTITIES_PER_CELL
    #define SPATIAL_GRID_MAX_ENTITIES_PER_CELL 24
#endif

#ifndef SPATIAL_GRID_MAX_STATIC_PER_CELL
    #define SPATIAL_GRID_MAX_STATIC_PER_CELL 12
#endif

#ifndef SPATIAL_GRID_MAX_DYNAMIC_PER_CELL
    #define SPATIAL_GRID_MAX_DYNAMIC_PER_CELL 12
#endif

#ifndef PHYSICS_MAX_CONTACTS
    #define PHYSICS_MAX_CONTACTS 128
#endif

#ifndef PHYSICS_MAX_PAIRS
    #define PHYSICS_MAX_PAIRS 128
#endif

#ifndef PR32_VELOCITY_ITERATIONS
    #define PR32_VELOCITY_ITERATIONS 2
#endif

// Deprecated alias for backward compatibility
#define PHYSICS_RELAXATION_ITERATIONS PR32_VELOCITY_ITERATIONS

// =============================================================================
// Hardware Capabilities
// =============================================================================

#if defined(CONFIG_IDF_TARGET_ESP32) || defined(CONFIG_IDF_TARGET_ESP32S3) || defined(CONFIG_IDF_TARGET_ESP32P4)
    // ESP32 (Xtensa LX6), ESP32-S3 (Xtensa LX7), ESP32-P4 (RISC-V with FPU) have hardware FPU
    #define PR32_HAS_FPU_MACRO 1
#elif defined(CONFIG_IDF_TARGET_ESP32C3) || defined(CONFIG_IDF_TARGET_ESP32C2) || defined(CONFIG_IDF_TARGET_ESP32C6) || defined(CONFIG_IDF_TARGET_ESP32S2)
    // ESP32-C series (RISC-V) and S2 lack FPU
    #undef PR32_HAS_FPU_MACRO
#else
    // Default fallback (e.g. desktop/native build usually has FPU)
    #define PR32_HAS_FPU_MACRO 1
#endif

// Allow manual override for testing/benchmarking
#if defined(PR32_FORCE_FIXED)
    #undef PR32_HAS_FPU_MACRO
#endif

namespace pixelroot32::platforms::config {
    // ... existing configs ...

    // Hardware Capabilities
    #if defined(PR32_HAS_FPU_MACRO)
    inline constexpr bool HasFPU = true;
    #else
    inline constexpr bool HasFPU = false;
    #endif

    // Physical Display
    inline constexpr int PhysicalDisplayWidth = PHYSICAL_DISPLAY_WIDTH;
    inline constexpr int PhysicalDisplayHeight = PHYSICAL_DISPLAY_HEIGHT;

    // Logical Display
    inline constexpr int LogicalWidth = LOGICAL_WIDTH;
    inline constexpr int LogicalHeight = LOGICAL_HEIGHT;
    inline constexpr int DisplayRotation = DISPLAY_ROTATION;
    inline constexpr int XOffset = X_OFF_SET;
    inline constexpr int YOffset = Y_OFF_SET;

    // Scene Limits
    inline constexpr int MaxScenes = MAX_SCENES;
    inline constexpr int MaxLayers = MAX_LAYERS;
    inline constexpr int MaxEntities = MAX_ENTITIES;

    inline constexpr int kMaxBackgroundPaletteSlots = MAX_BACKGROUND_PALETTE_SLOTS;
    inline constexpr int kMaxSpritePaletteSlots = MAX_SPRITE_PALETTE_SLOTS;

    // Spatial Grid
    inline constexpr int SpatialGridCellSize = SPATIAL_GRID_CELL_SIZE;
    inline constexpr int SpatialGridMaxEntitiesPerCell = SPATIAL_GRID_MAX_ENTITIES_PER_CELL;
    inline constexpr int SpatialGridMaxStaticPerCell = SPATIAL_GRID_MAX_STATIC_PER_CELL;
    inline constexpr int SpatialGridMaxDynamicPerCell = SPATIAL_GRID_MAX_DYNAMIC_PER_CELL;

    // Physics
    inline constexpr int PhysicsMaxContacts = PHYSICS_MAX_CONTACTS;
    inline constexpr int PhysicsMaxPairs = PHYSICS_MAX_PAIRS;
    inline constexpr int VelocityIterations = PR32_VELOCITY_ITERATIONS;
    
    // Deprecated for backward compatibility
    inline constexpr int PhysicsRelaxationIterations = VelocityIterations;

    // Profiling & Debug
    #ifdef PIXELROOT32_ENABLE_PROFILING
    inline constexpr bool EnableProfiling = true;
    #else
    inline constexpr bool EnableProfiling = false;
    #endif

    #ifdef PIXELROOT32_ENABLE_DEBUG_OVERLAY
    inline constexpr bool EnableDebugOverlay = true;
    #else
    inline constexpr bool EnableDebugOverlay = false;
    #endif

    #ifdef PIXELROOT32_DEBUG_MODE
    inline constexpr bool EnableLogging = true;
    #else
    inline constexpr bool EnableLogging = false;
    #endif

    // Sprites
    #ifdef PIXELROOT32_ENABLE_2BPP_SPRITES
    inline constexpr bool Enable2BppSprites = true;
    #else
    inline constexpr bool Enable2BppSprites = false;
    #endif

    #ifdef PIXELROOT32_ENABLE_4BPP_SPRITES
    inline constexpr bool Enable4BppSprites = true;
    #else
    inline constexpr bool Enable4BppSprites = false;
    #endif

    #ifdef PIXELROOT32_ENABLE_SCENE_ARENA
    inline constexpr bool EnableSceneArena = true;
    #else
    inline constexpr bool EnableSceneArena = false;
    #endif

    inline unsigned long profilerMicros() {
        return micros();
    }
}
