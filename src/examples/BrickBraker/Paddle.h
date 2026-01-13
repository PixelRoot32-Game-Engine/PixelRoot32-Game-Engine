#pragma once
#include "core/Actor.h"
#include "input/InputManager.h"
#include "GameLayers.h"

class Paddle : public Actor {
private:
    float speed = 200.0f;           // Pixeles por segundo
    uint32_t color = 0xFFFFFFFF;    // Color
    InputManager& input;            // Referencia al gestor de entrada del motor
    float screenWidth;              // Para limitar el movimiento

public:
    Paddle(float x, float y, int w, int h, InputManager& inputMgr, float sWidth) 
        : Actor(x, y, w, h), input(inputMgr), screenWidth(sWidth) {
        
        // Configuración de Colisión tipo Godot
        this->setCollisionLayer(Layers::PADDLE);
        this->setCollisionMask(Layers::BALL); // El paddle detecta a la bola
    }

    // Métodos core del motor
    void update(unsigned long dt) override;
    void draw(Renderer& renderer) override;
    Rect getHitBox() override { return {x, y, width, height}; }
    void onCollision(Actor* other) override;
};