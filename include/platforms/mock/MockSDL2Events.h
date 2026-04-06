/*
 * Copyright (c) 2026 PixelRoot32
 * Licensed under the MIT License
 * 
 * Mock SDL2 event handling for controlled event testing
 */

#ifndef PIXELROOT32_MOCK_SDL2_EVENTS_H
#define PIXELROOT32_MOCK_SDL2_EVENTS_H

#ifdef PLATFORM_NATIVE

#include <cstdint>
#include <vector>
#include <queue>
#include <SDL2/SDL.h> 

namespace pixelroot32::platforms::mock {

/**
 * @brief Mock SDL2 event provider for testing
 * 
 * Allows tests to inject specific SDL events and control
 * event processing for deterministic testing of Engine::run()
 */
class MockSDL2EventProvider {
public:
    MockSDL2EventProvider() : shouldQuit_(false), frameCount_(0), maxFrames_(0) {}

    /**
     * @brief Queue an SDL event to be returned by pollEvent
     * @param eventType SDL event type (e.g., SDL_QUIT, SDL_KEYDOWN)
     */
    void queueEvent(uint32_t eventType) {
        eventQueue_.push(eventType);
    }

    /**
     * @brief Set quit flag to exit the game loop
     * @param quit true to signal quit
     */
    void setQuit(bool quit) {
        shouldQuit_ = quit;
    }

    /**
     * @brief Set maximum number of frames before auto-quit
     * @param frames Number of frames to run before returning false
     */
    void setMaxFrames(int frames) {
        maxFrames_ = frames;
        frameCount_ = 0;
    }

    /**
     * @brief Reset event provider state
     */
    void reset() {
        shouldQuit_ = false;
        frameCount_ = 0;
        maxFrames_ = 0;
        while (!eventQueue_.empty()) {
            eventQueue_.pop();
        }
    }

    /**
     * @brief Mock implementation of processEvents
     * @return false if quit requested, true otherwise
     */
    bool processEvents() {
        // Check if we've hit frame limit
        if (maxFrames_ > 0) {
            frameCount_++;
            if (frameCount_ >= maxFrames_) {
                return false;
            }
        }

        // Check quit flag
        if (shouldQuit_) {
            return false;
        }

        // Process queued events
        if (!eventQueue_.empty()) {
            uint32_t event = eventQueue_.front();
            eventQueue_.pop();
            if (event == SDL_QUIT) {
                return false;
            }
        }

        return true;
    }

    /**
     * @brief Get current frame count
     */
    int getFrameCount() const {
        return frameCount_;
    }

private:
    bool shouldQuit_;
    int frameCount_;
    int maxFrames_;
    std::queue<uint32_t> eventQueue_;
};

/**
 * @brief Global mock SDL2 event provider
 */
extern MockSDL2EventProvider* g_mockSDL2Events;

} // namespace pixelroot32::platforms::mock

#endif // PLATFORM_NATIVE

#endif // PIXELROOT32_MOCK_SDL2_EVENTS_H
