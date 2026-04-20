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
#include <type_traits>

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
    friend class Scene;  // Scene accesses protected update/draw/renderLayer

public:
    pixelroot32::math::Vector2 position;        ///< Position in world space.
    int width, height; ///< Width and Height of the entity.
    EntityType type;   ///< The specific type of this entity.

    bool isVisible = true; ///< If false, the entity's draw method will not be called.
    
protected:
    unsigned char renderLayer = 1;
    Rect dirtyBounds_;  ///< Dirty region for partial updates (initialized in constructor).
    bool autoMarkDirty_ = true;  ///< Auto-mark dirty after draw() (default: enabled).

public:
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

    // ========================================================================
    // Dirty tracking API
    // ========================================================================

    /**
     * @brief Sets the dirty bounds region for auto-marking.
     * @param x X offset from entity position.
     * @param y Y offset from entity position.
     * @param w Width of dirty region.
     * @param h Height of dirty region.
     */
    void setDirtyBounds(int x, int y, int w, int h) {
        dirtyBounds_.position.x = pixelroot32::math::toScalar(x);
        dirtyBounds_.position.y = pixelroot32::math::toScalar(y);
        dirtyBounds_.width = w;
        dirtyBounds_.height = h;
    }

    /**
     * @brief Gets the dirty bounds region.
     * @return The Rect representing the dirty region.
     */
    Rect getDirtyBounds() const { return dirtyBounds_; }

    /**
     * @brief Enables or disables automatic dirty marking after draw.
     * @param enabled true to enable auto-marking, false to disable.
     */
    void setAutoMarkDirty(bool enabled) { autoMarkDirty_ = enabled; }

    /**
     * @brief Checks if automatic dirty marking is enabled.
     * @return true if auto-marking is enabled.
     */
    bool isAutoMarkDirty() const { return autoMarkDirty_; }

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
        : position(pos), width(w), height(h), type(t), 
          dirtyBounds_(Rect{{pixelroot32::math::toScalar(0), pixelroot32::math::toScalar(0)}, w, h}) {}

    /**
     * @brief Constructor.
     * @param x Initial X position.
     * @param y Initial Y position.
     * @param w Width.
     * @param h Height.
     * @param t EntityType.
     */
    Entity(pixelroot32::math::Scalar x, pixelroot32::math::Scalar y, int w, int h, EntityType t) 
        : position(x, y), width(w), height(h), type(t),
          dirtyBounds_(Rect{{pixelroot32::math::toScalar(0), pixelroot32::math::toScalar(0)}, w, h}) {}
    
    /**
     * @brief Constructor with float coordinates for convenience.
     * Only enabled if Scalar is NOT float to avoid ambiguity.
     */
    template <typename T = float, typename std::enable_if<!std::is_same<pixelroot32::math::Scalar, T>::value, int>::type = 0>
    Entity(float x, float y, int w, int h, EntityType t) 
        : position(pixelroot32::math::toScalar(x), pixelroot32::math::toScalar(y)), width(w), height(h), type(t),
          dirtyBounds_(Rect{{pixelroot32::math::toScalar(0), pixelroot32::math::toScalar(0)}, w, h}) {}

        
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
