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
#else
    #include "drivers/esp32/TFT_eSPI_Drawer.h"
#endif

#ifdef TEST_MOCK_GRAPHICS
#include <stdexcept>
#endif

namespace pixelroot32::graphics {

    void DisplayConfig::initDrawSurface() {
        if (drawSurface) return; // Already initialized or custom set
        
        DrawSurface* rawSurface = nullptr;
        #ifdef TEST_MOCK_GRAPHICS
            class MockDrawer : public DrawSurface {
            public:
                void init() override {}
                void setRotation(uint16_t) override {}
                void clearBuffer() override {}
                void sendBuffer() override {}
                void drawText(const char*, int16_t, int16_t, uint16_t, uint8_t) override {}
                void drawTextCentered(const char*, int16_t, uint16_t, uint8_t) override {}
                void drawFilledCircle(int, int, int, uint16_t) override {}
                void drawCircle(int, int, int, uint16_t) override {}
                void drawRectangle(int, int, int, int, uint16_t) override {}
                void drawFilledRectangle(int, int, int, int, uint16_t) override {}
                void drawLine(int, int, int, int, uint16_t) override {}
                void drawBitmap(int, int, int, int, const uint8_t*, uint16_t) override {}
                void drawPixel(int, int, uint16_t) override {}
                void setContrast(uint8_t) override {}
                void setTextColor(uint16_t) override {}
                void setTextSize(uint8_t) override {}
                void setCursor(int16_t, int16_t) override {}
                uint16_t color565(uint8_t, uint8_t, uint8_t) override { return 0; }
                void setDisplaySize(int, int) override {}
                void setPhysicalSize(int, int) override {}
                void present() override {}
            };
            rawSurface = new MockDrawer();
        #elif defined(PLATFORM_NATIVE)
            rawSurface = new pixelroot32::drivers::native::SDL2_Drawer();
        #else
            switch (type)
            {
            case DisplayType::ST7789:
            case DisplayType::ST7735:
            default:
                rawSurface = new pixelroot32::drivers::esp32::TFT_eSPI_Drawer();
                break;
            }
        #endif
        
        if (rawSurface == nullptr) {
            #ifdef PLATFORM_NATIVE
                throw std::runtime_error("Failed to initialize Display Driver: No valid driver selected or supported for this platform.");
            #else
                // In ESP32, exceptions may not be enabled by default
                // while(1); 
            #endif
        }
        drawSurface = std::unique_ptr<DrawSurface>(rawSurface);
    }

}
