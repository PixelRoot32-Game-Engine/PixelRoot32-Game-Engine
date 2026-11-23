/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#include "platforms/PlatformSelect.h"

#if !PIXELROOT32_HAS_ARDUINO
#include "platforms/mock/MockSPI.h"
SPIClass SPI;
#endif // !PIXELROOT32_HAS_ARDUINO  
