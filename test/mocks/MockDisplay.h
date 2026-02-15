/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#pragma once

#include "graphics/DrawSurface.h"
#include <vector>
#include <string>

namespace pixelroot32::graphics {

/**
 * @class MockDisplay
 * @brief A mock implementation of DrawSurface for testing hardware-related display functions.
 * 
 * Captures display-specific state changes and drawing calls.
 */
class MockDisplay : public DrawSurface {
public:
    struct DrawCall {
        std::string type;
        int x, y, x2, y2, w, h, r;
        uint16_t color;
        std::string text;
    };

    std::vector<DrawCall> calls;
    uint16_t rotation = 0;
    int logicalWidth = 240;
    int logicalHeight = 240;
    int physicalWidth = 240;
    int physicalHeight = 240;
    uint8_t contrast = 255;
    bool bufferCleared = false;
    bool bufferSent = false;
    bool presented = false;

    void init() override {}
    
    void setRotation(uint16_t rot) override {
        rotation = rot;
    }
    
    void clearBuffer() override {
        calls.clear();
        bufferCleared = true;
    }
    
    void sendBuffer() override {
        bufferSent = true;
    }
    
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

    void setContrast(uint8_t level) override {
        contrast = level;
    }
    
    void setTextColor(uint16_t color) override {}
    void setTextSize(uint8_t size) override {}
    void setCursor(int16_t x, int16_t y) override {}
    
    uint16_t color565(uint8_t r, uint8_t g, uint8_t b) override {
        return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
    }
    
    void setDisplaySize(int w, int h) override {
        logicalWidth = w;
        logicalHeight = h;
    }
    
    void setPhysicalSize(int w, int h) override {
        physicalWidth = w;
        physicalHeight = h;
    }
    
    void present() override {
        presented = true;
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
