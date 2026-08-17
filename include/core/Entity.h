/*
 * Original work:
 * Copyright (c) nbourre
 * Licensed under the MIT License
 *
 * Modifications:
 * Copyright (c) 2026 PixelRoot32
 *
 * This file remains licensed under the MIT License.
 */
#pragma once

#include "graphics/Renderer.h"
#include "math/Scalar.h"
#include "math/Vector2.h"
#include "platforms/EngineConfig.h"

#include <cstdint>

namespace pixelroot32::core {

/**
 * @struct Rect
 * @brief Represents a 2D rectangle, typically used for hitboxes or bounds.
 * 
 * Uses adaptable Scalar type for coordinates to support both float and fixed-point math.
 */
struct Rect {
    pixelroot32::math::Vector2 position;   ///< Top-left corner coordinates.
    int width, height; ///< Dimensions of the rectangle.

    /**
     * @brief Checks if this rectangle intersects with another.
     * @param other The other rectangle to check against.
     * @return true if the rectangles overlap, false otherwise.
     */
    bool intersects(const Rect& other) const {
        return !(position.x + width < other.position.x || position.x > other.position.x + other.width ||
                 position.y + height < other.position.y || position.y > other.position.y + other.height);
    }
};

/**
 * @enum EntityType
 * @brief Categorizes entities for type-safe casting and logic differentiation.
 */
enum class EntityType { GENERIC, ACTOR, UI_ELEMENT };

/**
 * @class Entity
 * @brief Abstract base class for all game objects.
 *
 * Entities are the fundamental building blocks of the scene. They have a position,
 * size, and lifecycle methods (update, draw).
 * 
 * Uses adaptable Scalar type for position to ensure consistent physics across platforms.
 */
class Entity {
public:
    pixelroot32::math::Vector2 position;        ///< Position in world space.
    int width, height; ///< Width and Height of the entity.
    EntityType type;   ///< The specific type of this entity.

    bool isVisible = true; ///< If false, the entity's draw method will not be called.
    
    /**
     * @brief Sets the visibility of the entity.
     * @param v true to show, false to hide.
     */
    virtual void setVisible(bool v) { isVisible = v; }

    bool isEnabled = true; ///< If false, the entity's update method will not be called.

    /**
     * @brief Sets the enabled state of the entity.
     * @param e true to enable, false to disable.
     */
    virtual void setEnabled(bool e) { isEnabled = e; }

protected:
    unsigned char renderLayer = 1;

public:
#if PIXELROOT32_ENABLE_DEPTH_SORT
    /**
     * @brief Paint-order key, written by game code and only ever COMPARED by
     *        the engine (see gameplay::compareByDepthKey).
     *
     * Exists so draw order can be expressed independently of any coordinate
     * system. Ordering by `position.y + height` is correct only while screen
     * depth is a monotone function of world Y — true for an axis-aligned
     * top-down game, false under any non-identity projection (isometric,
     * oblique), where two cells can share a screen row at different world Ys.
     * Such a game writes `depthKey` from its projected anchor instead.
     *
     * The engine never assigns this field, and never derives it: `core` must
     * not know about a projection. A default of `0` means a game that ignores
     * it gets stable layer-only ordering rather than a wrong inherited answer
     * — which is the reason this is a field and not a virtual getter with a
     * `position.y + height` default.
     *
     * Cost: `Entity` has a single trailing pad byte and an `int16_t` cannot
     * occupy it, so this grows the object by 4 bytes on a 32-bit target
     * (28 -> 32) and 8 on 64-bit native. At MAX_ENTITIES = 64 that is 256 B,
     * paid only by builds that opt into depth sorting.
     */
    int16_t depthKey = 0;
#endif

    /**
     * @brief Gets the current render layer.
     * @return The layer index (0-255).
     */
    unsigned char getRenderLayer() const { return renderLayer; }

    /**
     * @brief Sets the render layer.
     * @param layer The layer index (0 to MaxLayers-1). Clamped if exceeded.
     */
    virtual void setRenderLayer(unsigned char layer) { 
        if (layer >= pixelroot32::platforms::config::MaxLayers) {
            renderLayer = static_cast<unsigned char>(pixelroot32::platforms::config::MaxLayers - 1);
        } else {
            renderLayer = layer; 
        }
    }

    /**
     * @brief Constructor.
     * @param position Initial position.
     * @param w Width.
     * @param h Height.
     * @param t EntityType.
     */
    Entity(pixelroot32::math::Vector2 pos, int w, int h, EntityType t) 
        : position(pos), width(w), height(h), type(t) {}

    /**
     * @brief Constructor.
     * @param x Initial X position.
     * @param y Initial Y position.
     * @param w Width.
     * @param h Height.
     * @param t EntityType.
     */
    Entity(pixelroot32::math::Scalar x, pixelroot32::math::Scalar y, int w, int h, EntityType t) 
        : position(x, y), width(w), height(h), type(t) {}
    
    /**
     * @brief Constructor with float coordinates for convenience.
     * Only enabled if Scalar is NOT float to avoid ambiguity.
     */
    template <typename T = float, typename std::enable_if<!std::is_same<pixelroot32::math::Scalar, T>::value, int>::type = 0>
    Entity(float x, float y, int w, int h, EntityType t) 
        : position(pixelroot32::math::toScalar(x), pixelroot32::math::toScalar(y)), width(w), height(h), type(t) {}

        
    virtual ~Entity() {}

    /**
     * @brief Updates the entity's logic.
     * @param deltaTime Time elapsed since the last frame in milliseconds.
     */
    virtual void update(unsigned long deltaTime) = 0;

    /**
     * @brief Renders the entity.
     * @param renderer Reference to the renderer to use for drawing.
     */
    virtual void draw(pixelroot32::graphics::Renderer& renderer) = 0;
};

}
