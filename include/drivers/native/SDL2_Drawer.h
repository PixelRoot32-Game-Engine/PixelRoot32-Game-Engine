/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#pragma once
#ifndef SDL2_DRAWER_H
#define SDL2_DRAWER_H

#ifdef PLATFORM_NATIVE

#include "graphics/BaseDrawSurface.h"
// SDL2 specific includes would go here
#include <SDL2/SDL.h>
#include <stdint.h>

// Forward declarations
namespace pixelroot32::input {
    class InputManager;
    class TouchEventDispatcher;
}

// Font constants removed - now using native bitmap font system via Renderer

namespace pixelroot32::drivers::native {

class SDL2_Drawer : public pixelroot32::graphics::BaseDrawSurface {
public:
    SDL2_Drawer();
    virtual ~SDL2_Drawer();

    void init() override;
    /**
     * @brief Sets the screen rotation.
     * @param rotation 0-3 corresponding to 0, 90, 180, 270 degrees.
     */
    void setRotation(uint16_t rotation) override;

    void clearBuffer() override;
    void sendBuffer() override;

    void drawFilledCircle(int x, int y, int radius, uint16_t color) override;
    void drawCircle(int x, int y, int radius, uint16_t color) override;
    void drawRectangle(int x, int y, int width, int height, uint16_t color) override;
    void drawFilledRectangle(int x, int y, int width, int height, uint16_t color) override;
    void drawLine(int x1, int y1, int x2, int y2, uint16_t color) override;
    void drawBitmap(int x, int y, int width, int height, const uint8_t *bitmap, uint16_t color) override;
    void drawPixel(int x, int y, uint16_t color) override;

    /**
     * @brief Direct tile write (not optimized for SDL2 - uses fallback).
     */
    void drawTileDirect(uint16_t x, uint16_t y, uint16_t width, uint16_t height, const uint8_t* data) override;

    /**
     * @brief Get pointer to sprite buffer (not supported in SDL2).
     */
    uint8_t* getSpriteBuffer() override {
        return nullptr;
    }

    bool processEvents() override;
    
    /**
     * @brief Set the TouchEventDispatcher to receive mouse events.
     * @param touchDispatcher Pointer to the Engine's TouchEventDispatcher.
     * 
     * This is the preferred method when PIXELROOT32_ENABLE_TOUCH is enabled.
     * Mouse events are mapped directly to touch events.
     */
    void setTouchDispatcher(pixelroot32::input::TouchEventDispatcher* touchDispatcher);
    
    /**
     * @brief Set the InputManager for backwards compatibility.
     * @param inputManager Pointer to the InputManager instance.
     * 
     * Deprecated: Use setTouchDispatcher() instead.
     */
    void setInputManager(pixelroot32::input::InputManager* inputManager);

private:
    SDL_Window* window;
    SDL_Renderer* renderer;
    SDL_Texture* texture;
    uint16_t* pixels; // Framebuffer (RGB565 format, matches texture)
    pixelroot32::input::TouchEventDispatcher* touchDispatcher;  ///< Direct touch dispatcher
    pixelroot32::input::InputManager* inputManager;  ///< Legacy fallback

    void updateTexture();

    inline void rgb565ToRGBA(
        uint16_t color,
        uint8_t& r,
        uint8_t& g,
        uint8_t& b,
        uint8_t& a
    ) {
        r = ((color >> 11) & 0x1F) * 255 / 31;
        g = ((color >> 5)  & 0x3F) * 255 / 63;
        b = ( color        & 0x1F) * 255 / 31;
        a = 255;
    }

    inline void drawHLine(int x, int y, int w, uint16_t color) {
        for (int i = 0; i < w; i++) {
            setPixel(x + i, y, color);
        }
    }

    inline void setPixel(int x, int y, uint16_t color) {
        if (x < 0 || y < 0 || x >= logicalWidth || y >= logicalHeight) return;
        
        pixels[y * logicalWidth + x] = color;
    }
};

} // namespace pixelroot32::drivers::native

#endif // PLATFORM_NATIVE

#endif // SDL2_DRAWER_H
