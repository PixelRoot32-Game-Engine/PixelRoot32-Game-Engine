#pragma once
#include "core/Actor.h"
#include "GameLayers.h"


class BrickBreakerScene;

class BrickEntity : public Actor {
    BrickBreakerScene* scene;
public:
    int hp;      
    bool active;

    BrickEntity(int x, int y, int hp, BrickBreakerScene* scene);

    void update(unsigned long deltaTime) override;
    void draw(Renderer& renderer) override;
    void hit(); 
    uint16_t getColor();

    // Mejoramos el HitBox para que sea preciso
    Rect getHitBox() override { return {x, y, (float)width, (float)height}; }
    void onCollision(Actor* other) override;
};