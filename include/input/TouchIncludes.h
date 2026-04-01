/*
 * PixelRoot32 Game Engine
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 *
 * TouchIncludes.h - Unified include header for touch subsystem
 * Include this single header to get all touch-related declarations
 */
#pragma once

// Core data structures - use TouchPoint.h for raw touch data
#include "input/TouchPoint.h"        // TouchPoint, TOUCH_MAX_POINTS

// Touch Event System (Phase 3) - use these as the canonical sources
#include "input/TouchEventTypes.h"   // TouchEventType enum (TouchDown, TouchUp, Click, etc.)
#include "input/TouchEvent.h"       // TouchEvent struct (12 bytes packed, with x, y directly)
#include "input/TouchEventQueue.h"   // TouchEventQueue ring buffer

// State machine for gesture detection
#include "input/TouchStateMachine.h" // TouchStateMachine

// Event dispatcher (pull-based API)
#include "input/TouchEventDispatcher.h"

// Calibration with factory presets
#include "input/TouchAdapter.h"     // TouchCalibration, DisplayPreset, TouchRotation

// NOTE: TouchPointBuffer is for raw touch points (different from TouchEventQueue)
// Only include if needed for raw point buffering
// #include "input/TouchPointBuffer.h"  

// Manager and factory
#include "input/TouchManager.h"     // TouchManager
#include "input/TouchFactory.h"    // TouchFactory

// Hardware adapters (if enabled)
#if defined(TOUCH_DRIVER_XPT2046)
    #include "input/adapters/XPT2046Adapter.h"
#elif defined(TOUCH_DRIVER_GT911)
    #include "input/adapters/GT911Adapter.h"
#endif

// Configuration
#if __has_include("graphics/TouchConfig.h")
    #include "graphics/TouchConfig.h"
#endif
