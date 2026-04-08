#pragma once
#include <core/Actor.h>
#include <graphics/Renderer.h>

namespace spaceinvaders {

/**
 * @enum AlienType
 * @brief Alien variant determining sprite, dimensions, and score value.
 */
enum class AlienType {
    SQUID,   ///< Top row, 30 points
    CRAB,    ///< Middle rows, 20 points
    OCTOPUS  ///< Bottom rows, 10 points
};

/**
 * @class AlienActor
 * @brief Enemy alien with step-based animation and type-specific visuals.
 *
 * Movement is controlled by SpaceInvadersScene; aliens do not move themselves
 * in update(). The Scene calls move() on each step to advance position and
 * animation frame in lockstep.
 */
class AlienActor : public pixelroot32::core::Actor {
public:
    /**
     * @brief Constructs an alien at the given position.
     * @param position World position (top-left).
     * @param type Alien variant (determines sprite and dimensions).
     */
    AlienActor(pixelroot32::math::Vector2 position, AlienType type);

    /**
     * @brief No-op; movement is driven by Scene via move().
     */
    void update(unsigned long deltaTime) override;

    /**
     * @brief Renders the alien using the type-specific sprite animation.
     * @param renderer Reference to the renderer.
     */
    void draw(pixelroot32::graphics::Renderer& renderer);

    /**
     * @brief Moves the alien and advances animation one frame.
     * Called by SpaceInvadersScene on each formation step.
     * @param dx Horizontal delta.
     * @param dy Vertical delta.
     */
    void move(pixelroot32::math::Scalar dx, pixelroot32::math::Scalar dy);

    pixelroot32::core::Rect getHitBox() override;
    void onCollision(pixelroot32::core::Actor* other) override;

    /**
     * @brief Returns true if the alien is alive and should be processed.
     */
    bool isActive() const { return active; }

    /**
     * @brief Marks the alien as destroyed.
     */
    void kill() { active = false; }

    /**
     * @brief Returns the score awarded when this alien is destroyed.
     */
    int getScoreValue() const;

private:
    AlienType type;                               ///< Alien variant
    bool active;                                  ///< Alive state
    pixelroot32::graphics::SpriteAnimation animation; ///< Step-based sprite animation
};

}
