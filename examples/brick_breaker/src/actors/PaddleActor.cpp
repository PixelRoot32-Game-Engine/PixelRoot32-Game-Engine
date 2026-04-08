#include "PaddleActor.h"
#include <core/Engine.h>
#include "GameConstants.h"

namespace pr32 = pixelroot32;

extern pr32::core::Engine engine;

namespace brickbreaker {

namespace core = pr32::core;
namespace gfx = pr32::graphics;
namespace math = pr32::math;

PaddleActor::PaddleActor(math::Vector2 position, int w, int h, int sWidth)
    : KinematicActor(position, w, h), screenWidth(sWidth) 
{
    setShape(core::CollisionShape::AABB);
    setCollisionLayer(Layers::PADDLE);
    setCollisionMask(Layers::BALL);
}

void PaddleActor::update(unsigned long dt) {
    math::Scalar deltaTimeSec = math::toScalar(dt) / math::toScalar(1000.0f);
    math::Vector2 motion(0, 0);

    if (engine.getInputManager().isButtonDown(BTN_LEFT)) {
        motion.x = -speed * deltaTimeSec;
    }

    if (engine.getInputManager().isButtonDown(BTN_RIGHT)) {
        motion.x = speed * deltaTimeSec;
    }

    if (motion.x != 0 || motion.y != 0) {
        moveAndSlide(motion);
    }

    if (position.x < math::toScalar(0)) {
        position.x = math::toScalar(0);
    }
    
    if (position.x + math::toScalar(width) > math::toScalar(screenWidth)) {
        position.x = math::toScalar(screenWidth) - math::toScalar(width);
    }
}

void PaddleActor::draw(gfx::Renderer& renderer) {
    renderer.drawFilledRectangle((int)position.x, (int)position.y, width, height, color);
}

void PaddleActor::onCollision(core::Actor* other) {
    (void)other;
}

}
