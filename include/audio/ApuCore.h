/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
/*
 * Transitional re-export shim: the APU core now lives in the shared
 * PixelRoot32-APU library (same pixelroot32::audio namespace). Existing
 * engine/game includes of "audio/ApuCore.h" keep working unchanged.
 * New code should include <pixelroot32/apu/ApuCore.h> directly.
 */
#pragma once

#include <pixelroot32/apu/ApuCore.h>
