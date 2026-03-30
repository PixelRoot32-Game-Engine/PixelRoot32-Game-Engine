/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#pragma once

#include "core/Scene.h"

// Mock Scene for testing Engine integration
class MockScene : public Scene {
public:
    MockScene() : updateCount_(0), drawCount_(0) {}
    
    void update(unsigned long deltaTime) override {
        updateCount_++;
        lastDeltaTime_ = deltaTime;
    }
    
    void draw(Renderer& renderer) override {
        drawCount_++;
        (void)renderer;
    }
    
    int getUpdateCount() const { return updateCount_; }
    int getDrawCount() const { return drawCount_; }
    unsigned long getLastDeltaTime() const { return lastDeltaTime_; }
    
private:
    int updateCount_;
    int drawCount_;
    unsigned long lastDeltaTime_;
};