#include "BrickBreakerScene.h"
#include "Config.h"
#include "core/EDGE.h"
#include "particles/ParticlePresets.h"

extern EDGE engine;

// Configuración clásica de Brick Breaker
#define BRICK_WIDTH 30
#define BRICK_HEIGHT 12
#define PADDLE_W 40
#define PADDLE_H 8
#define BALL_SIZE 6
#define BORDER_TOP 20 // Espacio para el puntaje

void BrickBreakerScene::init() {
    clearEntities(); // Asegura que no haya entidades previas

    int sw = engine.getRenderer().getWidth();
    int sh = engine.getRenderer().getHeight();

    // Inicializar Paddle y Ball como Entidades
    paddle = new BrickPaddleEntity(sw/2 - PADDLE_W/2, sh - 20, PADDLE_W, PADDLE_H, false);
    ball = new BrickBallEntity(sw/2, sh - 30, BALL_SIZE/2, 150.0f);
    
    // Creamos el emisor con un pool de 50 partículas
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

                BrickEntity* b = new BrickEntity(posX, posY, brickHP);
                bricks.push_back(b);
                addEntity(b);
            }
        }
    }
}

void BrickBreakerScene::resetBall() {
    ball->x = paddle->x + paddle->width / 2;
    ball->y = paddle->y - ball->radius - 2;
    ball->vx = 0;
    ball->vy = 0;
    gameStarted = false;
}

void BrickBreakerScene::update(unsigned long deltaTime) {
    if (explosionEffect) {
        explosionEffect->update(deltaTime);
    }

    if(lblGameOver){
        lblGameOver->update(deltaTime);
    }

    if(lblStartMessage){
        lblStartMessage->update(deltaTime);
    }

    if (gameOver) {
        if (engine.getInputManager().isButtonPressed(0)) init();
        lblGameOver->setVisible(true);
        lblStartMessage->setVisible(false);
        return;
    }

    // 1. Control del Paddle (Limitado a la pantalla)
    paddle->velocity = 0;
    if (engine.getInputManager().isButtonDown(2)) paddle->velocity = -180.0f;
    if (engine.getInputManager().isButtonDown(1)) paddle->velocity = 180.0f;
    
    paddle->update(deltaTime);
    if (paddle->x < 0) paddle->x = 0;
    if (paddle->x + paddle->width > engine.getRenderer().getWidth()) 
        paddle->x = engine.getRenderer().getWidth() - paddle->width;

    // 2. Lógica de la Bola
    if (!gameStarted) {
        ball->x = paddle->x + paddle->width / 2;
        ball->y = paddle->y - ball->radius - 2;
        if (engine.getInputManager().isButtonPressed(0)) {
            gameStarted = true;
            ball->vx = 120.0f;
            ball->vy = -120.0f;
        }

        lblGameOver->setVisible(false);
        lblStartMessage->setVisible(true);
    } else {
        lblGameOver->setVisible(false);
        lblStartMessage->setVisible(false);

        ball->update(deltaTime);
        
        // Rebotes Paredes
        if (ball->x - ball->radius < 0 || ball->x + ball->radius > engine.getRenderer().getWidth()) {
            ball->vx *= -1;
        }
        if (ball->y - ball->radius < BORDER_TOP) {
            ball->vy *= -1;
            ball->y = BORDER_TOP + ball->radius;
        }

        // Rebote Paddle con angulación
        if (ball->vy > 0 && ball->y + ball->radius >= paddle->y && 
            ball->x >= paddle->x && ball->x <= paddle->x + paddle->width) {
            
            float hitPoint = (ball->x - (paddle->x + paddle->width / 2.0f)) / (paddle->width / 2.0f);
            ball->vx = hitPoint * 150.0f; // El ángulo depende de dónde toque
            ball->vy *= -1;
            ball->y = paddle->y - ball->radius - 1; 
        }

        checkBrickCollisions();

        // Verificar si ganó el nivel (si no quedan ladrillos visibles)
        bool levelCleared = true;
        for (const auto& b : bricks) {
            if (b->active) { levelCleared = false; break; }
        }

        if (levelCleared) {
            currentLevel++;
            loadLevel(currentLevel);
            resetBall();
        }

        // Perder vida
        if (ball->y > engine.getRenderer().getHeight()) {
            lives--;
            if (lives <= 0) gameOver = true;
            else resetBall();
        }
    }
}

void BrickBreakerScene::checkBrickCollisions() {
for (auto& b : bricks) {
        if (b->active) {
            // AABB Collision
            if (ball->x + ball->radius > b->x && ball->x - ball->radius < b->x + b->width &&
                ball->y + ball->radius > b->y && ball->y - ball->radius < b->y + b->height) {
                
                // optengo el color del ladrino antes de bajar la dureza.
                uint16_t  brickColor = b->getColor();
                b->hit();        // Baja la dureza y cambia color
                ball->vy *= -1;  // Rebote
                
                if (!b->active) {
                    // Disparamos 15 partículas en la posición del ladrillo
                    explosionEffect->burst(b->x + (b->width/2), b->y + (b->height/2), 15);
                    score += 50; // Bonus por destruir
                } else {
                    score += 10; // Puntos por golpe
                }
                break; 
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