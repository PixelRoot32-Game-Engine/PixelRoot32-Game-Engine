#include "BrickEntity.h"

#include "Config.h"

BrickEntity::BrickEntity(int x, int y, int hp) 
    : Actor(x, y, 30, 12),
        hp(hp),
        active(true) {}

uint16_t BrickEntity::getColor() {
    switch (this->hp) {
        case 4: return COLOR_BLUE; // Plateado/Azul Oscuro (Muy duro)
        case 3: return COLOR_RED; // Rojo (Duro)
        case 2: return COLOR_ORANGE; // Naranja (Medio)
        case 1: return COLOR_YELLOW; // Amarillo (Débil - 1 golpe más)
        default: return COLOR_WHITE;
    }
}

void BrickEntity::hit() {
    if (hp > 0) hp--;
    if (hp <= 0) active = false;
}

void BrickEntity::update(unsigned long deltaTime) {
    // Los ladrillos son estáticos, no necesitan lógica de movimiento
}

void BrickEntity::draw(Renderer& renderer) {
    if (active && hp > 0) {
        // Forzamos un color brillante para probar si se ven
        uint16_t color = getColor();
        
        // Asegúrate de que width y height tengan valores (ej: 30 y 12)
        renderer.drawFilledRectangle((int)x, (int)y, width, height, color);
    }
}

Rect BrickEntity::getHitBox() {
    return {x, y, width, height};
}

void BrickEntity::onCollision(Entity* other) {
    // Lógica de colisión del ladrillo (si es necesario)
}