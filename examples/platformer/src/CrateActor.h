#pragma once
#include "graphics/Renderer.h"
#include "physics/RigidActor.h"

namespace platformerdemo {

/**
 * @class CrateActor
 * @brief `RigidActor` prop: falls under gravity, rests on the shelf, can be pushed.
 *
 * Godot equivalent: a `RigidBody2D`.
 *
 * A kinematic body may push a rigid one, never the other way round: the
 * character's path is resolved by `moveAndSlide()` and a crate that touches it
 * receives the impulse. Standing on a crate is therefore not stable, which is
 * why the shelf is the only surface the crates are meant to sit on.
 */
class CrateActor : public pixelroot32::physics::RigidActor {
public:
    /**
     * @param x Left edge in world pixels.
     * @param y Top edge in world pixels.
     * @param size Width and height in pixels (crates are square).
     */
    CrateActor(pixelroot32::math::Scalar x, pixelroot32::math::Scalar y, int size);

    void draw(pixelroot32::graphics::Renderer& renderer) override;
};

}
