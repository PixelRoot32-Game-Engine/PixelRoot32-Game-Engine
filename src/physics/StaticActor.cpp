/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#include "physics/StaticActor.h"

namespace pixelroot32::physics {

StaticActor::StaticActor(pixelroot32::math::Scalar x, pixelroot32::math::Scalar y, int w, int h)
    : pixelroot32::core::PhysicsActor(x, y, w, h) {
    setBodyType(pixelroot32::core::PhysicsBodyType::STATIC);
}

StaticActor::StaticActor(pixelroot32::math::Vector2 position, int w, int h)
    : pixelroot32::core::PhysicsActor(position, w, h) {
    setBodyType(pixelroot32::core::PhysicsBodyType::STATIC);
}

void StaticActor::draw(pixelroot32::graphics::Renderer& renderer) {
    // Default behavior: call base Actor draw if needed, or implement specific logic
    (void)renderer;
}

} // namespace pixelroot32::physics
