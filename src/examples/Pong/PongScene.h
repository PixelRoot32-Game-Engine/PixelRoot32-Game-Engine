#pragma once
#include "Scene.h"
#include "PaddleActor.h"
#include "BallActor.h"
#include "ui/UILabel.h"

class PongScene : public pixelroot32::core::Scene {
public:
    void init() override;
    void update(unsigned long deltaTime) override;
    void draw(pixelroot32::graphics::Renderer& renderer) override;

    float getBallX() { return ball->x; }
    float getBallY() { return ball->y; }

private:
    pixelroot32::graphics::ui::UILabel* lblLeftScore;
    pixelroot32::graphics::ui::UILabel* lblRightScore;
    pixelroot32::graphics::ui::UILabel* lblStartMessage;
    pixelroot32::graphics::ui::UILabel* lblGameOver;

    PaddleActor* leftPaddle;
    PaddleActor* rightPaddle;
    BallActor* ball;

    int leftScore, rightScore;
    bool gameOver;

    void resetGame();
};
