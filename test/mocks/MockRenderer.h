/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#pragma once

#include "graphics/Renderer.h"
#include <vector>
#include <string>

namespace pixelroot32::graphics {

/**
 * @class MockRenderer
 * @brief A mock implementation of Renderer for testing.
 * 
 * Captures high-level drawing commands instead of pixel-level operations.
 * Inherits from Renderer to be compatible with systems expecting a Renderer.
 */
class MockRenderer : public Renderer {
public:
    struct DrawCall {
        std::string type;
        int x, y, x2, y2, w, h, r;
        Color color;
        std::string text;
        float scaleX, scaleY;
    };

    std::vector<DrawCall> rendererCalls;

    MockRenderer(const DisplayConfig& config) : Renderer(config) {}

    // Override common drawing methods to capture calls
    void drawText(const char* text, int16_t x, int16_t y, Color color, uint8_t size) {
        rendererCalls.push_back({"text", x, y, 0, 0, 0, 0, 0, color, text, 1.0f, 1.0f});
    }

    void drawTextCentered(const char* text, int16_t y, Color color, uint8_t size) {
        rendererCalls.push_back({"text_centered", 0, y, 0, 0, 0, 0, 0, color, text, 1.0f, 1.0f});
    }

    void drawFilledCircle(int x, int y, int radius, Color color) {
        rendererCalls.push_back({"filled_circle", x, y, 0, 0, 0, 0, radius, color, "", 1.0f, 1.0f});
    }

    void drawCircle(int x, int y, int radius, Color color) {
        rendererCalls.push_back({"circle", x, y, 0, 0, 0, 0, radius, color, "", 1.0f, 1.0f});
    }

    void drawRectangle(int x, int y, int width, int height, Color color) {
        rendererCalls.push_back({"rectangle", x, y, 0, 0, width, height, 0, color, "", 1.0f, 1.0f});
    }

    void drawFilledRectangle(int x, int y, int width, int height, Color color) {
        rendererCalls.push_back({"filled_rectangle", x, y, 0, 0, width, height, 0, color, "", 1.0f, 1.0f});
    }

    void drawLine(int x1, int y1, int x2, int y2, Color color) {
        rendererCalls.push_back({"line", x1, y1, x2, y2, 0, 0, 0, color, "", 1.0f, 1.0f});
    }

    void drawPixel(int x, int y, Color color) {
        rendererCalls.push_back({"pixel", x, y, 0, 0, 0, 0, 0, color, "", 1.0f, 1.0f});
    }

    void drawSprite(const Sprite& sprite, int x, int y, Color color, bool flipX = false) {
        rendererCalls.push_back({"sprite", x, y, 0, 0, sprite.width, sprite.height, 0, color, "", 1.0f, 1.0f});
    }

    void drawSprite(const Sprite& sprite, int x, int y, float scaleX, float scaleY, Color color, bool flipX = false) {
        rendererCalls.push_back({"sprite_scaled", x, y, 0, 0, sprite.width, sprite.height, 0, color, "", scaleX, scaleY});
    }

    void clear() {
        rendererCalls.clear();
    }

    // Helper to find if a specific call was made
    bool hasCall(const std::string& type) const {
        for (const auto& call : rendererCalls) {
            if (call.type == type) return true;
        }
        return false;
    }
};

} // namespace pixelroot32::graphics
