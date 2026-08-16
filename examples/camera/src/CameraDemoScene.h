#pragma once
#include "core/Scene.h"
#include "graphics/Camera2D.h"
#include "graphics/CameraTween.h"

#include "GameConstants.h"

#include <memory>

namespace camerademo {

class PlayerCube;
class CameraDemoScene2;

/**
 * @class CameraDemoScene
 * @brief Side-scrolling platformer demo with Camera2D, parallax, camera
 *        effects and camera tweens.
 *
 * Features: Camera2D (smoothing, bounds), parallax layers, platformer
 * physics, shake/punch/offset effects, and a scripted camera pan.
 *
 * The effects and the tween share one camera with `followTarget()`, which is
 * the part worth reading: an effect is a per-frame *offset* applied at draw
 * time and never touches the camera position, so following keeps working
 * underneath it. A tween, by contrast, writes `camera.setPosition()` directly,
 * so following must be suspended while the tour runs or the two fight for the
 * same value every frame.
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

protected:
    void resetState() noexcept override;

private:
    static constexpr int OWNED_ENTITY_CAP = 8;

    /// Which effect BTN_EFFECT fires next. Cycles back to Shake after Offset.
    enum class EffectStep : uint8_t {
        Shake = 0,
        PunchUp,
        PunchDown,
        PunchLeft,
        PunchRight,
        Offset,
        COUNT
    };

    /// Stage of the scripted camera pan started by BTN_TWEEN.
    enum class TourStage : uint8_t {
        Idle = 0,   ///< No tour running; the camera follows the player.
        Out,        ///< Panning from the player to the target.
        Hold,       ///< Parked on the target.
        Back        ///< Panning back to where the player is now.
    };

    void fireNextEffect();
    void startCameraTour();
    void updateCameraTour(unsigned long deltaTime);
    void drawHud(pixelroot32::graphics::Renderer& renderer);

    pixelroot32::graphics::Camera2D camera;
    std::unique_ptr<PlayerCube> player;
    std::unique_ptr<pixelroot32::core::Entity> ownedEntities[OWNED_ENTITY_CAP];
    int entityCount = 0;
    float levelWidth;                 ///< World width in pixels
    bool jumpInputReady;              ///< Fire rate limiting for jump
    CameraDemoScene2* scene2Ref_;     ///< Reference to scene2 for end-of-level transition
    bool endReached_;                 ///< True once end-of-level transition is triggered

    pixelroot32::graphics::CameraTween<CAMERA_TWEEN_SLOTS> tweens;
    EffectStep nextEffect = EffectStep::Shake;
    const char* activeEffectName = nullptr;
    TourStage tourStage = TourStage::Idle;
    unsigned long tourHoldElapsed = 0;
    bool wasOnFloor = false;          ///< Previous-frame floor state, for the landing punch
};

}

