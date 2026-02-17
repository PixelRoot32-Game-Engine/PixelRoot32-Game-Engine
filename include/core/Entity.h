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

namespace pixelroot32::core {

/**
 * @struct Rect
 * @brief Represents a 2D rectangle, typically used for hitboxes or bounds.
 * 
 * Uses adaptable Scalar type for coordinates to support both float and fixed-point math.
 */
struct Rect {
    pixelroot32::math::Scalar x, y;   ///< Top-left corner coordinates.
    int width, height; ///< Dimensions of the rectangle.

    /**
     * @brief Checks if this rectangle intersects with another.
     * @param other The other rectangle to check against.
     * @return true if the rectangles overlap, false otherwise.
     */
    bool intersects(const Rect& other) const {
        return !(x + width < other.x || x > other.x + other.width ||
                 y + height < other.y || y > other.y + other.height);
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
    pixelroot32::math::Scalar x, y;        ///< X and Y position in world space.
    int width, height; ///< Width and Height of the entity.
    EntityType type;   ///< The specific type of this entity.

    bool isVisible = true; ///< If false, the entity's draw method will not be called.
    unsigned char renderLayer = 1;

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

    /**
     * @brief Gets the current render layer.
     * @return The layer index (0-255).
     */
    unsigned char getRenderLayer() const { return renderLayer; }

    /**
     * @brief Sets the render layer.
     * @param layer The layer index (0-255). Lower layers are drawn first.
     */
    virtual void setRenderLayer(unsigned char layer) { renderLayer = layer; }

    /**
     * @brief Constructor.
     * @param x Initial X position.
     * @param y Initial Y position.
     * @param w Width.
     * @param h Height.
     * @param t EntityType.
     */
    Entity(pixelroot32::math::Scalar x, pixelroot32::math::Scalar y, int w, int h, EntityType t) 
        : x(x), y(y), width(w), height(h), type(t) {}
    
    /**
     * @brief Constructor with float coordinates for convenience.
     * Only enabled if Scalar is NOT float to avoid ambiguity.
     */
    template <typename T = float, typename std::enable_if<!std::is_same<pixelroot32::math::Scalar, T>::value, int>::type = 0>
    Entity(float x, float y, int w, int h, EntityType t) 
        : x(pixelroot32::math::toScalar(x)), y(pixelroot32::math::toScalar(y)), width(w), height(h), type(t) {}
        
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
