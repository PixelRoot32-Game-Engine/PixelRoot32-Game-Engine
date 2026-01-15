// BrickEntity.cpp
#include "BrickActor.h"
#include "BrickBreakerScene.h"
#include "Config.h"

using namespace pixelroot32::core;
using namespace pixelroot32::graphics;

namespace brickbreaker {

BrickActor::BrickActor(int x, int y, int hp, BrickBreakerScene* scene) 
    : Actor(x, y, 30, 12), scene(scene), hp(hp), active(true) {
    this->setCollisionLayer(Layers::BRICK); //
}
pixelroot32::graphics::Color BrickActor::getColor() {
    switch (this->hp) {
        case 4: return Color::DarkGray;
        case 3: return Color::Red;
        case 2: return Color::Orange;
        case 1: return Color::Yellow;
        default: return Color::White;
    }
}

void BrickActor::hit() {
    if (hp > 0) hp--;
    if (hp <= 0) {
        active = false;
        // Aquí podrías marcar la entidad para ser destruida por el motor
    }
}

void BrickActor::update(unsigned long deltaTime) {
    (void)deltaTime;
    // Evita el warning de parámetro no usado
}

void BrickActor::draw(Renderer& renderer) {
    if (!active || hp <= 0) return;

    Color mainColor = getColor();
    
    // 1. Dibujar el cuerpo del ladrillo
    renderer.drawFilledRectangle((int)x, (int)y, width, height, mainColor);
    
    // 2. EFECTO VISUAL: Añadir un pequeño borde oscuro para dar relieve
    // Esto hace que los ladrillos se vean mucho mejor en pantalla
    renderer.drawRectangle((int)x, (int)y, width, height, Color::Black); // Borde negro
}

void BrickActor::onCollision(Actor* other) {
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

}