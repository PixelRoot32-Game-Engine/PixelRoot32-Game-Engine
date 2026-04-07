#include "PlayerActor.h"
#include "platforms/EngineConfig.h"
#include "GameConstants.h"
#include "core/Engine.h"
#include "assets/PlayerSprites.h"

namespace pr32 = pixelroot32;
extern pr32::core::Engine engine;

namespace spaceinvaders {

using Sprite = pr32::graphics::Sprite;


PlayerActor::PlayerActor(pixelroot32::math::Vector2 position)
    : KinematicActor(position, static_cast<int>(PLAYER_WIDTH), static_cast<int>(PLAYER_HEIGHT)), 
      isAlive(true),
      currentPosition(0),
      targetPosition(0),
      isMoving(false),
      moveTimer(0),
      startX(pr32::math::toScalar(0)),
      targetX(pr32::math::toScalar(0)),
      leftButtonPressed(false),
      rightButtonPressed(false),
      autoRepeatTimer(0),
      autoRepeatActive(false) {
    
    setShape(pr32::core::CollisionShape::AABB);

    currentPosition = calculatePositionFromX(position.x);
    targetPosition = currentPosition;
    position.x = calculateXFromPosition(currentPosition);
}

int PlayerActor::calculatePositionFromX(pixelroot32::math::Scalar x) const {
    float xFloat = static_cast<float>(x);
    float usableWidth = DISPLAY_WIDTH - PLAYER_WIDTH;
    float step = usableWidth / (NUM_POSITIONS - 1);
    
    int pos = static_cast<int>((xFloat / step) + 0.5f);
    if (pos < 0) pos = 0;
    if (pos >= NUM_POSITIONS) pos = NUM_POSITIONS - 1;
    
    return pos;
}

pr32::math::Scalar PlayerActor::calculateXFromPosition(int pos) const {
    float usableWidth = DISPLAY_WIDTH - PLAYER_WIDTH;
    float step = usableWidth / (NUM_POSITIONS - 1);
    float x = pos * step;
    
    return pr32::math::toScalar(x);
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
        float t = static_cast<float>(moveTimer) / static_cast<float>(MOVE_DURATION_MS);
        t = 1.0f - (1.0f - t) * (1.0f - t) * (1.0f - t);  // Ease-out-cubic
        
        float currentX = static_cast<float>(startX) + (static_cast<float>(targetX) - static_cast<float>(startX)) * t;
        position.x = pr32::math::toScalar(currentX);
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

void PlayerActor::draw(pr32::graphics::Renderer& renderer) {
    if (!isAlive) return;
    
    using Color = pr32::graphics::Color;
    renderer.drawSprite(PLAYER_SHIP_SPRITE,
                        static_cast<int>(position.x),
                        static_cast<int>(position.y),
                        SPRITE_SCALE,
                        SPRITE_SCALE,
                        Color::Yellow);
}

void PlayerActor::onCollision(pr32::core::Actor* other) {
    (void)other;
}

}
