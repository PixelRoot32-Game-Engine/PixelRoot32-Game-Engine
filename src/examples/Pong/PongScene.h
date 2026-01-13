#pragma once
#include "core/Scene.h"
#include "PaddleEntity.h"
#include "BallEntity.h"

class PongScene : public Scene {
public:
    void init() override;
    void update(unsigned long deltaTime) override;
    void draw(Renderer& renderer) override;

    float getBallX() { return ball->x; }
    float getBallY() { return ball->y; }

private:
    PaddleEntity* leftPaddle;
    PaddleEntity* rightPaddle;
    BallEntity* ball;

    int leftScore, rightScore;
    bool gameOver;

    void resetGame();
};
