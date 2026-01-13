#include "core/Scene.h"
#include "Paddle.h"
#include "Ball.h"
#include "graphics/ui/UILabel.h"
#include <vector>
#include "BrickEntity.h"
#include "particles/ParticleEmitter.h"

class BrickBreakerScene : public Scene {
public:
    void init() override;
    void update(unsigned long deltaTime) override;
    void draw(Renderer& renderer) override;

    void addScore(int score);
    ParticleEmitter* getParticleEmiter() { return explosionEffect; }


private:
    void loadLevel(int level);
    void resetBall();
    
    ParticleEmitter* explosionEffect;
    Paddle* paddle;
    Ball* ball;
    std::vector<BrickEntity*> bricks;

    UI::UILabel* lblGameOver;
    UI::UILabel* lblStartMessage;
    
    int score;
    int lives;
    int currentLevel;
    bool gameStarted;
    bool gameOver;
};