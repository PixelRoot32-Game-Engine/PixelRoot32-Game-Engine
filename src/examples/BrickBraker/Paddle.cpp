#include "Paddle.h"
#include <EDGE.h>

extern EDGE engine;

void Paddle::update(unsigned long dt) {
    // 1. Convertir Delta Time a segundos (float)
    float deltaTimeSec = dt / 1000.0f;

    // 2. Leer Input y aplicar velocidad
    // Usamos los IDs de botones que definiste (1 y 2)
    if (input.isButtonDown(2)) { // Izquierda
        x -= speed * deltaTimeSec;
    }

    if (input.isButtonDown(1)) { // Derecha
        x += speed * deltaTimeSec;
    }

    // 3. Limitar Bordes (Clamping)
    // Evita que el paddle "salga" de la pantalla física
    if (x < 0) {
        x = 0;
    }
    
    if (x + width > screenWidth) {
        x = screenWidth - width;
    }
}

void Paddle::draw(Renderer& renderer) {
    renderer.drawFilledRectangle((int)x, (int)y, width, height, color);
}

void Paddle::onCollision(Actor* other) {}