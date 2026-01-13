#include "BrickBallEntity.h"
#include "Config.h"
#include <cmath>

BrickBallEntity::BrickBallEntity(int x, int y, float radius, int speed) 
    : Actor(x, y, (int)(radius * 2.0f), (int)(radius * 2.0f)),
        radius(radius), 
        speed(speed), 
        vx(0), 
        vy(0), 
        color(0xFFFF0000) {}

void BrickBallEntity::update(unsigned long deltaTime) {
    // Movimiento físico simple
    this->x += vx * (deltaTime / 1000.0f);
    this->y += vy * (deltaTime / 1000.0f);
}

void BrickBallEntity::draw(Renderer& renderer) {
    // Dibujamos la bola como un cuadrado pequeño (estilo retro)
    renderer.drawFilledCircle((int)x, (int)y, radius, COLOR_BLUE);
}

void BrickBallEntity::reset() {
    // Esta lógica se sobreescribe normalmente desde la Scene 
    // pero dejamos valores por defecto seguros
    this->vx = 0;
    this->vy = 0;
}

void BrickBallEntity::onCollision(Entity* other) {
    // Lógica de colisión simple: invertir la velocidad en Y
    this->vy = -this->vy;
}

