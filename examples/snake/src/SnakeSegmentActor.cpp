#include "SnakeSegmentActor.h"
#include "math/Scalar.h"
#include "math/Vector2.h"

namespace pr32 = pixelroot32;

namespace snake {

namespace gfx = pr32::graphics;
namespace core = pr32::core;
namespace physics = pr32::physics;
namespace math = pr32::math;

static constexpr physics::CollisionLayer LAYER_SNAKE_HEAD = 1 << 0;
static constexpr physics::CollisionLayer LAYER_SNAKE_BODY = 1 << 1;

SnakeSegmentActor::SnakeSegmentActor(int gridX, int gridY, bool head)
    : pr32::core::Actor(
        math::Vector2(math::toScalar(gridX * CELL_SIZE), math::toScalar(gridY * CELL_SIZE)),
        CELL_SIZE - 1, CELL_SIZE - 1),
      cellX(gridX),
      cellY(gridY),
      isHead(head),
      alive(true) {
    setHead(head);
}

void SnakeSegmentActor::update(unsigned long deltaTime) {
    (void)deltaTime;
}

void SnakeSegmentActor::draw(gfx::Renderer& renderer) {
    gfx::Color color = isHead ? gfx::Color::LightGreen : gfx::Color::DarkGreen;
    renderer.drawFilledRectangle(cellX * CELL_SIZE, cellY * CELL_SIZE, CELL_SIZE - 1, CELL_SIZE - 1, color);
}

core::Rect SnakeSegmentActor::getHitBox() {
    return { 
        math::Vector2(
            math::toScalar(cellX * CELL_SIZE),
            math::toScalar(cellY * CELL_SIZE)
        ),
        CELL_SIZE - 1,
        CELL_SIZE - 1
    };
}

void SnakeSegmentActor::onCollision(core::Actor* other) {
    (void)other;
    if (isHead) {
        alive = false;
    }
}

void SnakeSegmentActor::setCellPosition(int gridX, int gridY) {
    cellX = gridX;
    cellY = gridY;
    position.x = math::toScalar(cellX * CELL_SIZE);
    position.y = math::toScalar(cellY * CELL_SIZE);
}

int SnakeSegmentActor::getCellX() const {
    return cellX;
}

int SnakeSegmentActor::getCellY() const {
    return cellY;
}

void SnakeSegmentActor::setHead(bool head) {
    isHead = head;
    if (isHead) {
        setCollisionLayer(LAYER_SNAKE_HEAD);
        setCollisionMask(LAYER_SNAKE_BODY);
    } else {
        setCollisionLayer(LAYER_SNAKE_BODY);
        setCollisionMask(0);
    }
}

void SnakeSegmentActor::resetAlive() {
    alive = true;
}

bool SnakeSegmentActor::isAlive() const {
    return alive;
}

}
