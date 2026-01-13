#pragma once
#include "core/Actor.h"
#include "GameLayers.h"

class Ball : public Actor {
public:
    // Vectores de velocidad (si no están en la clase base Actor)
    float vx = 0;
    float vy = 0;
    
private:
    float radius;
    uint32_t color = 0xFFFFFFFF;
    bool isLaunched = false;
    Actor* paddleReference = nullptr;
    
    // Dimensiones del mundo para rebotes de pared
    float worldWidth;
    float borderTop;

public:
    Ball(float x, float y,float r, float worldW, float bTop) 
        : Actor(x, y, r * 2.0f, r * 2.0f), radius(r), worldWidth(worldW), borderTop(bTop) {
    
        // Identidad del objeto (Godot style)
        this->setCollisionLayer(Layers::BALL);
        // Con qué queremos chocar
        this->setCollisionMask(Layers::PADDLE | Layers::BRICK);
    }

    // Configuración inicial y estados
    void attachTo(Actor* paddle);
    void launch(float velocityX, float velocityY);
    void reset(Actor* paddle);

    // Getters básicos
    float getRadius() const { return radius; }
    bool getIsLaunched() const { return isLaunched; }

    // Métodos core del motor
    void update(unsigned long dt) override;
    void draw(Renderer& renderer) override;
    Rect getHitBox() override { return {x, y, width, height}; }
    void onCollision(Actor* other) override;
};