#include "PlayerActor.h"
#include "GameConstants.h"
#include "assets/PlayerSprites.h"
#include <core/Engine.h>
#include <platforms/EngineConfig.h>

namespace pr32 = pixelroot32;
extern pr32::core::Engine engine;

namespace spaceinvaders {

namespace core = pixelroot32::core;
namespace physics = pixelroot32::physics;
namespace gfx = pixelroot32::graphics;
namespace math = pixelroot32::math;

PlayerActor::PlayerActor(math::Vector2 position)
    : physics::KinematicActor(position, static_cast<int>(PLAYER_WIDTH), static_cast<int>(PLAYER_HEIGHT)), 
      isAlive(true),
      currentPosition(0),
      targetPosition(0),
      isMoving(false),
      moveTimer(0),
      startX(math::toScalar(0)),
      targetX(math::toScalar(0)),
      leftButtonPressed(false),
      rightButtonPressed(false),
      autoRepeatTimer(0),
      autoRepeatActive(false) {
    
    setShape(core::CollisionShape::AABB);

    currentPosition = calculatePositionFromX(position.x);
    targetPosition = currentPosition;
    position.x = calculateXFromPosition(currentPosition);
}

int PlayerActor::calculatePositionFromX(math::Scalar x) const {
    math::Scalar usableWidth = math::toScalar(DISPLAY_WIDTH - PLAYER_WIDTH);
    math::Scalar step = usableWidth / math::toScalar(NUM_POSITIONS - 1);
    
    int pos = static_cast<int>((x / step) + math::toScalar(0.5f));
    if (pos < 0) pos = 0;
    if (pos >= NUM_POSITIONS) pos = NUM_POSITIONS - 1;
    
    return pos;
}

math::Scalar PlayerActor::calculateXFromPosition(int pos) const {
    math::Scalar usableWidth = math::toScalar(DISPLAY_WIDTH - PLAYER_WIDTH);
    math::Scalar step = usableWidth / math::toScalar(NUM_POSITIONS - 1);
    math::Scalar x = math::toScalar(pos) * step;
    
    return x;
}

void PlayerActor::moveToPosition(int newPosition) {
    if (newPosition < 0) newPosition = 0;
    if (newPosition >= NUM_POSITIONS) newPosition = NUM_POSITIONS - 1;
    
    if (newPosition != targetPosition) {
        targetPosition = newPosition;
        isMoving = true;
        moveTimer = 0;
        startX = position.x;
        targetX = calculateXFromPosition(targetPosition);
    }
}

void PlayerActor::update(unsigned long deltaTime) {
    handleInput(deltaTime);
    
    if (isMoving) {
        updatePositionMovement(deltaTime);
    }
}

void PlayerActor::updatePositionMovement(unsigned long deltaTime) {
    moveTimer += deltaTime;

    if (moveTimer >= MOVE_DURATION_MS) {
        position.x = targetX;
        currentPosition = targetPosition;
        isMoving = false;
    } else {
        math::Scalar t = math::toScalar(moveTimer) / math::toScalar(MOVE_DURATION_MS);
        t = math::toScalar(1.0f) - (math::toScalar(1.0f) - t) * (math::toScalar(1.0f) - t) * (math::toScalar(1.0f) - t);  // Ease-out-cubic
        
        math::Scalar currentX = math::toScalar(startX) + (math::toScalar(targetX) - math::toScalar(startX)) * t;
        position.x = currentX;
    }
}

void PlayerActor::handleInput(unsigned long deltaTime) {
    auto& input = engine.getInputManager();
    
    bool leftDown = input.isButtonDown(BTN_LEFT);
    bool rightDown = input.isButtonDown(BTN_RIGHT);
    bool leftPressed = leftDown && !leftButtonPressed;
    bool rightPressed = rightDown && !rightButtonPressed;
    
    if (leftPressed && !rightDown) {
        moveToPosition(targetPosition - 1);
        autoRepeatActive = false;
        autoRepeatTimer = 0;
    } else if (rightPressed && !leftDown) {
        moveToPosition(targetPosition + 1);
        autoRepeatActive = false;
        autoRepeatTimer = 0;
    }

    if ((leftDown && !rightDown) || (rightDown && !leftDown)) {
        autoRepeatTimer += deltaTime;
        
        if (!autoRepeatActive) {
            if (autoRepeatTimer >= AUTO_REPEAT_DELAY_MS) {
                autoRepeatActive = true;
                autoRepeatTimer = 0;
                if (leftDown) {
                    moveToPosition(targetPosition - 1);
                } else {
                    moveToPosition(targetPosition + 1);
                }
            }
        } else {
            if (autoRepeatTimer >= AUTO_REPEAT_INTERVAL_MS) {
                autoRepeatTimer = 0;
                if (leftDown) {
                    moveToPosition(targetPosition - 1);
                } else {
                    moveToPosition(targetPosition + 1);
                }
            }
        }
    } else {
        autoRepeatActive = false;
        autoRepeatTimer = 0;
    }

    leftButtonPressed = leftDown;
    rightButtonPressed = rightDown;
}

bool PlayerActor::isFireDown() const {
    auto& input = engine.getInputManager();
    return input.isButtonDown(BTN_FIRE);
}

bool PlayerActor::wantsToShoot() const {
    auto& input = engine.getInputManager();
    return input.isButtonPressed(BTN_FIRE);
}

void PlayerActor::draw(gfx::Renderer& renderer) {
    if (!isAlive) return;
    
    renderer.drawSprite(PLAYER_SHIP_SPRITE,
                        static_cast<int>(position.x),
                        static_cast<int>(position.y),
                        SPRITE_SCALE,
                        SPRITE_SCALE,
                        gfx::Color::Yellow
                    );
}

void PlayerActor::onCollision(core::Actor* other) {
    (void)other;
}

}
