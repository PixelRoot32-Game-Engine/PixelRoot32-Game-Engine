/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#pragma once

#include "graphics/BaseDrawSurface.h"
#include <vector>
#include <string>

namespace pixelroot32::graphics {

/**
 * @class MockDrawSurface
 * @brief A mock implementation of DrawSurface for testing.
 * 
 * Captures drawing calls so they can be verified in tests.
 */
class MockDrawSurface : public BaseDrawSurface {
public:
    static int instances;
    MockDrawSurface() { instances++; }
    virtual ~MockDrawSurface() { instances--; }

    struct DrawCall {
        std::string type;
        int x, y, x2, y2, w, h, r;
        uint16_t color;
        std::string text;
    };

    std::vector<DrawCall> calls;

    void init() override {}
    void clearBuffer() override { calls.clear(); }
    void sendBuffer() override {}
    
    void drawText(const char* text, int16_t x, int16_t y, uint16_t color, uint8_t size) override {
        calls.push_back({"text", x, y, 0, 0, 0, 0, 0, color, text});
    }
    
    void drawTextCentered(const char* text, int16_t y, uint16_t color, uint8_t size) override {
        calls.push_back({"text_centered", 0, y, 0, 0, 0, 0, 0, color, text});
    }
    
    void drawFilledCircle(int x, int y, int radius, uint16_t color) override {
        calls.push_back({"filled_circle", x, y, 0, 0, 0, 0, radius, color, ""});
    }
    
    void drawCircle(int x, int y, int radius, uint16_t color) override {
        calls.push_back({"circle", x, y, 0, 0, 0, 0, radius, color, ""});
    }
    
    void drawRectangle(int x, int y, int width, int height, uint16_t color) override {
        calls.push_back({"rectangle", x, y, 0, 0, width, height, 0, color, ""});
    }
    
    void drawFilledRectangle(int x, int y, int width, int height, uint16_t color) override {
        calls.push_back({"filled_rectangle", x, y, 0, 0, width, height, 0, color, ""});
    }
    
    void drawLine(int x1, int y1, int x2, int y2, uint16_t color) override {
        calls.push_back({"line", x1, y1, x2, y2, 0, 0, 0, color, ""});
    }
    
    void drawBitmap(int x, int y, int width, int height, const uint8_t *bitmap, uint16_t color) override {
        calls.push_back({"bitmap", x, y, 0, 0, width, height, 0, color, ""});
    }
    
    void drawPixel(int x, int y, uint16_t color) override {
        calls.push_back({"pixel", x, y, 0, 0, 0, 0, 0, color, ""});
    }

    // Helper to find if a specific call was made
    bool hasCall(const std::string& type) const {
        for (const auto& call : calls) {
            if (call.type == type) return true;
        }
        return false;
    }
};

} // namespace pixelroot32::graphics
