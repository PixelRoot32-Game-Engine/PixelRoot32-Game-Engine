#include "PipeActor.h"
#include "GameLayers.h"

using namespace pixelroot32::math;
using namespace pixelroot32::physics;
using namespace pixelroot32::core;
using namespace pixelroot32::graphics;

namespace flappy {

PipeActor::PipeActor(Vector2 pos, int width, int height, bool isTop) 
    : KinematicActor(pos, width, height), isTopPipe(isTop) {
    
    setShape(CollisionShape::AABB);
    setCollisionLayer(Layers::PIPE);
    setCollisionMask(Layers::BIRD);
}

void PipeActor::update(unsigned long deltaTime) {
    unsigned long clampedDT = (deltaTime > 100) ? 16 : deltaTime;
    float dtFactor = clampedDT / 16.67f;
    
    position.x -= toScalar(PIPE_SPEED * dtFactor);
}

void PipeActor::draw(Renderer& renderer) {
    renderer.drawFilledRectangle(static_cast<int>(position.x), 
                                 static_cast<int>(position.y), 
                                 width, height, Color::White);
}

bool PipeActor::resetIfOffScreen(int screenWidth, int screenHeight, int newGapY) {
    if (static_cast<int>(position.x) + width < 0) {
        position.x = toScalar((float)screenWidth);
        passed = false;
        
        if (isTopPipe) {
            position.y = toScalar(0);
            setSize(PIPE_WIDTH, newGapY);
        } else {
            position.y = toScalar((float)(newGapY + PIPE_GAP));
            setSize(PIPE_WIDTH, screenHeight - (newGapY + PIPE_GAP));
        }
        return true;
    }
    return false;
}

void PipeActor::setSize(int width, int height) {
    this->width = width;
    this->height = height;
}

bool PipeActor::isPassed(Vector2 birdPos) const {
    if (!passed && isTopPipe) {
        if (static_cast<float>(position.x) + width < static_cast<float>(birdPos.x)) {
            return true;
        }
    }
    return false;
}

} // namespace flappy
