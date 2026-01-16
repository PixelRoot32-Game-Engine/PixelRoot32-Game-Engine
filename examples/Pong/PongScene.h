#pragma once
#include "core/Scene.h"
#include "PaddleActor.h"
#include "BallActor.h"
#include "graphics/ui/UILabel.h"
#include "graphics/Color.h"
#include "Config.h"
#include "GameConstants.h"

namespace pong {

class PongScene : public pixelroot32::core::Scene {
public:
    void init() override;
    void update(unsigned long deltaTime) override;
    void draw(pixelroot32::graphics::Renderer& renderer) override;

    float getBallX() { return ball->x; }
    float getBallY() { return ball->y; }

private:
    pixelroot32::graphics::ui::UILabel* lblScore;
    pixelroot32::graphics::ui::UILabel* lblStartMessage;
    pixelroot32::graphics::ui::UILabel* lblGameOver;

    PaddleActor* leftPaddle;
    PaddleActor* rightPaddle;
    BallActor* ball;

    int leftScore, rightScore;
    bool gameOver;
    
    // Play area bounds
    int playAreaTop;
    int playAreaBottom;

    void resetGame();
};

}
