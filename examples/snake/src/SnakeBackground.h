#pragma once
#include "core/Entity.h"
#include "graphics/Renderer.h"
#include "GameConstants.h"


namespace snake {

/**
 * @class SnakeBackground
 * @brief Renders the game background and play area.
 *
 * Draws black background and red-bordered play area.
 */
class SnakeBackground : public pixelroot32::core::Entity {
public:
    SnakeBackground();

    void update(unsigned long deltaTime) override;
    void draw(pixelroot32::graphics::Renderer& renderer) override;
};

}