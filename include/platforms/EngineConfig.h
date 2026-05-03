/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#pragma once

#include "platforms/PlatformDefaults.h"
#include "core/Log.h"

/**
 * @file EngineConfig.h
 * @brief Archivo maestro de configuración en tiempo de compilación para PixelRoot32.
 * 
 * Define límites de hardware, banderas de depuración, resolución de pantalla
 * y capacidades del motor. La mayoría de los valores pueden ser sobrescritos
 * inyectando macros desde el sistema de construcción.
 */

// =============================================================================
// Logging (must be defined before including Log.h)
// =============================================================================
/**
 * @def PIXELROOT32_DEBUG_MODE
 * @brief Enables engine logging.
 * 
 * When defined, log() calls print to the platform output. 
 * If not defined, log() is a no-op.
 */
// #define PIXELROOT32_DEBUG_MODE

#ifdef PLATFORM_NATIVE
    #include "platforms/mock/MockArduino.h"
#else
    #include <Arduino.h>
#endif

// =============================================================================
// Physical Display Configuration (Hardware)
// =============================================================================
/** @brief Actual hardware display width resolution. */
#ifndef PHYSICAL_DISPLAY_WIDTH
#define PHYSICAL_DISPLAY_WIDTH  240
#endif

/** @brief Actual hardware display height resolution. */
#ifndef PHYSICAL_DISPLAY_HEIGHT
#define PHYSICAL_DISPLAY_HEIGHT 240
#endif

// =============================================================================
// Logical Resolution Configuration (Rendering)
// =============================================================================
/**
 * @def LOGICAL_WIDTH
 * @brief Internal rendering logic width.
 * 
 * The engine will scale from logical to physical resolution automatically.
 * Recommended presets for ESP32:
 * - 128x128 (Best performance, 72% memory savings)
 * - 160x160 (Balanced, 44% memory savings)
 * - 240x240 (Full resolution)
 */
#ifndef LOGICAL_WIDTH
#define LOGICAL_WIDTH  PHYSICAL_DISPLAY_WIDTH
#endif

/** @brief Internal rendering logic height. */
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
/** @brief Deprecated. Use LOGICAL_WIDTH/HEIGHT. */
#define DISPLAY_WIDTH  LOGICAL_WIDTH
#define DISPLAY_HEIGHT LOGICAL_HEIGHT

// =============================================================================
// Input Systems
// =============================================================================

/**
 * @def PIXELROOT32_ENABLE_TOUCH
 * @brief Enable touch input system.
 * 
 * Engine processes touch events automatically and sends them to the current scene.
 */
#ifndef PIXELROOT32_ENABLE_TOUCH
#define PIXELROOT32_ENABLE_TOUCH 0
#endif

// =============================================================================
// Debug & Profiling
// =============================================================================
/** @brief Enable performance profiling in Serial monitor. */
// #define PIXELROOT32_ENABLE_PROFILING
// #define PIXELROOT32_ENABLE_DIRTY_REGION_PROFILING 

#ifndef PIXELROOT32_PROFILE_BEGIN
#define PIXELROOT32_PROFILE_BEGIN(name) (void)0
#endif
#ifndef PIXELROOT32_PROFILE_END
#define PIXELROOT32_PROFILE_END(name) (void)0
#endif

/** @brief Enable a discrete debug overlay with FPS, RAM and CPU metrics. */
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

/** @brief Compile-time toggle for dirty-cell regions (`DirtyGrid`): selective framebuffer clear and dynamic tilemap marking.
 *  Define to `1` before including EngineConfig / build with `-D PIXELROOT32_ENABLE_DIRTY_REGIONS=1`.
 *  When `0` (default): `Renderer::beginFrame` always performs a full clear; no dirty grid allocation in `Renderer::init`;
 *  `pixelroot32::platforms::config::EnableDirtyRegions` is `false`.
 */
#ifndef PIXELROOT32_ENABLE_DIRTY_REGIONS
#define PIXELROOT32_ENABLE_DIRTY_REGIONS 0
#endif

/** @brief When `1` and `PIXELROOT32_DEBUG_MODE` is defined, `Renderer::beginFrame` may log dirty-region stats (serial cost). */
#ifndef PIXELROOT32_ENABLE_DIRTY_REGION_PROFILING
#define PIXELROOT32_ENABLE_DIRTY_REGION_PROFILING 0
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

#ifndef PHYSICS_MAX_ENTITIES
    #define PHYSICS_MAX_ENTITIES 64
#endif

#ifndef PHYSICS_MAX_CONTACTS
    #define PHYSICS_MAX_CONTACTS 128
#endif

#ifndef PHYSICS_MAX_PAIRS
    #define PHYSICS_MAX_PAIRS 128
#endif

#ifndef PIXELROOT32_VELOCITY_ITERATIONS
    #define PIXELROOT32_VELOCITY_ITERATIONS 2
#endif

// Deprecated alias for backward compatibility
#define PHYSICS_RELAXATION_ITERATIONS PIXELROOT32_VELOCITY_ITERATIONS

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

/**
 * @namespace pixelroot32::platforms::config
 * @brief Type-safe exposure of engine configuration macros.
 * 
 * Contains constexpr versions of the engine limits and features,
 * recommended for use in C++ code instead of direct macros.
 */
namespace pixelroot32::platforms::config {
    // ... existing configs ...

    // Hardware Capabilities
    #if defined(PR32_HAS_FPU_MACRO)

    /** @brief Type-safe access to HasFPU configuration. */
    inline constexpr bool HasFPU = true;
    #else

    /** @brief Type-safe access to HasFPU configuration. */
    inline constexpr bool HasFPU = false;
    #endif

    // Physical Display

    /** @brief Type-safe access to PhysicalDisplayWidth configuration. */
    inline constexpr int PhysicalDisplayWidth = PHYSICAL_DISPLAY_WIDTH;

    /** @brief Type-safe access to PhysicalDisplayHeight configuration. */
    inline constexpr int PhysicalDisplayHeight = PHYSICAL_DISPLAY_HEIGHT;

    // Logical Display

    /** @brief Type-safe access to LogicalWidth configuration. */
    inline constexpr int LogicalWidth = LOGICAL_WIDTH;

    /** @brief Type-safe access to LogicalHeight configuration. */
    inline constexpr int LogicalHeight = LOGICAL_HEIGHT;

    /** @brief Type-safe access to DisplayRotation configuration. */
    inline constexpr int DisplayRotation = DISPLAY_ROTATION;

    /** @brief Type-safe access to XOffset configuration. */
    inline constexpr int XOffset = X_OFF_SET;

    /** @brief Type-safe access to YOffset configuration. */
    inline constexpr int YOffset = Y_OFF_SET;

    // Tile Animation

    /** @brief Type-safe access to MaxTilesetSize configuration. */
    inline constexpr uint16_t MaxTilesetSize = MAX_TILESET_SIZE;

    // Scene Limits

    /** @brief Type-safe access to MaxScenes configuration. */
    inline constexpr int MaxScenes = MAX_SCENES;

    /** @brief Type-safe access to MaxLayers configuration. */
    inline constexpr int MaxLayers = MAX_LAYERS;

    /** @brief Type-safe access to MaxEntities configuration. */
    inline constexpr int MaxEntities = MAX_ENTITIES;

    /** @brief Type-safe access to kMaxBackgroundPaletteSlots configuration. */
    inline constexpr int kMaxBackgroundPaletteSlots = MAX_BACKGROUND_PALETTE_SLOTS;

    /** @brief Type-safe access to kMaxSpritePaletteSlots configuration. */
    inline constexpr int kMaxSpritePaletteSlots = MAX_SPRITE_PALETTE_SLOTS;

    // Spatial Grid

    /** @brief Type-safe access to SpatialGridCellSize configuration. */
    inline constexpr int SpatialGridCellSize = SPATIAL_GRID_CELL_SIZE;

    /** @brief Type-safe access to SpatialGridMaxEntitiesPerCell configuration. */
    inline constexpr int SpatialGridMaxEntitiesPerCell = SPATIAL_GRID_MAX_ENTITIES_PER_CELL;

    /** @brief Type-safe access to SpatialGridMaxStaticPerCell configuration. */
    inline constexpr int SpatialGridMaxStaticPerCell = SPATIAL_GRID_MAX_STATIC_PER_CELL;

    /** @brief Type-safe access to SpatialGridMaxDynamicPerCell configuration. */
    inline constexpr int SpatialGridMaxDynamicPerCell = SPATIAL_GRID_MAX_DYNAMIC_PER_CELL;

    // Physics

    /** @brief Type-safe access to PhysicsMaxEntities configuration. */
    inline constexpr int PhysicsMaxEntities = PHYSICS_MAX_ENTITIES;

    /** @brief Type-safe access to PhysicsMaxContacts configuration. */
    inline constexpr int PhysicsMaxContacts = PHYSICS_MAX_CONTACTS;

    /** @brief Type-safe access to PhysicsMaxPairs configuration. */
    inline constexpr int PhysicsMaxPairs = PHYSICS_MAX_PAIRS;

    /** @brief Type-safe access to VelocityIterations configuration. */
    inline constexpr int VelocityIterations = PIXELROOT32_VELOCITY_ITERATIONS;
    
    // Deprecated for backward compatibility

    /** @brief Type-safe access to PhysicsRelaxationIterations configuration. */
    inline constexpr int PhysicsRelaxationIterations = VelocityIterations;

    // Profiling & Debug
    #ifdef PIXELROOT32_ENABLE_PROFILING

    /** @brief Type-safe access to EnableProfiling configuration. */
    inline constexpr bool EnableProfiling = true;
    #else

    /** @brief Type-safe access to EnableProfiling configuration. */
    inline constexpr bool EnableProfiling = false;
    #endif

    #if PIXELROOT32_ENABLE_DIRTY_REGION_PROFILING && defined(PIXELROOT32_DEBUG_MODE)

    /** @brief Type-safe access to EnableDirtyRegionProfiling configuration. */
    inline constexpr bool EnableDirtyRegionProfiling = true;
    #else

    /** @brief Type-safe access to EnableDirtyRegionProfiling configuration. */
    inline constexpr bool EnableDirtyRegionProfiling = false;
    #endif

    #ifdef PIXELROOT32_ENABLE_DEBUG_OVERLAY

    /** @brief Type-safe access to EnableDebugOverlay configuration. */
    inline constexpr bool EnableDebugOverlay = true;
    #else

    /** @brief Type-safe access to EnableDebugOverlay configuration. */
    inline constexpr bool EnableDebugOverlay = false;
    #endif

    #ifdef PIXELROOT32_DEBUG_MODE

    /** @brief Type-safe access to EnableLogging configuration. */
    inline constexpr bool EnableLogging = true;
    #else

    /** @brief Type-safe access to EnableLogging configuration. */
    inline constexpr bool EnableLogging = false;
    #endif

    // Sprites
    #ifdef PIXELROOT32_ENABLE_2BPP_SPRITES

    /** @brief Type-safe access to Enable2BppSprites configuration. */
    inline constexpr bool Enable2BppSprites = true;
    #else

    /** @brief Type-safe access to Enable2BppSprites configuration. */
    inline constexpr bool Enable2BppSprites = false;
    #endif

    #ifdef PIXELROOT32_ENABLE_4BPP_SPRITES

    /** @brief Type-safe access to Enable4BppSprites configuration. */
    inline constexpr bool Enable4BppSprites = true;
    #else

    /** @brief Type-safe access to Enable4BppSprites configuration. */
    inline constexpr bool Enable4BppSprites = false;
    #endif

    #ifdef PIXELROOT32_ENABLE_SCENE_ARENA

    /** @brief Type-safe access to EnableSceneArena configuration. */
    inline constexpr bool EnableSceneArena = true;
    #else

    /** @brief Type-safe access to EnableSceneArena configuration. */
    inline constexpr bool EnableSceneArena = false;
    #endif

    #ifndef PIXELROOT32_ENABLE_TILE_ANIMATIONS

    /** @brief Type-safe access to EnableTileAnimations configuration. */
    inline constexpr bool EnableTileAnimations = false;
    #else

    /** @brief Type-safe access to EnableTileAnimations configuration. */
    inline constexpr bool EnableTileAnimations = true;
    #endif

    #if PIXELROOT32_ENABLE_STATIC_TILEMAP_FB_CACHE

    /** @brief Type-safe access to EnableStaticTilemapFbCache configuration. */
    inline constexpr bool EnableStaticTilemapFbCache = true;
    #else

    /** @brief Type-safe access to EnableStaticTilemapFbCache configuration. */
    inline constexpr bool EnableStaticTilemapFbCache = false;
    #endif

    #if PIXELROOT32_ENABLE_DIRTY_REGIONS

    /** @brief Type-safe access to EnableDirtyRegions configuration. */
    inline constexpr bool EnableDirtyRegions = true;
    #else

    /** @brief Type-safe access to EnableDirtyRegions configuration. */
    inline constexpr bool EnableDirtyRegions = false;
    #endif

    inline unsigned long profilerMicros() {
        return micros();
    }
}
