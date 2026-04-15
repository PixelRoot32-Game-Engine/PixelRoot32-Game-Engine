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
namespace core = pr32::core;
namespace audio = pr32::audio;
namespace math = pr32::math;

SnakeScene::SnakeScene()
    : background(nullptr),
      dir(DIR_RIGHT),
      nextDir(DIR_RIGHT),
      score(0),
      gameOver(false),
      lastMoveTime(0),
      moveInterval(INITIAL_MOVE_INTERVAL_MS),
      snakeLength(0) {
    background = std::make_unique<SnakeBackground>();
    addEntity(background.get());
    
    // Initialize segment pool with null pointers
    for (auto& segment : segmentPool) {
        segment = nullptr;
    }
}

SnakeScene::~SnakeScene() {
    for (size_t i = 0; i < snakeLength; ++i) {
        if (snakeSegments[i]) {
            removeEntity(snakeSegments[i]);
        }
    }
    
    if (background) {
        removeEntity(background.get());
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
    // Remove existing segments from scene
    for (size_t i = 0; i < snakeLength; ++i) {
        if (snakeSegments[i]) {
            removeEntity(snakeSegments[i]);
        }
    }
    
    // Initialize pool if empty
    static std::unique_ptr<SnakeSegmentActor> poolStorage[MaxSnakeSegments];
    static bool poolInitialized = false;
    
    if (!poolInitialized) {
        for (int i = 0; i < MaxSnakeSegments; ++i) {
            poolStorage[i] = std::make_unique<SnakeSegmentActor>(0, 0, false);
            segmentPool[i] = poolStorage[i].get();
        }
        poolInitialized = true;
    }

    // Clear active tracking
    activeSegments.reset();
    snakeLength = 0;

    int centerX = GRID_WIDTH / 2;
    int centerY = GRID_HEIGHT / 2;

    // Spawn initial snake (growing backwards from center)
    const int initialLength = 4;
    for (int i = 0; i < initialLength; ++i) {
        int x = centerX - i;
        int y = centerY;
        bool head = (i == 0);
        
        SnakeSegmentActor* segment = segmentPool[i];
        segment->setCellPosition(x, y);
        segment->resetAlive();
        segment->setHead(head);
        
        snakeSegments[i] = segment;
        activeSegments.set(i);
        addEntity(segment);
    }
    snakeLength = initialLength;

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

        // Get head position
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
            audio::AudioEvent crashSound;
            crashSound.type = audio::WaveType::NOISE;
            crashSound.frequency = 50.0f;
            crashSound.duration = 0.5f;
            crashSound.volume = 0.8f;
            crashSound.duty = 0.5f;
            engine.getAudioEngine().playEvent(crashSound);
            
            return;
        }

        // Collision with self (check all active segments)
        for (size_t i = 0; i < snakeLength; ++i) {
            if (snakeSegments[i]->getCellX() == newHeadX && snakeSegments[i]->getCellY() == newHeadY) {
                gameOver = true;
                
                // Play crash sound
                audio::AudioEvent crashSound;
                crashSound.type = audio::WaveType::NOISE;
                crashSound.frequency = 50.0f;
                crashSound.duration = 0.5f;
                crashSound.volume = 0.8f;
                crashSound.duty = 0.5f;
                engine.getAudioEngine().playEvent(crashSound);
                
                return;
            }
        }

        // Play move sound
        audio::AudioEvent moveSound;
        moveSound.type = audio::WaveType::TRIANGLE;
        moveSound.frequency = 200.0f;
        moveSound.duration = 0.05f;
        moveSound.volume = 0.2f;
        moveSound.duty = 0.5f;
        engine.getAudioEngine().playEvent(moveSound);

        // Check if ate food
        bool ateFood = (newHeadX * CELL_SIZE == static_cast<int>(food.x) && newHeadY * CELL_SIZE == static_cast<int>(food.y));

        if (ateFood) {
            // Growth: Find free slot in pool
            if (snakeLength < MaxSnakeSegments) {
                size_t newIndex = 0;
                for (size_t i = 0; i < MaxSnakeSegments; ++i) {
                    if (!activeSegments.test(i)) {
                        newIndex = i;
                        break;
                    }
                }
                
                SnakeSegmentActor* newSegment = segmentPool[newIndex];
                newSegment->setCellPosition(newHeadX, newHeadY);
                newSegment->setHead(true);
                newSegment->resetAlive();
                
                if (snakeLength > 0) {
                    snakeSegments[0]->setHead(false); // Old head becomes body
                }
                
                // Shift all segments back to make room at front
                for (size_t i = snakeLength; i > 0; --i) {
                    snakeSegments[i] = snakeSegments[i - 1];
                }
                
                snakeSegments[0] = newSegment;
                activeSegments.set(newIndex);
                ++snakeLength;
                addEntity(newSegment);
                
                score += 10;
                spawnFood();
                
                // Speed up
                if (moveInterval > MIN_MOVE_INTERVAL_MS) {
                    moveInterval -= 2;
                }
                
                // Play eat sound
                audio::AudioEvent eatSound;
                eatSound.type = audio::WaveType::PULSE;
                eatSound.frequency = 600.0f;
                eatSound.duration = 0.1f;
                eatSound.volume = 0.7f;
                eatSound.duty = 0.5f;
                engine.getAudioEngine().playEvent(eatSound);
            }
        } else {
            // Movement only: reuse tail as new head
            SnakeSegmentActor* tail = snakeSegments[snakeLength - 1];
            
            tail->setCellPosition(newHeadX, newHeadY);
            tail->setHead(true);
            
            if (snakeLength > 0) {
                snakeSegments[0]->setHead(false);
            }
            
            // Shift all segments back
            for (size_t i = snakeLength; i > 0; --i) {
                snakeSegments[i] = snakeSegments[i - 1];
            }
            
            snakeSegments[0] = tail;
        }
    }
}

void SnakeScene::draw(gfx::Renderer& renderer) {
    // Draw all entities (background, snake segments)
    Scene::draw(renderer);

    // Draw Food
    renderer.drawFilledRectangle(static_cast<int>(food.x), static_cast<int>(food.y), CELL_SIZE - 1, CELL_SIZE - 1, gfx::Color::Red);

    // Score
    char scoreBuffer[16];
    std::snprintf(scoreBuffer, sizeof(scoreBuffer), "SCORE: %d", score);
    renderer.drawText(scoreBuffer, SCORE_TEXT_X, SCORE_TEXT_Y, gfx::Color::Red, 1);

    if (gameOver) {
        renderer.drawTextCentered("GAME OVER", GAME_OVER_TEXT_Y, gfx::Color::Red, 1);
        renderer.drawTextCentered("PRESS ACTION", PRESS_ACTION_TEXT_Y, gfx::Color::Red, 1);
    }
}

void SnakeScene::spawnFood() {
    bool valid = false;
    while (!valid) {
        int gx = std::rand() % GRID_WIDTH;
        int gy = std::rand() % (GRID_HEIGHT - TOP_UI_GRID_ROWS) + TOP_UI_GRID_ROWS;
        food.x = math::toScalar(gx * CELL_SIZE);
        food.y = math::toScalar(gy * CELL_SIZE);

        valid = true;
        for (size_t i = 0; i < snakeLength; ++i) {
            if (snakeSegments[i]->position == food) {
                valid = false;
                break;
            }
        }
    }
}

}
