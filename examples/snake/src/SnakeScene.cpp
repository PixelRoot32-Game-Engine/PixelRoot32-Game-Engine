#include "SnakeScene.h"
#include "core/Engine.h"
#include "audio/AudioTypes.h"
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include "math/Vector2.h"

namespace pr32 = pixelroot32;

extern pr32::core::Engine engine;

namespace snake {

namespace gfx = pr32::graphics;

using Color = gfx::Color;

class SnakeScene::SnakeBackground : public pr32::core::Entity {
public:
    SnakeBackground()
        : pr32::core::Entity(pr32::math::Vector2::ZERO(), DISPLAY_WIDTH, DISPLAY_HEIGHT, pr32::core::EntityType::GENERIC) {
        setRenderLayer(0);
    }

    void update(unsigned long) override {
    }

    void draw(gfx::Renderer& renderer) override {
        renderer.drawFilledRectangle(0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT, Color::Black);

        int playAreaX = 0;
        int playAreaY = TOP_UI_GRID_ROWS * CELL_SIZE;
        int playAreaW = GRID_WIDTH * CELL_SIZE - 1;
        int playAreaH = (GRID_HEIGHT - TOP_UI_GRID_ROWS) * CELL_SIZE - 1;
        renderer.drawRectangle(playAreaX, playAreaY, playAreaW, playAreaH, Color::DarkGreen);
    }
};

SnakeScene::SnakeScene()
    : background(nullptr),
      dir(DIR_RIGHT),
      nextDir(DIR_RIGHT),
      score(0),
      gameOver(false),
      lastMoveTime(0),
      moveInterval(INITIAL_MOVE_INTERVAL_MS) {
    background = std::make_unique<SnakeBackground>();
    addEntity(background.get());
}

SnakeScene::~SnakeScene() {
    for (auto* segment : snakeSegments) {
        removeEntity(segment);
    }
    snakeSegments.clear();
    segmentPool.clear();

    if (background) {
        removeEntity(background.get());
        background = nullptr;
    }
}

// Seed the random generator once and reset the snake game state.
void SnakeScene::init() {
    gfx::setPalette(gfx::PaletteType::GB);
    static bool seeded = false;
    if (!seeded) {
        std::srand(static_cast<unsigned int>(std::time(nullptr)));
        seeded = true;
    }
    resetGame();
}

void SnakeScene::resetGame() {
    if (segmentPool.empty()) {
        segmentPool.reserve(MaxSnakeSegments);
        for (int i = 0; i < MaxSnakeSegments; ++i) {
            auto segment = std::make_unique<SnakeSegmentActor>(0, 0, false);
            segmentPool.push_back(std::move(segment));
        }
    }

    for (auto* segment : snakeSegments) {
        removeEntity(segment);
    }
    snakeSegments.clear();

    int centerX = GRID_WIDTH / 2;
    int centerY = GRID_HEIGHT / 2;

    const int initialLength = 4;
    for (int i = 0; i < initialLength; ++i) {
        int x = centerX - i;
        int y = centerY;
        bool head = (i == 0);
        SnakeSegmentActor* segment = segmentPool[i].get();
        segment->setCellPosition(x, y);
        segment->resetAlive();
        segment->setHead(head);
        snakeSegments.push_back(segment);
        addEntity(segment);
    }

    spawnFood();
    dir = DIR_RIGHT;
    nextDir = DIR_RIGHT;
    score = 0;
    gameOver = false;
    moveInterval = INITIAL_MOVE_INTERVAL_MS;
    lastMoveTime = engine.getMillis();
}

void SnakeScene::update(unsigned long deltaTime) {
    (void)deltaTime;

    auto& input = engine.getInputManager();

    if (gameOver) {
        if (input.isButtonPressed(BTN_RESTART)) {
            resetGame();
        }
        return;
    }

    if (input.isButtonPressed(BTN_UP) && dir != DIR_DOWN) nextDir = DIR_UP;
    else if (input.isButtonPressed(BTN_DOWN) && dir != DIR_UP) nextDir = DIR_DOWN;
    else if (input.isButtonPressed(BTN_LEFT) && dir != DIR_RIGHT) nextDir = DIR_LEFT;
    else if (input.isButtonPressed(BTN_RIGHT) && dir != DIR_LEFT) nextDir = DIR_RIGHT;

    unsigned long now = engine.getMillis();
    if (now - lastMoveTime >= (unsigned long)moveInterval) {
        lastMoveTime = now;
        dir = nextDir;

        int newHeadX = snakeSegments[0]->getCellX();
        int newHeadY = snakeSegments[0]->getCellY();

        switch (dir) {
            case DIR_UP:    newHeadY--; break;
            case DIR_DOWN:  newHeadY++; break;
            case DIR_LEFT:  newHeadX--; break;
            case DIR_RIGHT: newHeadX++; break;
        }

        // Collision with walls
        if (newHeadX < 0 || newHeadX >= GRID_WIDTH || 
            newHeadY < TOP_UI_GRID_ROWS || newHeadY >= GRID_HEIGHT) {
            gameOver = true;
            
            // Play crash sound
            pr32::audio::AudioEvent crashSound;
            crashSound.type = pr32::audio::WaveType::NOISE;
            crashSound.frequency = 50.0f;
            crashSound.duration = 0.5f;
            crashSound.volume = 0.8f;
            crashSound.duty = 0.5f;
            engine.getAudioEngine().playEvent(crashSound);
            
            return;
        }

        // Collision with self
        for (auto* segment : snakeSegments) {
            if (segment->getCellX() == newHeadX && segment->getCellY() == newHeadY) {
                gameOver = true;
                
                // Play crash sound
                pr32::audio::AudioEvent crashSound;
                crashSound.type = pr32::audio::WaveType::NOISE;
                crashSound.frequency = 50.0f;
                crashSound.duration = 0.5f;
                crashSound.volume = 0.8f;
                crashSound.duty = 0.5f;
                engine.getAudioEngine().playEvent(crashSound);
                
                return;
            }
        }

        // Play move sound
        pr32::audio::AudioEvent moveSound;
        moveSound.type = pr32::audio::WaveType::TRIANGLE;
        moveSound.frequency = 200.0f;
        moveSound.duration = 0.05f;
        moveSound.volume = 0.2f;
        moveSound.duty = 0.5f;
        engine.getAudioEngine().playEvent(moveSound);

        // Move snake
        bool ateFood = (newHeadX * CELL_SIZE == static_cast<int>(food.x) && newHeadY * CELL_SIZE == static_cast<int>(food.y));

        if (ateFood) {
            // Add new segment
            if (snakeSegments.size() < MaxSnakeSegments) {
                 // Use a pooled segment if available (we always have pool full, but some are unused?)
                 // Ah, logic: segmentPool owns ALL segments. snakeSegments points to active ones.
                 // We need to find an unused segment in the pool.
                 
                 SnakeSegmentActor* newSegment = nullptr;
                 for (auto& pooled : segmentPool) {
                     bool used = false;
                     for (auto* active : snakeSegments) {
                         if (active == pooled.get()) {
                             used = true;
                             break;
                         }
                     }
                     if (!used) {
                         newSegment = pooled.get();
                         break;
                     }
                 }
                 
                 if (newSegment) {
                    newSegment->setCellPosition(newHeadX, newHeadY);
                    newSegment->setHead(true);
                    newSegment->resetAlive();
                    snakeSegments[0]->setHead(false); // Old head becomes body
                    
                    // Insert at front
                    snakeSegments.insert(snakeSegments.begin(), newSegment);
                    addEntity(newSegment);
                    
                    score += 10;
                    spawnFood();
                    
                    // Speed up
                    if (moveInterval > MIN_MOVE_INTERVAL_MS) {
                        moveInterval -= 2;
                    }
                    
                    // Play eat sound
                    pr32::audio::AudioEvent eatSound;
                    eatSound.type = pr32::audio::WaveType::PULSE;
                    eatSound.frequency = 600.0f;
                    eatSound.duration = 0.1f;
                    eatSound.volume = 0.7f;
                    eatSound.duty = 0.5f;
                    engine.getAudioEngine().playEvent(eatSound);
                 }
            }
        } else {
            // Move tail to head
            SnakeSegmentActor* tail = snakeSegments.back();
            snakeSegments.pop_back();
            
            tail->setCellPosition(newHeadX, newHeadY);
            tail->setHead(true);
            snakeSegments[0]->setHead(false);
            
            snakeSegments.insert(snakeSegments.begin(), tail);
        }
    }
}

void SnakeScene::draw(gfx::Renderer& renderer) {
    // Draw all entities (background, snake segments)
    Scene::draw(renderer);

    // Draw Food
    renderer.drawFilledRectangle(static_cast<int>(food.x), static_cast<int>(food.y), CELL_SIZE - 1, CELL_SIZE - 1, Color::Red);

    // Score
    char scoreBuffer[16];
    std::snprintf(scoreBuffer, sizeof(scoreBuffer), "SCORE: %d", score);
    renderer.drawText(scoreBuffer, 2, 2, Color::White, 1);

    if (gameOver) {
        renderer.drawTextCentered("GAME OVER", 20, Color::White, 1);
        renderer.drawTextCentered("PRESS ACTION", 40, Color::White, 1);
    }
}

void SnakeScene::spawnFood() {
    bool valid = false;
    while (!valid) {
        int gx = std::rand() % GRID_WIDTH;
        int gy = std::rand() % (GRID_HEIGHT - TOP_UI_GRID_ROWS) + TOP_UI_GRID_ROWS;
        food.x = pr32::math::toScalar(gx * CELL_SIZE);
        food.y = pr32::math::toScalar(gy * CELL_SIZE);

        valid = true;
        for (auto* segment : snakeSegments) {
            if (segment->position == food) {
                valid = false;
                break;
            }
        }
    }
}

}
