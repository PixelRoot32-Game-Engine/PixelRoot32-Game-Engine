/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */

#ifdef UNIT_TEST
#include "core/Engine.h"
#include "graphics/DisplayConfig.h"

/**
 * @file MockEngineInstance.cpp
 * @brief Provides a global Engine instance for unit tests.
 * 
 * Many parts of the engine (like ParticleEmitter) expect a global 'engine' 
 * object to be available. This file provides that instance during tests.
 */

namespace {
    // Static config to avoid global namespace pollution
    pixelroot32::graphics::DisplayConfig test_dConfig(pixelroot32::graphics::DisplayType::NONE);
}

// Global engine instance required by the engine core
pixelroot32::core::Engine engine(test_dConfig);

#endif
