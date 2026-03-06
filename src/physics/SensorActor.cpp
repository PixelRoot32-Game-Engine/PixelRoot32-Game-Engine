/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#include "physics/SensorActor.h"

namespace pixelroot32::physics {

SensorActor::SensorActor(pixelroot32::math::Scalar x, pixelroot32::math::Scalar y, int w, int h)
    : StaticActor(x, y, w, h) {
    setSensor(true);
}

SensorActor::SensorActor(pixelroot32::math::Vector2 position, int w, int h)
    : StaticActor(position, w, h) {
    setSensor(true);
}

void SensorActor::draw(pixelroot32::graphics::Renderer& renderer) {
    (void)renderer;
}

} // namespace pixelroot32::physics
