// BrickEntity.cpp
#include "BrickEntity.h"
#include "BrickBreakerScene.h"
#include "Config.h"

BrickEntity::BrickEntity(int x, int y, int hp, BrickBreakerScene* scene) 
    : Actor(x, y, 30, 12), hp(hp), active(true), scene(scene) {
    this->setCollisionLayer(Layers::BRICK); //
}
uint16_t BrickEntity::getColor() {
    switch (this->hp) {
        case 4: return 0x7BEF;
        case 3: return 0xF800;
        case 2: return 0xFD20;
        case 1: return 0xFFE0;
        default: return 0xFFFF;
    }
}

void BrickEntity::hit() {
    if (hp > 0) hp--;
    if (hp <= 0) {
        active = false;
        // Aquí podrías marcar la entidad para ser destruida por el motor
    }
}

void BrickEntity::update(unsigned long deltaTime) {
    // (void)deltaTime; // Evita el warning de parámetro no usado
}

void BrickEntity::draw(Renderer& renderer) {
    if (!active || hp <= 0) return;

    uint16_t mainColor = getColor();
    
    // 1. Dibujar el cuerpo del ladrillo
    renderer.drawFilledRectangle((int)x, (int)y, width, height, mainColor);
    
    // 2. EFECTO VISUAL: Añadir un pequeño borde oscuro para dar relieve
    // Esto hace que los ladrillos se vean mucho mejor en pantalla
    renderer.drawRectangle((int)x, (int)y, width, height, 0x0000); // Borde negro
}

void BrickEntity::onCollision(Actor* other) {
    // PRIMERA DEFENSA: Si ya no está activo, ignorar cualquier colisión
    if (!active) return; 

    if (other->isInLayer(Layers::BALL)) {
        this->hit(); // Reduce HP y pone active=false si llega a 0
        
        if (!active) {
            // Efectos de explosión
            scene->getParticleEmiter()->burst(x + (width/2), y + (height/2), 15);
            scene->addScore(50);
            
            // SEGUNDA DEFENSA: Quitar las máscaras de colisión
            // Esto le dice al motor: "Ya no existo para la física"
            this->setCollisionLayer(0);
            this->setCollisionMask(0);
        } else {
            scene->addScore(10);
        }
    }
}