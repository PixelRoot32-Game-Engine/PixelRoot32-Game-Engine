#include "Ball.h"
#include <Config.h>

void Ball::attachTo(Actor* paddle) {
    this->paddleReference = paddle;
    this->isLaunched = false;
    this->vx = 0;
    this->vy = 0;
}

void Ball::launch(float velocityX, float velocityY) {
    this->vx = velocityX;
    this->vy = velocityY;
    this->isLaunched = true;
    this->paddleReference = nullptr; // Liberamos la referencia al lanzar
}

void Ball::reset(Actor* paddle) {
    attachTo(paddle);
}

void Ball::update(unsigned long dt) {
    float deltaTimeSec = dt / 1000.0f;

    // ESTADO 1: La bola sigue al paddle (No lanzada)
    if (!isLaunched && paddleReference != nullptr) {
        this->x = paddleReference->x + (paddleReference->width / 2);
        this->y = paddleReference->y - (this->radius * 2) - 2;

        Actor::update(dt);
        return; 
    }

    // ESTADO 2: Movimiento físico (Lanzada)
    // 1. Aplicar movimiento
    x += vx * deltaTimeSec;
    y += vy * deltaTimeSec;

    // 2. Rebotes con paredes laterales
    if (x - radius < 0) {
        x = radius;
        vx *= -1;
    } else if (x + radius > worldWidth) {
        x = worldWidth - radius;
        vx *= -1;
    }

    // 3. Rebote con techo
    if (y - radius < borderTop) {
        y = borderTop + radius;
        vy *= -1;
    }

    Actor::update(dt);
}

void Ball::draw(Renderer& renderer) {
   renderer.drawFilledCircle((int)x, (int)y, radius, color);
   
}

void Ball::onCollision(Actor* other) {
    if (!isLaunched) return;

    // REBOTE CON PADDLE (Con angulación según punto de impacto)
    if (other->isInLayer(Layers::PADDLE)) {
        if (vy > 0) { // Solo rebotar si va bajando
            float hitPoint = (this->x - (other->x + other->width / 2.0f)) / (other->width / 2.0f);
            this->vx = hitPoint * 150.0f; // Ajusta la fuerza del ángulo
            this->vy *= -1;
            this->y = other->y - (this->radius * 2) - 1; // Corregir posición para evitar stuck
        }
    }

    // REBOTE CON LADRILLO
    if (other->isInLayer(Layers::BRICK)) {
        // Inversión simple de vector Y (puedes mejorar esto con lógica AABB lateral)
        this->vy *= -1;
        // Nota: El ladrillo se destruirá a sí mismo en su propio onCollision
    }
}