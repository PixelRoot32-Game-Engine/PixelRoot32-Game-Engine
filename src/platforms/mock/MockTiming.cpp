/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 * 
 * Mock timing provider implementation
 */

#include "platforms/mock/MockTiming.h"

namespace pixelroot32::platforms::mock {

// Global mock timing instance (nullptr by default)
MockTimingProvider* g_mockTiming = nullptr;

} // namespace pixelroot32::platforms::mock
