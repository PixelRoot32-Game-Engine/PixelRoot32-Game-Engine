#include "BrickPaddleEntity.h"

BrickPaddleEntity::BrickPaddleEntity(int x, int y, int width, int height, bool isAI) 
    : Actor((float)x, (float)y, (float)width, (float)height),
      velocity(0),
      isAI(isAI), 
      color(0xFFFFFFFF) {}

void BrickPaddleEntity::update(unsigned long deltaTime) {
    // Movimiento basado en velocidad y tiempo (unidades por segundo)
    this->x += velocity * (deltaTime / 1000.0f);
}

void BrickPaddleEntity::draw(Renderer& renderer) {
    renderer.drawFilledRectangle((int)x, (int)y, width, height, color);
}

void BrickPaddleEntity::onCollision(Entity* other) {
    // Lógica de colisión del paddle (si es necesario)
}