#pragma once
#include "core/Scene.h"
#include "PaddleActor.h"
#include "BallActor.h"
#include "graphics/ui/UILabel.h"
#include <vector>
#include "BrickActor.h"
#include "graphics/particles/ParticleEmitter.h"

namespace brickbreaker {

class BrickBreakerScene : public pixelroot32::core::Scene {
public:
    void init() override;
    void update(unsigned long deltaTime) override;
    void draw(pixelroot32::graphics::Renderer& renderer) override;

    void addScore(int score);
    pixelroot32::graphics::particles::ParticleEmitter* getParticleEmiter() { return explosionEffect; }


private:
    void loadLevel(int level);
    void resetBall();
    
    pixelroot32::graphics::particles::ParticleEmitter* explosionEffect;
    PaddleActor* paddle;
    BallActor* ball;
    std::vector<BrickActor*> bricks;

    pixelroot32::graphics::ui::UILabel* lblGameOver;
    pixelroot32::graphics::ui::UILabel* lblStartMessage;
    
    int score;
    int lives;
    int currentLevel;
    bool gameStarted;
    bool gameOver;
};

}
