#include "BrickBreakerScene.h"
#include "Config.h"
#include "core/EDGE.h"
#include "particles/ParticlePresets.h"
#include "GameLayers.h"

extern EDGE engine;

#define BRICK_WIDTH 30
#define BRICK_HEIGHT 12
#define PADDLE_W 40
#define PADDLE_H 8
#define BALL_SIZE 6
#define BORDER_TOP 20.0

void BrickBreakerScene::init() {
    clearEntities(); 

    int sw = engine.getRenderer().getWidth();
    int sh = engine.getRenderer().getHeight();

    paddle = new Paddle(sw/2 - PADDLE_W/2, sh - 20, PADDLE_W, PADDLE_H, engine.getInputManager(), sw);
    ball = new Ball(sw/2, sh - 30, (float)BALL_SIZE / 2.0f, (float)sw, (float)BORDER_TOP);
    ball->attachTo(paddle);
    
    explosionEffect = new ParticleEmitter(100,100, ParticlePresets::Explosion);

    addEntity(paddle);
    addEntity(ball);
    addEntity(explosionEffect);

    lblGameOver = new UI::UILabel("GAME OVER", 0, 120, COLOR_WHITE, 2);
    lblGameOver->centerX(sw); 
    lblGameOver->setVisible(false);
    addEntity(lblGameOver);

    // Label de Inicio
    lblStartMessage = new UI::UILabel("PRESS A TO START", 0, 150, COLOR_WHITE, 1);
    lblStartMessage->centerX(sw);
    lblStartMessage->setVisible(true);
    addEntity(lblStartMessage);

    score = 0;
    lives = 3;
    currentLevel = 1;
    gameStarted = false;
    gameOver = false;

    loadLevel(currentLevel);
    resetBall();
}

void BrickBreakerScene::addScore(int score) {
    this->score += score;
}

void BrickBreakerScene::loadLevel(int level) {
    // 1. Limpieza de entidades previas (Importante en tu motor)
    // Suponiendo que bricks es un std::vector<BrickEntity*>
    for(auto* b : bricks) {
        // Aquí deberías llamar a una función del motor para eliminar la entidad 
        // o marcar b->active = false para que el motor la limpie.
    }
    bricks.clear(); 

    // 2. Parámetros dinámicos basados en el nivel
    int cols = 7;
    int spacingX = 32;
    int spacingY = 14;
    int offsetX = (engine.getRenderer().getWidth() - (cols * spacingX)) / 2 + 2;

    // Aumentamos filas cada 2 niveles, máximo 7 filas
    int rows = 3 + (level / 2); 
    if (rows > 7) rows = 7;

    for (int row = 0; row < rows; row++) {
        for (int col = 0; col < cols; col++) {
            
            // Lógica de diseño por nivel
            bool shouldCreate = true;
            
            // Ejemplo: En niveles pares, hacemos un patrón de "tablero de ajedrez"
            if (level % 2 == 0 && (row + col) % 2 == 0) shouldCreate = false;
            
            // Ejemplo: En nivel 3 y superiores, dejamos huecos centrales
            if (level >= 3 && col >= 2 && col <= 4 && row == 1) shouldCreate = false;

            if (shouldCreate) {
                int posX = offsetX + (col * spacingX);
                int posY = 40 + (row * spacingY);
                
                int brickHP = 1; // Valor por defecto

                if (level == 1) {
                    // NIVEL 1: Forzamos que todos sean dureza 1
                    brickHP = 1;
                } else {
                    // NIVELES SUPERIORES: Dureza progresiva
                    // Las filas de arriba son más duras, incrementado por el nivel
                    int baseHP = (rows - row); 
                    brickHP = baseHP + (level / 2) - 1;
                }

                // Asegurar rango válido entre 1 y 4 para evitar el color blanco
                if (brickHP > 4) brickHP = 4;
                if (brickHP < 1) brickHP = 1;

                BrickEntity* b = new BrickEntity(posX, posY, brickHP, this);
                bricks.push_back(b);
                addEntity(b);
            }
        }
    }
}

void BrickBreakerScene::resetBall() {
    ball->reset(paddle);
    gameStarted = false;
}

void BrickBreakerScene::update(unsigned long deltaTime) {
    // 1. ACTUALIZACIÓN AUTOMÁTICA (Estilo Godot)
    // Llama al update de todas las entidades (Ball, Paddle, Bricks, Labels)
    // Cada objeto ya sabe cómo moverse internamente.
    Scene::update(deltaTime);

    // 2. SISTEMA DE COLISIONES GENÉRICO
    // Procesa rebotes entre Ball-Paddle y Ball-Brick usando Layers/Masks
    collisionSystem.update();

    // 3. CONTROL DE ESTADOS DE LA ESCENA
    if (gameOver) {
        if (engine.getInputManager().isButtonPressed(0)) init();
        lblGameOver->setVisible(true);
        lblStartMessage->setVisible(false);
        return; // Detener lógica de juego si terminó
    }

    if (!gameStarted) {
        if (engine.getInputManager().isButtonPressed(0)) {
            gameStarted = true;
            ball->launch(120.0f, -120.0f); // Método nuevo en Ball
        }
        lblStartMessage->setVisible(true);
        lblGameOver->setVisible(false);
    } else {
        lblStartMessage->setVisible(false);
        lblGameOver->setVisible(false);

        // 4. LÓGICA DE VICTORIA / DERROTA (Reglas de la Escena)
        
        // Comprobar si quedan ladrillos activos
        bool levelCleared = true;
        for (auto& b : bricks) {
            if (b->active) {
                 levelCleared = false; 
                 break; 
            }
        }

        if (levelCleared) {
            currentLevel++;
            loadLevel(currentLevel); // Genera nuevos ladrillos
            resetBall();
        }

        // Condición de caída (Muerte)
        if (ball->y > engine.getRenderer().getHeight()) {
            lives--;
            if (lives <= 0) {
                gameOver = true;
            } else {
                resetBall();
            }
        }
    }
}

void BrickBreakerScene::draw(Renderer& renderer) {
    // Dibujar Ladrillos
    for (const auto& b : bricks) {
        if (b->active) {
            renderer.drawFilledRectangle(b->x, b->y, b->width, b->height, b->getColor());
        }
    }

    // --- Dibujar UI ---
    // 1. Puntuación (Mantenemos el texto a la izquierda)
    char scoreStr[16];
    snprintf(scoreStr, sizeof(scoreStr), "S: %d", score);
    renderer.drawText(scoreStr, 10, 5, COLOR_WHITE, 2);

    // 2. Vidas (Cuadritos rojos a la derecha superior)
    const int rectSize = 10;     // Un poco más grandes para que se vean bien
    const int spacing = 6;      
    const int marginRight = 6; 
    const int posY = 6;
    
    // Obtenemos el ancho real de la superficie de dibujo actual
    int currentWidth = renderer.getWidth(); 

    for (int i = 0; i < lives; i++) {
        // Calculamos X basándonos en el ancho real detectado
        int x = currentWidth - marginRight - (i + 1) * (rectSize + spacing);
        
        // Dibujamos el cuadrito
        renderer.drawFilledRectangle(x, posY, rectSize, rectSize, COLOR_RED);
    }

    Scene::draw(renderer); // Dibuja paddle y bola automáticamente
}