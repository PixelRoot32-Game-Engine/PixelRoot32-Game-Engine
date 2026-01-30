/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#pragma once

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

// Enable a discrete debug overlay with FPS, RAM and CPU metrics.
// Replaces the old PIXELROOT32_ENABLE_FPS_DISPLAY.
#define PIXELROOT32_ENABLE_DEBUG_OVERLAY
