/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#pragma once

/**
 * @file EngineConfig.h
 * @brief Forwarding header for backward compatibility.
 * @deprecated This file has moved to include/platforms/EngineConfig.h
 */

#include "platforms/EngineConfig.h"

#if defined(__GNUC__) || defined(__clang__)
#pragma message("EngineConfig.h has moved to include/platforms/. Please update your includes.")
#endif
