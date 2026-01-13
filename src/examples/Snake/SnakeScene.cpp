#include "SnakeScene.h"
#include "Config.h"
#include "core/EDGE.h"
#include "particles/ParticlePresets.h"

extern EDGE engine;

SnakeScene::SnakeScene() : Scene() {
    moveInterval = 150; // Velocidad inicial
}

void SnakeScene::init() {
    resetGame();
}

void SnakeScene::resetGame() {
    snake.clear();
    // Iniciar con 3 segmentos en el centro
    snake.push_back({12, 12}); 
    snake.push_back({11, 12});
    snake.push_back({10, 12});
    
    dir = DIR_RIGHT;
    nextDir = DIR_RIGHT;
    score = 0;
    gameOver = false;
    gameStarted = false;
    lastMoveTime = millis();
    spawnFood();

    
    // Creamos el emisor con un pool de 50 partículas
    explosionEffect = new ParticleEmitter(100, 100, ParticlePresets::Explosion);
    addEntity(explosionEffect);
}

void SnakeScene::spawnFood() {
    bool valid = false;
    while (!valid) {
        food.x = random(0, GRID_WIDTH);
        food.y = random(0, GRID_HEIGHT);
        valid = true;
        for (auto& p : snake) {
            if (p.x == food.x && p.y == food.y) {
                valid = false;
                break;
            }
        }
    }
}

void SnakeScene::update(unsigned long deltaTime) {
    if (gameOver) {
        if (engine.getInputManager().isButtonPressed(4)) resetGame();
        return;
    }

    if(explosionEffect) {
        explosionEffect->update(deltaTime);
    }


    // Leer Input (Evitar que se mueva en dirección opuesta directamente)
    if (engine.getInputManager().isButtonDown(0) && dir != DIR_DOWN) nextDir = DIR_UP;    // Arriba
    if (engine.getInputManager().isButtonDown(1) && dir != DIR_UP) nextDir = DIR_DOWN;    // Abajo
    if (engine.getInputManager().isButtonDown(2) && dir != DIR_RIGHT) nextDir = DIR_LEFT; // Izquierda
    if (engine.getInputManager().isButtonDown(3) && dir != DIR_LEFT) nextDir = DIR_RIGHT; // Derecha

    if (millis() - lastMoveTime >= moveInterval) {
        lastMoveTime = millis();
        dir = nextDir;

        // Calcular nueva cabeza
        Point newHead = snake[0];
        if (dir == DIR_UP) newHead.y--;
        else if (dir == DIR_DOWN) newHead.y++;
        else if (dir == DIR_LEFT) newHead.x--;
        else if (dir == DIR_RIGHT) newHead.x++;

        // Colisión con paredes
        if (newHead.x < 0 || newHead.x >= GRID_WIDTH || newHead.y < 0 || newHead.y >= GRID_HEIGHT) {
            gameOver = true;
            return;
        }

        // Colisión con el cuerpo
        for (auto& p : snake) {
            if (p.x == newHead.x && p.y == newHead.y) {
                gameOver = true;
                return;
            }
        }

        // Insertar nueva cabeza
        snake.insert(snake.begin(), newHead);

        // Colisión con comida
        if (newHead.x == food.x && newHead.y == food.y) {
            score += 10;
            spawnFood();
            explosionEffect->burst(food.x, food.y, 15);
            if (moveInterval > 50) moveInterval -= 2; // Aumentar velocidad
        } else {
            // Si no come, quitar la cola
            snake.pop_back();
        }
    }
}

void SnakeScene::draw(Renderer& renderer) {
    // 1. Dibujar Comida (Roja)
    renderer.drawFilledRectangle(food.x * CELL_SIZE, food.y * CELL_SIZE, CELL_SIZE - 1, CELL_SIZE - 1, 0xF800);

    // 2. Dibujar Serpiente
    for (size_t i = 0; i < snake.size(); i++) {
        // La cabeza es de un verde más brillante
        uint16_t color = (i == 0) ? 0x07E0 : 0x0400; 
        renderer.drawFilledRectangle(snake[i].x * CELL_SIZE, snake[i].y * CELL_SIZE, CELL_SIZE - 1, CELL_SIZE - 1, color);
    }

    // 3. UI
    char scoreStr[16];

    snprintf(scoreStr, sizeof(scoreStr), "SCORE: %d", score);
    renderer.drawText(scoreStr,5, 5, COLOR_WHITE, 2);

    if (gameOver) {
        renderer.drawTextCentered("GAME OVER", 100, COLOR_WHITE, 2);
        renderer.drawTextCentered("Press A to Restart", 130, COLOR_WHITE, 1);
    }
}