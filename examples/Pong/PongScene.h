#pragma once
#include "core/Scene.h"
#include "PaddleActor.h"
#include "BallActor.h"
#include "graphics/ui/UILabel.h"
#include "graphics/Color.h"

// Game constants
#define PADDLE_WIDTH 10
#define PADDLE_HEIGHT 50
#define BALL_RADIUS 6
#define BALL_SPEED 120.0f
#define SCORE_TO_WIN 5

// Rectangular play area (more width than height, like classic Pong)
#define PONG_PLAY_AREA_HEIGHT 160  // Reduced height for rectangular aspect
#define PONG_PLAY_AREA_TOP ((DISPLAY_HEIGHT - PONG_PLAY_AREA_HEIGHT) / 2)  // Center vertically
#define PONG_PLAY_AREA_BOTTOM (PONG_PLAY_AREA_TOP + PONG_PLAY_AREA_HEIGHT)

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

    void resetGame();
};

}
