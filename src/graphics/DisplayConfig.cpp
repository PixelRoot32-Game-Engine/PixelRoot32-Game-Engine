/*
 * Original work:
 * Copyright (c) nbourre
 * Licensed under the MIT License
 *
 * Modifications:
 * Copyright (c) 2026 PixelRoot32
 *
 * This file remains licensed under the MIT License.
 */
#include "graphics/DisplayConfig.h"
#include "graphics/DrawSurface.h"

#ifdef PLATFORM_NATIVE
    #include "drivers/native/SDL2_Drawer.h"
    #include <cassert>
#else
    #include "platforms/PlatformDefaults.h"
    #if defined(PIXELROOT32_USE_TFT_ESPI_DRIVER)
        #include "drivers/esp32/TFT_eSPI_Drawer.h"
    #elif defined(PIXELROOT32_USE_U8G2_DRIVER)
        #include "drivers/esp32/U8G2_Drawer.h"
    #endif
#endif

#ifdef TEST_MOCK_GRAPHICS
#include "graphics/BaseDrawSurface.h"
#include <cassert>
#endif

namespace pixelroot32::graphics {

    void DisplayConfig::initDrawSurface() {
        if (drawSurface) return; // Already initialized or custom set
        
        DrawSurface* rawSurface = nullptr;
        #ifdef TEST_MOCK_GRAPHICS
            class MockDrawer : public BaseDrawSurface {
            public:
                void init() override {}
                void clearBuffer() override {}
                void sendBuffer() override {}
                void drawPixel(int, int, uint16_t) override {}
                void drawBitmap(int, int, int, int, const uint8_t*, uint16_t) override {}
            };
            rawSurface = new MockDrawer();
        #elif defined(PLATFORM_NATIVE)
            rawSurface = new pixelroot32::drivers::native::SDL2_Drawer();
        #else
            #if defined(PIXELROOT32_USE_TFT_ESPI_DRIVER)
                switch (type)
                {
                case DisplayType::ST7789:
                case DisplayType::ST7735:
                default:
                    rawSurface = new pixelroot32::drivers::esp32::TFT_eSPI_Drawer();
                    break;
                }
            #elif defined(PIXELROOT32_USE_U8G2_DRIVER)
                U8G2* u8g2_instance = nullptr;
                const u8g2_cb_t* rot_cb = U8G2_R0;
                switch (rotation) {
                    case 1: case 90:  rot_cb = U8G2_R1; break;
                    case 2: case 180: rot_cb = U8G2_R2; break;
                    case 3: case 270: rot_cb = U8G2_R3; break;
                }

                switch (type)
                {
                case DisplayType::OLED_SSD1306:
                    if (useHardwareI2C) {
                        u8g2_instance = new U8G2_SSD1306_128X64_NONAME_F_HW_I2C(rot_cb, 
                            resetPin == 255 ? U8X8_PIN_NONE : resetPin, 
                            clockPin == 255 ? U8X8_PIN_NONE : clockPin, 
                            dataPin == 255 ? U8X8_PIN_NONE : dataPin);
                    } else {
                        u8g2_instance = new U8G2_SSD1306_128X64_NONAME_F_SW_I2C(rot_cb, 
                            clockPin, dataPin, 
                            resetPin == 255 ? U8X8_PIN_NONE : resetPin);
                    }
                    break;
                case DisplayType::OLED_SH1106:
                    u8g2_instance = new U8G2_SH1106_128X64_NONAME_F_HW_I2C(rot_cb, 
                        resetPin == 255 ? U8X8_PIN_NONE : resetPin, 
                        clockPin == 255 ? U8X8_PIN_NONE : clockPin, 
                        dataPin == 255 ? U8X8_PIN_NONE : dataPin);
                    break;
                default:
                    // If not specified or CUSTOM, we don't instantiate here
                    break;
                }
                
                if (u8g2_instance) {
                    rawSurface = new pixelroot32::drivers::esp32::U8G2_Drawer(u8g2_instance, true);
                }
            #endif
        #endif
        
        if (rawSurface != nullptr) {
            rawSurface->setDisplaySize(logicalWidth, logicalHeight);
            rawSurface->setPhysicalSize(physicalWidth, physicalHeight);
            rawSurface->setOffset(xOffset, yOffset);
            rawSurface->setRotation(rotation);
            drawSurface = std::unique_ptr<DrawSurface>(rawSurface);
        } else {
            #ifdef PLATFORM_NATIVE
                assert(false && "Failed to initialize Display Driver: No valid driver selected or supported for this platform.");
            #else
                // In ESP32, exceptions may not be enabled by default
                // while(1); 
            #endif
        }
    }

}
