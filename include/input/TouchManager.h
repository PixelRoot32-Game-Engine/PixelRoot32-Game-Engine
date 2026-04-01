/*
 * PixelRoot32 Game Engine
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 *
 * TouchManager - Touch event aggregation layer (controller-agnostic)
 * Receives normalized TouchPoints from adapter, provides to UI
 */
#pragma once

#ifdef PLATFORM_NATIVE
    #include <platforms/mock/MockArduino.h>
#else
    #include <Arduino.h>
#endif

#include <input/TouchPoint.h>
#include <input/TouchAdapter.h>
#include <input/TouchEventDispatcher.h>
#include <core/Log.h>

#if defined(TOUCH_DRIVER_XPT2046)
#include <input/adapters/XPT2046Adapter.h>
#elif defined(TOUCH_DRIVER_GT911)
#include <input/adapters/GT911Adapter.h>
#endif

namespace pixelroot32::input {

/**
 * @class TouchManager
 * @brief Touch event aggregation layer
 *
 * This class is COMPLETELY INDEPENDENT of the touch adapter.
 * It receives normalized TouchPoints and provides:
 * - Circular buffer of recent touch points
 * - Active touch count query
 * - Coordinate mapping integration point for InputManager
 *
 * The pipeline is:
 * [XPT2046/GT911 Adapter] → [TouchManager] → [UI System]
 */
class TouchManager {
public:
    static constexpr uint8_t CIRCULAR_BUFFER_SIZE = 8;  ///< Fixed-size circular buffer

    /**
     * @brief Construct TouchManager
     * @param maxX Display width for coordinate clamping
     * @param maxY Display height for coordinate clamping
     */
    TouchManager(int16_t maxX = 320, int16_t maxY = 240);

    /**
     * @brief Destructor
     */
    ~TouchManager() = default;

    /**
     * @brief Initialize touch system
     * @return true if initialization successful
     */
    bool init();

    /**
     * @brief Update touch state - call this every frame
     * @param dt Delta time in milliseconds
     */
    void update(unsigned long dt);

    /**
     * @brief Get all active touch points
     * @param points Output buffer
     * @return Number of active touch points
     */
    uint8_t getTouchPoints(TouchPoint* points) const;

    /**
     * @brief Get number of active touch points
     * @return Count of currently pressed touches
     */
    uint8_t getActiveCount() const;

    /**
     * @brief Check if any touch is active
     * @return true if any touch is pressed
     */
    bool isTouchActive() const;

    /**
     * @brief Get touch point at specific index
     * @param index Index (0 to getActiveCount() - 1)
     * @return TouchPoint reference
     */
    const TouchPoint& getTouchPoint(uint8_t index) const;

    /**
     * @brief Check if specific area is touched
     * @param x X coordinate
     * @param y Y coordinate
     * @param radius Touch hit radius
     * @return true if touch detected in area
     */
    bool isTouchedInArea(int16_t x, int16_t y, int16_t radius) const;

    /**
     * @brief Set calibration parameters
     * @param calib Calibration data
     */
    void setCalibration(const TouchCalibration& calib);

    /**
     * @brief Check if touch controller is connected
     * @return true if responding
     */
    bool isConnected() const;

    /**
     * @brief Get events from the event system
     * @param buffer Output buffer for events
     * @param maxCount Maximum number of events to retrieve
     * @return Number of events retrieved
     * 
     * Pull-based API: consumer provides the buffer.
     * Events are removed from the internal queue.
     */
    uint8_t getEvents(TouchEvent* buffer, uint8_t maxCount);
    
    /**
     * @brief Peek at events without removing them
     * @param buffer Output buffer for events
     * @param maxCount Maximum events to peek
     * @return Number of events peeked
     */
    uint8_t peekEvents(TouchEvent* buffer, uint8_t maxCount) const;
    
    /**
     * @brief Check if events are available
     * @return true if there are pending events
     */
    bool hasEvents() const;
    
    /**
     * @brief Get number of pending events
     * @return Number of events in queue
     */
    uint8_t getEventCount() const;
    
    /**
     * @brief Clear all pending events
     */
    void clearEvents();
    
    /**
     * @brief Get current state for a touch ID
     * @param touchId Touch identifier
     * @return Current state
     */
    TouchState getTouchState(uint8_t touchId) const;

private:
    // Circular buffer for touch points
    TouchPoint touchBuffer[CIRCULAR_BUFFER_SIZE];
    uint8_t bufferHead;
    uint8_t activeCount;

    // Display bounds for clamping
    int16_t displayWidth;
    int16_t displayHeight;

    // Calibration
    TouchCalibration calibration;

    // Connection status
    bool controllerConnected;
    
    // Touch event dispatcher for gesture detection
    TouchEventDispatcher eventDispatcher;

    /**
     * @brief Add touch point to circular buffer
     * @param point Touch point to add
     */
    void addTouchPoint(const TouchPoint& point);

    /**
     * @brief Clear all touch points
     */
    void clearBuffer();

    /**
     * @brief Calculate distance between two points
     * @return Distance in pixels
     */
    static int16_t distance(int16_t x1, int16_t y1, int16_t x2, int16_t y2);
};

// Inline implementation

inline TouchManager::TouchManager(int16_t maxX, int16_t maxY)
    : bufferHead(0)
    , activeCount(0)
    , displayWidth(maxX)
    , displayHeight(maxY)
    , controllerConnected(false) {
    clearBuffer();
}

inline bool TouchManager::init() {
    // Initialize the active touch adapter
#if defined(TOUCH_DRIVER_XPT2046)
    controllerConnected = XPT2046Adapter::initImpl();
    XPT2046Adapter::setCalibrationImpl(calibration);
#elif defined(TOUCH_DRIVER_GT911)
    controllerConnected = GT911Adapter::initImpl();
    GT911Adapter::setCalibrationImpl(calibration);
#else
    controllerConnected = false;
#endif
    return controllerConnected;
}

inline void TouchManager::update(unsigned long dt) {
    (void)dt;  // Unused for now

    TouchPoint points[TOUCH_MAX_POINTS];
    uint8_t count = 0;

    // Read from active adapter
#if defined(TOUCH_DRIVER_XPT2046)
    XPT2046Adapter::readImpl(points, count);
#elif defined(TOUCH_DRIVER_GT911)
    GT911Adapter::readImpl(points, count);
#else
    count = 0;
#endif

    // Clear buffer and add new points
    clearBuffer();

    for (uint8_t i = 0; i < count; i++) {
        // Clamp to display bounds
        TouchPoint tp = points[i];
        if (tp.x < 0) tp.x = 0;
        if (tp.x > displayWidth) tp.x = displayWidth;
        if (tp.y < 0) tp.y = 0;
        if (tp.y > displayHeight) tp.y = displayHeight;

        addTouchPoint(tp);
    }

    // Process touch points through event dispatcher for gesture detection
    // Use millis() for timestamp - need to get a timestamp for the current frame
    // For now, use 0 as a placeholder - actual implementation should use proper timing
    uint32_t currentTime = 0;
#if defined(PLATFORM_NATIVE)
    // On native, we could use a mock time, but for now pass 0
    currentTime = 0;
#else
    currentTime = millis();
#endif

    static int16_t sLastTouchX = 0;
    static int16_t sLastTouchY = 0;
    static bool sHadActiveTouch = false;

    if (count > 0) {
        sLastTouchX = points[0].x;
        sLastTouchY = points[0].y;
        sHadActiveTouch = true;
        pixelroot32::core::logging::log("[TouchMgr] pts=%u x=%d y=%d evtsBefore=%u",
            count, points[0].x, points[0].y, eventDispatcher.getEventCount());
        eventDispatcher.processTouchPoints(points, count, currentTime);
        pixelroot32::core::logging::log("[TouchMgr] evtsAfter=%u", eventDispatcher.getEventCount());
    } else if (sHadActiveTouch) {
        pixelroot32::core::logging::log("[TouchMgr] RELEASE lastX=%d lastY=%d", sLastTouchX, sLastTouchY);
        eventDispatcher.processTouch(0, false, sLastTouchX, sLastTouchY, currentTime);
        pixelroot32::core::logging::log("[TouchMgr] evtsAfterRelease=%u", eventDispatcher.getEventCount());
        sHadActiveTouch = false;
    }

    // Update connection status
    if (count == 0 && activeCount == 0) {
        // Check if controller is still connected
#if defined(TOUCH_DRIVER_XPT2046)
        controllerConnected = XPT2046Adapter::isConnectedImpl();
#elif defined(TOUCH_DRIVER_GT911)
        controllerConnected = GT911Adapter::isConnectedImpl();
#endif
    }
}

inline uint8_t TouchManager::getTouchPoints(TouchPoint* points) const {
    uint8_t count = 0;
    for (uint8_t i = 0; i < activeCount && i < TOUCH_MAX_POINTS; i++) {
        points[i] = touchBuffer[i];
        count++;
    }
    return count;
}

inline uint8_t TouchManager::getActiveCount() const {
    return activeCount;
}

inline bool TouchManager::isTouchActive() const {
    return activeCount > 0;
}

inline const TouchPoint& TouchManager::getTouchPoint(uint8_t index) const {
    static TouchPoint empty;
    if (index < activeCount) {
        return touchBuffer[index];
    }
    return empty;
}

inline bool TouchManager::isTouchedInArea(int16_t x, int16_t y, int16_t radius) const {
    for (uint8_t i = 0; i < activeCount; i++) {
        if (touchBuffer[i].pressed) {
            int16_t dist = distance(touchBuffer[i].x, touchBuffer[i].y, x, y);
            if (dist <= radius) {
                return true;
            }
        }
    }
    return false;
}

inline void TouchManager::setCalibration(const TouchCalibration& calib) {
    calibration = calib;
#if defined(TOUCH_DRIVER_XPT2046)
    XPT2046Adapter::setCalibrationImpl(calib);
#elif defined(TOUCH_DRIVER_GT911)
    GT911Adapter::setCalibrationImpl(calib);
#endif
}

inline bool TouchManager::isConnected() const {
    return controllerConnected;
}

inline uint8_t TouchManager::getEvents(TouchEvent* buffer, uint8_t maxCount) {
    return eventDispatcher.getEvents(buffer, maxCount);
}

inline uint8_t TouchManager::peekEvents(TouchEvent* buffer, uint8_t maxCount) const {
    return eventDispatcher.peekEvents(buffer, maxCount);
}

inline bool TouchManager::hasEvents() const {
    return eventDispatcher.hasEvents();
}

inline uint8_t TouchManager::getEventCount() const {
    return eventDispatcher.getEventCount();
}

inline void TouchManager::clearEvents() {
    eventDispatcher.clearEvents();
}

inline TouchState TouchManager::getTouchState(uint8_t touchId) const {
    return eventDispatcher.getTouchState(touchId);
}

inline void TouchManager::addTouchPoint(const TouchPoint& point) {
    if (activeCount < CIRCULAR_BUFFER_SIZE) {
        touchBuffer[activeCount++] = point;
    }
}

inline void TouchManager::clearBuffer() {
    for (uint8_t i = 0; i < CIRCULAR_BUFFER_SIZE; i++) {
        touchBuffer[i] = TouchPoint();
    }
    activeCount = 0;
    bufferHead = 0;
}

inline int16_t TouchManager::distance(int16_t x1, int16_t y1, int16_t x2, int16_t y2) {
    int16_t dx = x2 - x1;
    int16_t dy = y2 - y1;
    // Simple Manhattan distance (faster than sqrt)
    return (dx < 0 ? -dx : dx) + (dy < 0 ? -dy : dy);
}

} // namespace pixelroot32::input
