/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 *
 * @file EngineModules.h
 * @brief Translation of feature flags to constexpr booleans.
 */
#pragma once
#include "platforms/EngineConfig.h"

/**
 * @namespace pixelroot32::modules
 * @brief Namespace containing constexpr booleans for compile-time module checks.
 */
namespace pixelroot32::modules {
    // Translation of feature flags to constexpr booleans for use with `if constexpr`
    constexpr bool Audio     = PIXELROOT32_ENABLE_AUDIO;
    constexpr bool Physics   = PIXELROOT32_ENABLE_PHYSICS;
    constexpr bool UI        = PIXELROOT32_ENABLE_UI_SYSTEM;
    constexpr bool Particles = PIXELROOT32_ENABLE_PARTICLES;

    // Optional ESP-NOW peer-to-peer networking module; off by default so games
    // that never opt in keep the Wi-Fi stack out of their image.
    constexpr bool Networking = PIXELROOT32_ENABLE_NETWORKING;

    // Optional settings/save-slot persistence module; off by default so games
    // that never store anything link none of it.
    constexpr bool Persistence = PIXELROOT32_ENABLE_PERSISTENCE;

    // Assertions in compile-time (Examples)
    // static_assert(!UI || Renderer, "UI Module needs Renderer");
}