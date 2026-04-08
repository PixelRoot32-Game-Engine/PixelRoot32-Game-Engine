/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 * 
 * PhysicsScheduler - Fixed timestep accumulator for consistent physics simulation
 */
#pragma once
#include <cstdint>
#include "physics/CollisionSystem.h"

#ifndef IRAM_ATTR
#define IRAM_ATTR __attribute__((always_inline))
#endif

namespace pixelroot32::physics {

class PhysicsScheduler {
public:
    /// Fixed timestep in microseconds (60 Hz)
    static constexpr uint32_t FIXED_DT_MICROS = 16667;
    
    /// Maximum physics steps per frame under normal conditions
    static constexpr uint8_t MAX_STEPS_NORMAL = 2;
    
    /// Maximum physics steps when behind (catch-up mode)
    static constexpr uint8_t MAX_STEPS_BACKLOG = 4;

    PhysicsScheduler() = default;

    /// Initialize the scheduler
    inline void init() {
        accumulatorMicros = 0;
        stepsExecuted = 0;
    }

    /// Update physics simulation with fixed timestep
    /// @param realDeltaMicros Time elapsed since last frame in microseconds
    /// @param collisionSystem Reference to the collision system
    /// @return Number of physics steps executed
    inline uint8_t update(uint32_t realDeltaMicros, CollisionSystem& collisionSystem) {
        // Add real elapsed time to accumulator
        accumulatorMicros += realDeltaMicros;
        
        // Determine max steps based on how far behind we are
        // If accumulator is more than 2.5 frames behind, allow extra catch-up steps
        uint8_t maxSteps = MAX_STEPS_NORMAL;
        if (accumulatorMicros > (FIXED_DT_MICROS * 5 / 2)) {
            maxSteps = MAX_STEPS_BACKLOG;
        }
        
        // Execute fixed timestep physics steps
        stepsExecuted = 0;
        while (accumulatorMicros >= FIXED_DT_MICROS && stepsExecuted < maxSteps) {
            // Execute one physics step
            collisionSystem.update();
            
            // Subtract fixed timestep from accumulator
            accumulatorMicros -= FIXED_DT_MICROS;
            
            stepsExecuted++;
        }
        
        // Note: We do NOT clamp/cap the accumulator
        // This preserves real time and allows catch-up on subsequent frames
        
        return stepsExecuted;
    }

    /// Get the number of physics steps executed in last update
    /// @return Steps executed
    uint8_t getStepsExecuted() const { return stepsExecuted; }
    
    /// Get the current accumulator value (for debugging/profiling)
    /// @return Accumulated microseconds pending
    uint32_t getAccumulator() const { return accumulatorMicros; }

private:
    /// Accumulated time in microseconds (no clamping - preserves real time)
    uint32_t accumulatorMicros = 0;
    
    /// Number of physics steps executed in last update
    uint8_t stepsExecuted = 0;
};

} // namespace physics