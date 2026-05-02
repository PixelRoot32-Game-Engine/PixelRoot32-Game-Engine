/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 */
#pragma once
#include "physics/StaticActor.h"

namespace pixelroot32::physics {

/**
 * @class SensorActor
 * @brief A static body that acts as a trigger: detects overlap but produces no physical response.
 *
 * Inherits from StaticActor.
 *
 * Use for collectibles, checkpoints, damage zones, or any area that should fire
 * onCollision() without pushing or blocking the other body.
 */
class SensorActor : public StaticActor {
public:
    /**
     * @brief Constructs a new SensorActor (static + sensor).
     */
    SensorActor(pixelroot32::math::Scalar x, pixelroot32::math::Scalar y, int w, int h);

    /**
     * @brief Constructs a new SensorActor with Vector2 position.
     */
    SensorActor(pixelroot32::math::Vector2 position, int w, int h);

    void draw(pixelroot32::graphics::Renderer& renderer) override;
};

} // namespace pixelroot32::physics
