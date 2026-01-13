#pragma once
#include "core/Actor.h"

class BrickEntity : public Actor {
public:
    int hp;      // Dureza actual (1, 2, 3 o 4)
    bool active;

    BrickEntity(int x, int y, int hp);

    void update(unsigned long deltaTime) override;
    void draw(Renderer& renderer) override;
    void hit(); // Método para reducir dureza
    uint16_t getColor();

    Rect getHitBox() override;
    void onCollision(Entity* other) override;
};