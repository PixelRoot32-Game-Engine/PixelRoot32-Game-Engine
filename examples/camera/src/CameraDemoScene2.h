#pragma once
#include "core/Scene.h"
#include "graphics/Camera2D.h"

#include <memory>

namespace camerademo {

class PlayerCube;
class CameraDemoScene;

/**
 * @class CameraDemoScene2
 * @brief Second platformer scene with full player controls and back-transition.
 *
 * Visually distinct from CameraDemoScene (different tile patterns, parallax colors).
 * Player can move freely. Transition back to Scene1 via Iris when reaching left edge.
 */
class CameraDemoScene2 : public pixelroot32::core::Scene {
public:
    CameraDemoScene2();
    ~CameraDemoScene2() override;

    void init() override;
    void update(unsigned long deltaTime) override;
    void draw(pixelroot32::graphics::Renderer& renderer) override;

    /**
     * @brief Set the reference to Scene1 for back-transition.
     * @param scene1 Pointer to the CameraDemoScene instance.
     */
    void setScene1(CameraDemoScene* scene1) { scene1Ref_ = scene1; }

protected:
    void resetState() noexcept override;

private:
    static constexpr int OWNED_ENTITY_CAP = 8;

    pixelroot32::graphics::Camera2D camera;
    std::unique_ptr<PlayerCube> player;
    std::unique_ptr<pixelroot32::core::Entity> ownedEntities[OWNED_ENTITY_CAP];
    int entityCount = 0;
    float levelWidth;
    bool jumpInputReady;
    CameraDemoScene* scene1Ref_;
    bool backReached_;
};

}
