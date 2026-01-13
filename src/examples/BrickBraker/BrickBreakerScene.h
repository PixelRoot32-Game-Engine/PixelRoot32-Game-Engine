#include "core/Scene.h"
#include "BrickPaddleEntity.h"
#include "BrickBallEntity.h"
#include "graphics/ui/UILabel.h"
#include <vector>
#include "BrickEntity.h"
#include "particles/ParticleEmitter.h"

class BrickBreakerScene : public Scene {
public:
    void init() override;
    void update(unsigned long deltaTime) override;
    void draw(Renderer& renderer) override;

private:
    void loadLevel(int level);
    void resetBall();
    void checkBrickCollisions();
    
    ParticleEmitter* explosionEffect;
    BrickPaddleEntity* paddle;
    BrickBallEntity* ball;
    std::vector<BrickEntity*> bricks;

    UI::UILabel* lblGameOver;
    UI::UILabel* lblStartMessage;
    
    int score;
    int lives;
    int currentLevel;
    bool gameStarted;
    bool gameOver;
};