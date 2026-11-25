#include <platforms/PlatformSelect.h>

// One entry point per target, and the order matters: the web build defines
// PLATFORM_NATIVE as a legacy alias, so it has to be tested before the desktop
// one or it would compile the desktop entry (two engines, no IDBFS).
#if PIXELROOT32_PLATFORM_WEB
#include "platforms/web.h"
#elif PIXELROOT32_PLATFORM_ESP32
    #ifdef PLATFORM_ESP32CYD
    #include "platforms/esp32_cyd.h"
    #else
    #include "platforms/esp32_dev.h"
    #endif
#else
#include "platforms/native.h"
#endif
