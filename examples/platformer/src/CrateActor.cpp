#include "CrateActor.h"

namespace platformerdemo {

namespace math = pixelroot32::math;

using math::toScalar;

CrateActor::CrateActor(math::Scalar x, math::Scalar y, int size)
    : RigidActor(x, y, size, size) {
    setShape(pixelroot32::core::CollisionShape::AABB);
    setMass(1.0f);
    setFriction(toScalar(0.5f));
    setRestitution(toScalar(0.1f));
    // Bounce off is what makes a crate rest instead of jittering on the shelf.
    setBounce(false);
    // RigidActor::integrate() adds a fixed 200 units/s^2 of world gravity. The
    // character uses 400, so the crates are scaled to fall at the same rate.
    setGravityScale(toScalar(2.0f));
}

void CrateActor::draw(pixelroot32::graphics::Renderer& renderer) {
    const int x = static_cast<int>(position.x);
    const int y = static_cast<int>(position.y);
    renderer.drawFilledRectangle(x, y, width, height, pixelroot32::graphics::Color::Orange);
    renderer.drawRectangle(x, y, width, height, pixelroot32::graphics::Color::DarkRed);
}

}
