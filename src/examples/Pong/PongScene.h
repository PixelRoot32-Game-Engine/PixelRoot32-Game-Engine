#pragma once
#include "Scene.h"
#include "PaddleActor.h"
#include "BallActor.h"
#include "ui/UILabel.h"

class PongScene : public Scene {
public:
    void init() override;
    void update(unsigned long deltaTime) override;
    void draw(Renderer& renderer) override;

    float getBallX() { return ball->x; }
    float getBallY() { return ball->y; }

private:
    UI::UILabel* lblLeftScore;
    UI::UILabel* lblRightScore;
    UI::UILabel* lblStartMessage;
    UI::UILabel* lblGameOver;

    PaddleActor* leftPaddle;
    PaddleActor* rightPaddle;
    BallActor* ball;

    int leftScore, rightScore;
    bool gameOver;

    void resetGame();
};
