/*
 * PixelRoot32 Game Engine
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 *
 * ActorTouchController.h - Touch-based actor dragging controller
 * Enables dragging game Actors via touch screen with drag threshold
 */
#pragma once

#include <cstdint>
#include "input/TouchEvent.h"
#include "core/Actor.h"

namespace pixelroot32::input {

/**
 * @struct ActorPool
 * @brief Fixed-size pool for managing draggable actors
 * 
 * Uses a fixed array to avoid dynamic memory allocation.
 * Maximum 8 actors can be registered for touch dragging.
 */
struct ActorPool {
    /// Maximum number of actors in the pool
    static constexpr uint8_t kMaxActors = 8;
    
    /// Array of actor pointers
    pixelroot32::core::Actor* actors[kMaxActors];
    
    /// Current number of registered actors
    uint8_t count;
    
    /**
     * @brief Construct empty pool
     */
    ActorPool() : count(0) {
        for (uint8_t i = 0; i < kMaxActors; ++i) {
            actors[i] = nullptr;
        }
    }
};

/**
 * @class ActorTouchController
 * @brief Handles touch-based dragging of game actors
 * 
 * This controller manages a pool of actors that can be dragged via touch input.
 * It implements:
 * - Drag threshold (5 pixels) to ignore jitter
 * - Offset preservation (actor moves relative to initial touch position)
 * - Single drag (only one actor dragged at a time)
 * - Fixed pool (no dynamic memory allocation)
 * 
 * Usage:
 * @code
 * ActorTouchController controller;
 * controller.registerActor(&myActor);
 * // In game loop:
 * TouchEvent events[16];
 * uint8_t count = dispatcher.getEvents(events, 16);
 * for (uint8_t i = 0; i < count; i++) {
 *     controller.handleTouch(events[i]);
 * }
 * @endcode
 */
class ActorTouchController {
public:
    /// Drag threshold in pixels (ignore movement below this) - TEST: value = 5
    static constexpr int16_t kDragThreshold = 5;
    
    /**
     * @brief Construct the ActorTouchController
     */
    ActorTouchController();

    /**
     * @brief Clear registered actors and drag state (e.g. scene reset / arena recycle).
     */
    void reset();
    
    /**
     * @brief Destructor
     */
    ~ActorTouchController() = default;
    
    /**
     * @brief Register an actor to the drag pool
     * @param actor Pointer to the actor to register
     * @return true if registration succeeded, false if pool is full
     * 
     * Note: Does not check for duplicates - caller should ensure
     * the actor is not already registered.
     */
    bool registerActor(pixelroot32::core::Actor* actor);

    /**
     * @brief Expand hit-test rectangles by this many pixels on each side (0 = exact hitbox only).
     *        Useful when calibrated screen coords lag the visual sprite on resistive panels.
     */
    void setTouchHitSlop(int16_t expandPixels);

    /** @brief Current hit slop in pixels (per side). */
    int16_t getTouchHitSlop() const { return touchHitSlopPixels; }
    
    /**
     * @brief Unregister an actor from the drag pool
     * @param actor Pointer to the actor to unregister
     * @return true if actor was found and removed, false if not found
     */
    bool unregisterActor(pixelroot32::core::Actor* actor);
    
    /**
     * @brief Handle a touch event
     * @param event The touch event to process
     * 
     * Routes events based on type:
     * - TouchDown: Check for hit, begin drag if threshold exceeded
     * - DragMove: Update dragged actor position
     * - TouchUp: End drag
     */
    void handleTouch(const TouchEvent& event);
    
    /**
     * @brief Check if currently dragging an actor
     * @return true if a drag operation is in progress
     */
    bool isDragging() const;
    
    /**
     * @brief Get the currently dragged actor
     * @return Pointer to the dragged actor, nullptr if not dragging
     */
    pixelroot32::core::Actor* getDraggedActor() const;

    // For testing only - make hitTest accessible in test environment
    #if defined(PIXELROOT32_TESTING)
    /**
     * @brief Perform hit test to find actor at touch position
     * @param x X coordinate
     * @param y Y coordinate
     * @return Pointer to hit actor, nullptr if none hit
     */
    pixelroot32::core::Actor* hitTest(int16_t x, int16_t y);
    #endif
    
private:
    /**
     * @brief Perform hit test to find actor at touch position
     * @param x X coordinate
     * @param y Y coordinate
     * @return Pointer to hit actor, nullptr if none hit
     */
    #if !defined(PIXELROOT32_TESTING)
    pixelroot32::core::Actor* hitTest(int16_t x, int16_t y);
    #endif
    
    /**
     * @brief Check if a point is inside a rectangle, optionally expanded by @a slop pixels per side.
     */
    bool pointInRect(int16_t px, int16_t py, const pixelroot32::core::Rect& rect, int16_t slop = 0);
    
    /**
     * @brief Handle touch down event
     * @param event The touch down event
     */
    void onTouchDown(const TouchEvent& event);
    
    /**
     * @brief Handle drag move event
     * @param event The drag move event
     */
    void onTouchMove(const TouchEvent& event);
    
    /**
     * @brief Handle touch up event
     * @param event The touch up event
     */
    void onTouchUp(const TouchEvent& event);

    /**
     * @brief Handle drag start (movement exceeded threshold after TouchDown).
     *        If TouchDown missed the actor but the finger is now on one, start dragging.
     */
    void onDragStart(const TouchEvent& event);

    /// Pool of registered actors
    ActorPool actorPool;
    
    /// Currently dragged actor (nullptr if not dragging)
    pixelroot32::core::Actor* draggedActor;
    
    /// X offset from touch point to actor position (preserved during drag)
    int16_t dragOffsetX;
    
    /// Y offset from touch point to actor position (preserved during drag)
    int16_t dragOffsetY;
    
    /// Initial touch position when drag started
    int16_t initialTouchX;
    int16_t initialTouchY;
    
    /// Flag indicating drag threshold has been exceeded
    bool thresholdExceeded;

    /// Padding around each actor hitbox for touch picking (per side)
    int16_t touchHitSlopPixels;
};

} // namespace pixelroot32::input
