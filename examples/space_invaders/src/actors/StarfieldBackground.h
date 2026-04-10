#pragma once
#include <core/Entity.h>
#include <graphics/Renderer.h>

namespace spaceinvaders {

class StarfieldBackground : public pixelroot32::core::Entity {
public:
    StarfieldBackground();

    void update(unsigned long deltaTime) override;
    void draw(pixelroot32::graphics::Renderer& renderer) override;
};

}