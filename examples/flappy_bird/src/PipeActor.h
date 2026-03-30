#pragma once

#include <physics/KinematicActor.h>

#include "FlappyBirdConstants.h"

namespace flappy {

/**
 * @class PipeActor
 * @brief Scrolling pipe obstacle (top or bottom half).
 *
 * Moves left via KinematicActor. Recycled when off-screen for object pooling.
 * Top pipe tracks scoring (bird passed through gap).
 */
class PipeActor : public pixelroot32::physics::KinematicActor {
public:
    /**
     * @brief Constructs pipe at position.
     * @param pos World position (top-left).
     * @param width Width in pixels.
     * @param height Height in pixels.
     * @param isTop True for top pipe, false for bottom.
     */
    PipeActor(pixelroot32::math::Vector2 pos, int width, int height, bool isTop);

    void update(unsigned long deltaTime) override;
    void draw(pixelroot32::graphics::Renderer& renderer) override;

    /** @brief Updates pipe dimensions. */
    void setSize(int width, int height);

    /**
     * @brief Recycles pipe when off-screen. Returns true if reset.
     * @param screenWidth Display width.
     * @param screenHeight Display height.
     * @param newGapY New gap center Y for next spawn.
     */
    bool resetIfOffScreen(int screenWidth, int screenHeight, int newGapY);

    /** @brief Returns true if bird has passed this pipe (scoring). */
    bool isPassed(pixelroot32::math::Vector2 birdPos) const;

    /** @brief Marks pipe as passed (score awarded). */
    void markPassed() { passed = true; }

    /** @brief Returns true if pipe was already passed. */
    bool hasPassed() const { return passed; }

private:
    bool isTopPipe;   ///< True for top pipe
    bool passed = false; ///< Scoring state
};

} // namespace flappy
