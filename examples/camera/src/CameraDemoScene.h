#pragma once
#include "core/Scene.h"
#include "graphics/Camera2D.h"

#include <memory>
#include <vector>

namespace camerademo {

class PlayerCube;

/**
 * @class CameraDemoScene
 * @brief Side-scrolling platformer demo with Camera2D and parallax.
 *
 * Features: Camera2D (smoothing, bounds), parallax layers, platformer physics.
 */
class CameraDemoScene : public pixelroot32::core::Scene {
public:
    CameraDemoScene();
    ~CameraDemoScene() override;

    void init() override;
    void update(unsigned long deltaTime) override;
    void draw(pixelroot32::graphics::Renderer& renderer) override;

private:
    pixelroot32::graphics::Camera2D camera;
    std::unique_ptr<PlayerCube> player;
    std::vector<std::unique_ptr<pixelroot32::core::Entity>> ownedEntities;
    float levelWidth;       ///< World width in pixels
    bool jumpInputReady;    ///< Fire rate limiting for jump
};

}

