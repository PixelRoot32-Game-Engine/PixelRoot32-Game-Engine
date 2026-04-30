#include "BrickBreakerScene.h"
#include "GameLayers.h"
#include "GameConstants.h"
#include "assets/BrickAudioTracks.h"

#include <platforms/EngineConfig.h>
#include "core/Engine.h"
#include <graphics/Color.h>
#include <graphics/particles/ParticlePresets.h>

namespace pr32 = pixelroot32;

extern pr32::core::Engine engine;

namespace brickaudio = brickbreaker::audio;

namespace brickbreaker {

namespace core = pr32::core;
namespace gfx = pr32::graphics;
namespace physics = pr32::physics;
namespace math = pr32::math;
namespace prAudio = pr32::audio;
namespace input = pr32::input;

BrickBreakerScene::BrickBreakerScene() {
    musicPlayer = std::make_unique<prAudio::MusicPlayer>(engine.getAudioEngine());
}

BrickBreakerScene::~BrickBreakerScene() {
}

void BrickBreakerScene::refreshBgmTempoForLevel() {
    if (!musicPlayer || !musicPlayer->isPlaying()) return;
    musicPlayer->setTempoFactor(brickaudio::tempoForLevel(currentLevel));
}

void BrickBreakerScene::setupMusic() {
    musicPlayer->play(brickaudio::BGM_STAGE);
    musicPlayer->setTempoFactor(brickaudio::tempoForLevel(currentLevel));
}

void BrickBreakerScene::init() {
    gfx::setPalette(gfx::PaletteType::GBC);

    clearEntities(); 

    int sw = engine.getRenderer().getLogicalWidth();
    int sh = engine.getRenderer().getLogicalHeight();

    // Pre-allocate brick pool (eliminates runtime heap allocations)
    for (int i = 0; i < MAX_BRICK_POOL; i++) {
        brickPool[i] = std::make_unique<BrickActor>(math::Vector2(0, 0), 1);
        brickPool[i]->setEnabled(false);
    }
    brickActive.reset();
    activeBrickCount = 0;

    paddle = std::make_unique<PaddleActor>(math::Vector2(sw/2 - PADDLE_W/2, sh - 20), PADDLE_W, PADDLE_H, sw);

    ball = std::make_unique<BallActor>(math::Vector2(sw/2, sh - 3), 0, BALL_SIZE);
    ball->attachTo(paddle.get());
    
    explosionEffect = std::make_unique<gfx::particles::ParticleEmitter>(math::Vector2(100,100), gfx::particles::ParticlePresets::Explosion);

    topWall = std::make_unique<pr32::physics::StaticActor>(
        pr32::math::toScalar(0), 
        pr32::math::toScalar(0), 
        sw, 
        10
    );
    topWall->setBounce(true);
    topWall->setCollisionLayer(Layers::WALL);
    topWall->setShape(core::CollisionShape::AABB);

    leftWall = std::make_unique<physics::StaticActor>(
        math::toScalar(0), 
        math::toScalar(0), 
        10, 
        sh
    );
    leftWall->setBounce(true);
    leftWall->setCollisionLayer(Layers::WALL);
    leftWall->setShape(core::CollisionShape::AABB);

    rightWall = std::make_unique<physics::StaticActor>(
        math::toScalar(sw - 10), 
        math::toScalar(0), 
        10, 
        sh
    );
    rightWall->setBounce(true);
    rightWall->setCollisionLayer(Layers::WALL);
    rightWall->setShape(pr32::core::CollisionShape::AABB);

    addEntity(paddle.get());
    addEntity(ball.get());
    addEntity(explosionEffect.get());
    addEntity(topWall.get());
    addEntity(leftWall.get());
    addEntity(rightWall.get());

    lblGameOver = std::make_unique<gfx::ui::UILabel>("GAME OVER", math::Vector2(0, 120), gfx::Color::White, 2);
    lblGameOver->centerX(sw); 
    lblGameOver->setVisible(false);
    addEntity(lblGameOver.get());

    lblStartMessage = std::make_unique<gfx::ui::UILabel>("PRESS START", math::Vector2(0, 150), gfx::Color::White, 1);
    lblStartMessage->centerX(sw);
    lblStartMessage->setVisible(true);
    addEntity(lblStartMessage.get());

    score = 0;
    lives = 3;
    currentLevel = 1;
    gameStarted = false;
    gameOver = false;

    loadLevel(currentLevel);
    resetBall();
    setupMusic();
}

void BrickBreakerScene::addScore(int score) {
    this->score += score;
}

void BrickBreakerScene::loadLevel(int level) {
    // Reset all bricks in the pool
    for (int i = 0; i < MAX_BRICK_POOL; i++) {
        if (brickActive[i]) {
            removeEntity(brickPool[i].get());
            brickPool[i]->setEnabled(false);
        }
    }
    brickActive.reset();
    activeBrickCount = 0;

    int cols = 7;
    int spacingX = 32;
    int spacingY = 14;
    int offsetX = (engine.getRenderer().getLogicalWidth() - (cols * spacingX)) / 2 + 2;

    int rows = 3 + (level / 2); 
    if (rows > 7) rows = 7;

    for (int row = 0; row < rows; row++) {
        for (int col = 0; col < cols; col++) {
            
            bool shouldCreate = true;
            
            if (level % 2 == 0 && (row + col) % 2 == 0) shouldCreate = false;
            
            if (level >= 3 && col >= 2 && col <= 4 && row == 1) shouldCreate = false;

            if (shouldCreate) {
                // Find next available brick from pool
                int poolIndex = -1;
                for (int i = 0; i < MAX_BRICK_POOL; i++) {
                    if (!brickActive[i]) {
                        poolIndex = i;
                        break;
                    }
                }
                
                if (poolIndex >= 0) {
                    int posX = offsetX + (col * spacingX);
                    int posY = 40 + (row * spacingY);
                    
                    int brickHP = 1;

                    if (level == 1) {
                        brickHP = 1;
                    } else {
                        int baseHP = (rows - row); 
                        brickHP = baseHP + (level / 2) - 1;
                    }

                    if (brickHP > 4) brickHP = 4;
                    if (brickHP < 1) brickHP = 1;

                    // Reuse brick from pool
                    BrickActor* b = brickPool[poolIndex].get();
                    b->position = math::Vector2(posX, posY);
                    b->hp = brickHP;
                    b->active = true;
                    b->setEnabled(true);
                    b->setCollisionLayer(Layers::BRICK);
                    b->setCollisionMask(Layers::BALL);
                    
                    addEntity(b);
                    brickActive.set(poolIndex);
                    activeBrickCount++;
                }
            }
        }
    }
    refreshBgmTempoForLevel();
}

void BrickBreakerScene::resetBall() {
    ball->reset(paddle.get());
    gameStarted = false;
}

void BrickBreakerScene::update(unsigned long deltaTime) {
    if (gameOver) {
        if (engine.getInputManager().isButtonPressed(BTN_START)) {
            engine.getAudioEngine().playEvent(sfx::START_GAME);
            init();
        }
        lblGameOver->setVisible(true);
        lblStartMessage->setText("PRESS START TO RETRY");
        lblStartMessage->centerX(engine.getRenderer().getLogicalWidth());
        lblStartMessage->setVisible(true);
        
        if (musicPlayer->isPlaying()) musicPlayer->stop();
    } else if (!gameStarted) {
        if (engine.getInputManager().isButtonPressed(BTN_START)) {
            gameStarted = true;
            ball->launch(math::Vector2(120, -120));
            engine.getAudioEngine().playEvent(sfx::START_GAME);
            setupMusic();
        }
        lblStartMessage->setVisible(true);
        lblGameOver->setVisible(false);
    } else {
        lblStartMessage->setVisible(false);
        lblGameOver->setVisible(false);
    }

    Scene::update(deltaTime);

    if (!gameOver && gameStarted) {
        // Check level cleared using pool
        bool levelCleared = true;
        for (int i = 0; i < MAX_BRICK_POOL; i++) {
            if (brickActive[i] && brickPool[i]->active) {
                levelCleared = false;
                break;
            }
        }

        if (levelCleared) {
            currentLevel++;
            loadLevel(currentLevel);
            resetBall();
            engine.getAudioEngine().playEvent(sfx::START_GAME);
        }

        if (ball->isLaunched && ball->position.y > engine.getRenderer().getLogicalHeight()) {
            lives--;
            engine.getAudioEngine().playEvent(sfx::LIFE_LOST);
            
            if (lives <= 0) {
                gameOver = true;
            }

            resetBall();
        }
    }
}

void BrickBreakerScene::draw(pr32::graphics::Renderer& renderer) {
    char scoreStr[16];
    snprintf(scoreStr, sizeof(scoreStr), "SCORE %d", score);
    renderer.drawText(scoreStr, 10, 5, gfx::Color::White, 1);

    char levelStr[16];
    snprintf(levelStr, sizeof(levelStr), "LVL %d", currentLevel);
    renderer.drawText(levelStr, 10, 15, gfx::Color::White, 1);

    const int rectSize = 8;
    const int spacing = 4;      
    const int marginRight = 6; 
    const int posY = 6;
    
    int currentWidth = renderer.getLogicalWidth(); 

    for (int i = 0; i < lives; i++) {
        int x = currentWidth - marginRight - (i + 1) * (rectSize + spacing);
        
        renderer.drawFilledRectangle(x, posY, rectSize, rectSize, gfx::Color::Red);
    }

    Scene::draw(renderer);
}

}
