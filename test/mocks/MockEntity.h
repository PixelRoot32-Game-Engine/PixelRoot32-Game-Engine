/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#pragma once

#include "core/Entity.h"

// Mock Entity implementation for testing real Entity class
class MockEntity : public Entity {
public:
    bool updateCalled = false;
    bool drawCalled = false;
    unsigned long lastDeltaTime = 0;
    
    MockEntity(float x, float y, int w, int h, EntityType t = EntityType::GENERIC)
        : Entity(pixelroot32::math::Vector2(x, y), w, h, t) {}
    
    void update(unsigned long deltaTime) override {
        updateCalled = true;
        lastDeltaTime = deltaTime;
    }
    
    void draw(pixelroot32::graphics::Renderer& renderer) override {
        (void)renderer;
        drawCalled = true;
    }
    
    void reset() {
        updateCalled = false;
        drawCalled = false;
        lastDeltaTime = 0;
    }
};