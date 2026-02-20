/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 * 
 * Physics Determinism Test Suite
 * Validates: determinism, energy conservation, stability
 */
#pragma once
#include "core/Scene.h"
#include "physics/RigidActor.h"
#include "physics/StaticActor.h"
#include <array>

namespace pixelroot32::test {

/**
 * @struct PhysicsSnapshot
 * @brief Captures physics state for determinism validation
 */
struct PhysicsSnapshot {
    pixelroot32::math::Vector2 position;
    pixelroot32::math::Vector2 velocity;
    uint32_t frameNumber;
    
    bool operator==(const PhysicsSnapshot& other) const {
        constexpr float EPSILON = 0.01f;
        return std::abs(static_cast<float>(position.x - other.position.x)) < EPSILON &&
               std::abs(static_cast<float>(position.y - other.position.y)) < EPSILON &&
               std::abs(static_cast<float>(velocity.x - other.velocity.x)) < EPSILON &&
               std::abs(static_cast<float>(velocity.y - other.velocity.y)) < EPSILON;
    }
};

/**
 * @class PhysicsTestSuite
 * @brief Comprehensive testing for Flat Solver
 */
class PhysicsTestSuite {
public:
    struct TestResult {
        bool passed = false;
        const char* testName = "";
        const char* errorMessage = "";
        float maxDeviation = 0.0f;
        uint32_t framesSimulated = 0;
    };

    // Test 1: Determinism - Same input produces same output
    static TestResult testDeterminism(int numRuns = 100, int numFrames = 600);
    
    // Test 2: Energy Conservation - No energy loss with restitution=1.0
    static TestResult testEnergyConservation(int numFrames = 1000);
    
    // Test 3: Stability - No drift or explosion with stacking
    static TestResult testStackingStability(int numBoxes = 10, int numFrames = 600);
    
    // Test 4: Bounce Consistency - Perfect elastic bounces
    static TestResult testBounceConsistency(int numBounces = 100);
    
    // Test 5: Performance - Measure time per frame
    static TestResult testPerformance(int numBodies = 20, int numFrames = 600);
    
    // Run all tests
    static void runAllTests();
    
private:
    static constexpr float ENERGY_TOLERANCE = 0.01f;  // 1% max energy loss
    static constexpr float POSITION_TOLERANCE = 0.1f; // 0.1px max deviation
    static constexpr float MAX_FRAME_TIME_MS = 16.67f; // 60 FPS minimum
};

/**
 * @class StressTestScene
 * @brief Scene for stress testing physics performance
 */
class StressTestScene : public pixelroot32::core::Scene {
public:
    void init() override;
    void update(unsigned long deltaTime) override;
    void draw(pixelroot32::graphics::Renderer& renderer) override;
    
    struct Metrics {
        unsigned long totalFrameTime = 0;
        unsigned long maxFrameTime = 0;
        unsigned long minFrameTime = 0xFFFFFFFF;
        uint32_t frameCount = 0;
        uint32_t collisionCount = 0;
        uint32_t activeBodies = 0;
    };
    
    const Metrics& getMetrics() const { return metrics; }
    void resetMetrics() { metrics = Metrics(); }
    
private:
    static constexpr int MAX_STRESS_BODIES = 50;
    std::array<std::unique_ptr<pixelroot32::physics::RigidActor>, MAX_STRESS_BODIES> bodies;
    std::unique_ptr<pixelroot32::physics::StaticActor> ground;
    std::unique_ptr<pixelroot32::physics::StaticActor> leftWall;
    std::unique_ptr<pixelroot32::physics::StaticActor> rightWall;
    Metrics metrics;
    unsigned long lastFrameTime = 0;
};

}
