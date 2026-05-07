#ifdef PLATFORM_NATIVE
#include "platforms/native.h"
#elif defined(PLATFORM_ESP32C3)
#include "platforms/esp32_c3.h"
#else
#include "platforms/esp32_dev.h"
#endif