#pragma once
#include "core/Scene.h"
#include "graphics/Renderer.h"
#include "platforms/EngineConfig.h"
#include "GameConstants.h"
#include "SnakeSegmentActor.h"
#include "math/Vector2.h"
#include <vector>
#include <memory>

namespace snake {

/**
 * @enum Direction
 * @brief Snake movement direction.
 */
enum Direction {
    DIR_UP,
    DIR_DOWN,
    DIR_LEFT,
    DIR_RIGHT
};

/**
 * @class SnakeScene
 * @brief Classic Snake game with grid-based movement and object pooling.
 *
 * Uses pre-allocated segment pool to avoid allocations. No physics engine;
 * movement is discrete (cell by cell). nextDir prevents 180-degree turns.
 */
class SnakeScene : public pixelroot32::core::Scene {
public:
    SnakeScene();
    ~SnakeScene();
    void init() override;
    void update(unsigned long deltaTime) override;
    void draw(pixelroot32::graphics::Renderer& renderer) override;

private:
    static constexpr int MaxSnakeSegments = GRID_WIDTH * GRID_HEIGHT;
    class SnakeBackground;
    std::unique_ptr<SnakeBackground> background;

    std::vector<SnakeSegmentActor*> snakeSegments;
    std::vector<std::unique_ptr<SnakeSegmentActor>> segmentPool;
    pixelroot32::math::Vector2 food;
    Direction dir;
    Direction nextDir;
    int score;
    bool gameOver;
    unsigned long lastMoveTime;
    int moveInterval;

    void resetGame();
    void spawnFood();
};

}
