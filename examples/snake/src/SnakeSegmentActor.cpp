#include "SnakeSegmentActor.h"
#include "math/Scalar.h"
#include "math/Vector2.h"

namespace pr32 = pixelroot32;

namespace snake {

namespace gfx = pr32::graphics;

using CollisionLayer = pr32::physics::CollisionLayer;
using gfx::Color;
using pr32::math::toScalar;
using pr32::math::Vector2;



static constexpr CollisionLayer LAYER_SNAKE_HEAD = 1 << 0;
static constexpr CollisionLayer LAYER_SNAKE_BODY = 1 << 1;

SnakeSegmentActor::SnakeSegmentActor(int gridX, int gridY, bool head)
    : pr32::core::Actor(
        Vector2(toScalar(gridX * CELL_SIZE), toScalar(gridY * CELL_SIZE)),
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
    Color color = isHead ? Color::LightGreen : Color::DarkGreen;
    renderer.drawFilledRectangle(cellX * CELL_SIZE, cellY * CELL_SIZE, CELL_SIZE - 1, CELL_SIZE - 1, color);
}

pr32::core::Rect SnakeSegmentActor::getHitBox() {
    return { Vector2(toScalar(cellX * CELL_SIZE), toScalar(cellY * CELL_SIZE)), CELL_SIZE - 1, CELL_SIZE - 1 };
}

void SnakeSegmentActor::onCollision(pr32::core::Actor* other) {
    (void)other;
    if (isHead) {
        alive = false;
    }
}

void SnakeSegmentActor::setCellPosition(int gridX, int gridY) {
    cellX = gridX;
    cellY = gridY;
    position.x = toScalar(cellX * CELL_SIZE);
    position.y = toScalar(cellY * CELL_SIZE);
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
