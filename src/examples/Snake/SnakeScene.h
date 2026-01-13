#pragma once
#include "core/Scene.h"
#include "core/Entity.h"
#include <vector>
#include "particles/ParticleEmitter.h"

#define MAX_SNAKE_LEN 100
#define CELL_SIZE 10
#define GRID_WIDTH 24  // Para pantalla de 240px
#define GRID_HEIGHT 24

enum Direction { DIR_UP, DIR_DOWN, DIR_LEFT, DIR_RIGHT };

struct Point {
    int x, y;
};

class SnakeScene : public Scene {
private:
    std::vector<Point> snake;
    Point food;
    Direction dir;
    Direction nextDir;
    ParticleEmitter* explosionEffect;
    
    unsigned long lastMoveTime;
    unsigned long moveInterval;
    int score;
    bool gameOver;
    bool gameStarted;

    void spawnFood();
    bool isSafe(int x, int y);
    void resetGame();

public:
    SnakeScene();
    void init() override;
    void update(unsigned long deltaTime) override;
    void draw(Renderer& renderer) override;
};