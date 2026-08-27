#pragma once
#include "core/Actor.h"
#include "graphics/Renderer.h"
#include "GameConstants.h"
#include "physics/CollisionTypes.h"



namespace snake {

/**
 * @class SnakeSegmentActor
 * @brief Single segment of the snake (head or body).
 *
 * Head collides with body (game over). Position is grid-based.
 */
class SnakeSegmentActor : public pixelroot32::core::Actor {
public:
    /**
     * @brief Constructs segment at grid position.
     * @param gridX Grid column.
     * @param gridY Grid row.
     * @param head True for head (collision with body = game over).
     */
    SnakeSegmentActor(int gridX, int gridY, bool head);

    void update(unsigned long deltaTime) override;
    void draw(pixelroot32::graphics::Renderer& renderer) override;
    pixelroot32::core::Rect getHitBox() override;
    void onCollision(pixelroot32::core::Actor* other) override;

    void setCellPosition(int gridX, int gridY);
    int getCellX() const;
    int getCellY() const;

    void setHead(bool head);
    void resetAlive();
    bool isAlive() const;

private:
    int cellX;
    int cellY;
    bool isHead;
    bool alive;
};

}

