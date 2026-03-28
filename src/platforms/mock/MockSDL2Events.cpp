/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 * 
 * Mock SDL2 events implementation
 */

#include "platforms/mock/MockSDL2Events.h"

namespace pixelroot32::platforms::mock {

// Global mock SDL2 event provider (nullptr by default)
MockSDL2EventProvider* g_mockSDL2Events = nullptr;

} // namespace pixelroot32::platforms::mock
