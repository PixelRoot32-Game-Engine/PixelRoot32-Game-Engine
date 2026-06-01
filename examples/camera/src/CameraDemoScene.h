#pragma once
#include "core/Scene.h"
#include "graphics/Camera2D.h"

#include <memory>

namespace camerademo {

class PlayerCube;
class CameraDemoScene2;

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

    /**
     * @brief Set the reference to the second scene for end-of-level transition.
     * @param scene2 Pointer to the CameraDemoScene2 instance.
     */
    void setScene2(CameraDemoScene2* scene2) { scene2Ref_ = scene2; }

private:
    static constexpr int OWNED_ENTITY_CAP = 8;

    pixelroot32::graphics::Camera2D camera;
    std::unique_ptr<PlayerCube> player;
    std::unique_ptr<pixelroot32::core::Entity> ownedEntities[OWNED_ENTITY_CAP];
    int entityCount = 0;
    float levelWidth;                 ///< World width in pixels
    bool jumpInputReady;              ///< Fire rate limiting for jump
    CameraDemoScene2* scene2Ref_;     ///< Reference to scene2 for end-of-level transition
    bool endReached_;                 ///< True once end-of-level transition is triggered
};

}

