/*
 * PixelRoot32 Game Engine
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 *
 * ActorTouchController.cpp - Touch-based actor dragging controller implementation
 */
#include "input/ActorTouchController.h"
#include "core/Log.h"

namespace pixelroot32::input {

ActorTouchController::ActorTouchController()
    : draggedActor(nullptr)
    , dragOffsetX(0)
    , dragOffsetY(0)
    , initialTouchX(0)
    , initialTouchY(0)
    , thresholdExceeded(false)
    , touchHitSlopPixels(0) {
    // Pool is initialized by ActorPool default constructor
}

void ActorTouchController::setTouchHitSlop(int16_t expandPixels) {
    touchHitSlopPixels = expandPixels < 0 ? 0 : expandPixels;
}

void ActorTouchController::reset() {
    for (uint8_t i = 0; i < ActorPool::kMaxActors; ++i) {
        actorPool.actors[i] = nullptr;
    }
    actorPool.count = 0;
    draggedActor = nullptr;
    dragOffsetX = 0;
    dragOffsetY = 0;
    initialTouchX = 0;
    initialTouchY = 0;
    thresholdExceeded = false;
}

bool ActorTouchController::registerActor(pixelroot32::core::Actor* actor) {
    if (actor == nullptr) {
        return false;
    }
    
    if (actorPool.count >= ActorPool::kMaxActors) {
        return false;
    }
    
    actorPool.actors[actorPool.count] = actor;
    actorPool.count++;
    return true;
}

bool ActorTouchController::unregisterActor(pixelroot32::core::Actor* actor) {
    if (actor == nullptr) {
        return false;
    }
    
    // Find the actor in the pool
    int8_t foundIndex = -1;
    for (uint8_t i = 0; i < actorPool.count; ++i) {
        if (actorPool.actors[i] == actor) {
            foundIndex = static_cast<int8_t>(i);
            break;
        }
    }
    
    if (foundIndex == -1) {
        return false;
    }
    
    // If this is the dragged actor, cancel the drag
    if (draggedActor == actor) {
        draggedActor = nullptr;
        thresholdExceeded = false;
    }
    
    // Remove actor and shift remaining actors
    for (uint8_t i = static_cast<uint8_t>(foundIndex); i < actorPool.count - 1; ++i) {
        actorPool.actors[i] = actorPool.actors[i + 1];
    }
    actorPool.actors[actorPool.count - 1] = nullptr;
    actorPool.count--;
    
    return true;
}

void ActorTouchController::handleTouch(const TouchEvent& event) {
    pixelroot32::core::logging::log("[ATC] handleTouch type=%u x=%d y=%d pool=%u",
        static_cast<uint8_t>(event.type), event.x, event.y, actorPool.count);

    switch (event.type) {
        case TouchEventType::TouchDown:
            onTouchDown(event);
            break;

        case TouchEventType::DragStart:
            onDragStart(event);
            break;

        case TouchEventType::DragMove:
            onTouchMove(event);
            break;

        case TouchEventType::TouchUp:
        case TouchEventType::DragEnd:
            onTouchUp(event);
            break;

        default:
            break;
    }
}

bool ActorTouchController::isDragging() const {
    return thresholdExceeded && (draggedActor != nullptr);
}

pixelroot32::core::Actor* ActorTouchController::getDraggedActor() const {
    // Return actor if we have one, regardless of threshold
    // (allows checking which actor was hit even before threshold exceeded)
    return draggedActor;
}

pixelroot32::core::Actor* ActorTouchController::hitTest(int16_t x, int16_t y) {
    // Iterate from top to bottom (last registered = on top)
    for (int8_t i = static_cast<int8_t>(actorPool.count) - 1; i >= 0; --i) {
        pixelroot32::core::Actor* actor = actorPool.actors[static_cast<uint8_t>(i)];
        if (actor == nullptr) {
            continue;
        }
        
        pixelroot32::core::Rect hitBox = actor->getHitBox();
        if (pointInRect(x, y, hitBox, touchHitSlopPixels)) {
            return actor;
        }
    }
    
    return nullptr;
}

bool ActorTouchController::pointInRect(int16_t px, int16_t py, const pixelroot32::core::Rect& rect,
                                       int16_t slop) {
    int rectX = static_cast<int>(static_cast<float>(rect.position.x));
    int rectY = static_cast<int>(static_cast<float>(rect.position.y));

    return (px >= rectX - slop && px < rectX + rect.width + slop && py >= rectY - slop &&
            py < rectY + rect.height + slop);
}

void ActorTouchController::onTouchDown(const TouchEvent& event) {
    initialTouchX = event.x;
    initialTouchY = event.y;

    for (uint8_t i = 0; i < actorPool.count; ++i) {
        auto* a = actorPool.actors[i];
        if (a == nullptr) continue;
        auto hb = a->getHitBox();
        int hx = static_cast<int>(static_cast<float>(hb.position.x));
        int hy = static_cast<int>(static_cast<float>(hb.position.y));
        pixelroot32::core::logging::log("[ATC] pool[%u] hb=(%d,%d %dx%d) touch=(%d,%d)",
            i, hx, hy, hb.width, hb.height, event.x, event.y);
    }

    pixelroot32::core::Actor* hitActor = hitTest(event.x, event.y);
    pixelroot32::core::logging::log("[ATC] hitTest => %s", hitActor ? "HIT" : "MISS");

    if (hitActor != nullptr) {
        draggedActor = hitActor;
        
        // Calculate offset from touch point to actor position
        int actorX = static_cast<int>(static_cast<float>(hitActor->position.x));
        int actorY = static_cast<int>(static_cast<float>(hitActor->position.y));
        
        dragOffsetX = static_cast<int16_t>(actorX - event.x);
        dragOffsetY = static_cast<int16_t>(actorY - event.y);
        
        // Reset threshold - will be set to true when exceeded
        thresholdExceeded = false;
    } else {
        draggedActor = nullptr;
        thresholdExceeded = false;
    }
}

void ActorTouchController::onDragStart(const TouchEvent& event) {
    if (draggedActor != nullptr) {
        thresholdExceeded = true;
        return;
    }

    initialTouchX = event.x;
    initialTouchY = event.y;

    pixelroot32::core::Actor* hitActor = hitTest(event.x, event.y);
    if (hitActor == nullptr) {
        return;
    }

    draggedActor = hitActor;
    const int actorX = static_cast<int>(static_cast<float>(hitActor->position.x));
    const int actorY = static_cast<int>(static_cast<float>(hitActor->position.y));
    dragOffsetX = static_cast<int16_t>(actorX - event.x);
    dragOffsetY = static_cast<int16_t>(actorY - event.y);
    thresholdExceeded = true;
}

void ActorTouchController::onTouchMove(const TouchEvent& event) {
    // If we have a dragged actor and threshold not yet exceeded, check if we should start
    if (draggedActor != nullptr && !thresholdExceeded) {
        // Calculate distance from initial touch position
        int16_t dx = event.x - initialTouchX;
        int16_t dy = event.y - initialTouchY;
        
        // Use squared Euclidean distance to avoid sqrt
        // Distance formula: sqrt(dx*dx + dy*dy), compare to threshold
        int32_t distSq = static_cast<int32_t>(dx) * dx + static_cast<int32_t>(dy) * dy;
        int32_t thresholdSq = static_cast<int32_t>(kDragThreshold) * kDragThreshold;
        
        // Threshold check: if distance >= threshold, start dragging
        // Using >= so exactly 5px movement triggers drag
        if (distSq >= thresholdSq) {
            thresholdExceeded = true;
        }
    }
    
    if (thresholdExceeded && draggedActor != nullptr) {
        pixelroot32::core::logging::log("[ATC] DRAG newPos=(%d,%d)", event.x + dragOffsetX, event.y + dragOffsetY);
        // New position = current touch position + preserved offset
        // This makes actor move relative to initial touch (not absolute position)
        int16_t newX = event.x + dragOffsetX;
        int16_t newY = event.y + dragOffsetY;
        
        draggedActor->position.x = pixelroot32::math::toScalar(static_cast<float>(newX));
        draggedActor->position.y = pixelroot32::math::toScalar(static_cast<float>(newY));
    }
}

void ActorTouchController::onTouchUp(const TouchEvent& event) {
    (void)event; // Unused - we just clear state
    
    // Clear drag state
    draggedActor = nullptr;
    thresholdExceeded = false;
    dragOffsetX = 0;
    dragOffsetY = 0;
    initialTouchX = 0;
    initialTouchY = 0;
}

} // namespace pixelroot32::input
