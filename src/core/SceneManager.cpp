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
#include "core/SceneManager.h"
#include "platforms/EngineConfig.h"

namespace pixelroot32::core {

    namespace gfx = pixelroot32::graphics;

        using gfx::Renderer;

    SceneManager::SceneManager() {
        for (int i = 0; i < pixelroot32::platforms::config::MaxScenes; i++) {
            sceneStack[i] = nullptr;  // Initialize empty stack
        }
    }

    void SceneManager::setCurrentScene(Scene* newScene) {
        sceneCount = 0;  // Clear previous scenes
        sceneStack[sceneCount++] = newScene;
        newScene->init();
    }

    void SceneManager::pushScene(Scene* newScene) {
        if (sceneCount < pixelroot32::platforms::config::MaxScenes) {
            sceneStack[sceneCount++] = newScene;
            newScene->init();
        }
    }

    void SceneManager::popScene() {
        if (sceneCount > 0) {
            sceneCount--;  // Remove top scene
        }
    }

    void SceneManager::update(unsigned long dt) {
        switch (transitionState_) {
            case TransitionState::Idle:
                // Normal operation — forward update to current scene.
                if (sceneCount > 0) {
                    sceneStack[sceneCount - 1]->update(dt);
                }
                break;

            case TransitionState::FadingOut:
                // Advance the transition effect (fade-out phase).
                if (transitionEffect_ != nullptr) {
                    transitionEffect_->update(dt);
                    // When the effect completes, move to the one-tick SceneSwap state.
                    if (!transitionEffect_->isActive()) {
                        transitionState_ = TransitionState::SceneSwap;
                    }
                } else {
                    // No effect available — skip immediately to SceneSwap.
                    transitionState_ = TransitionState::SceneSwap;
                }
                // NOTE: scene.update(dt) is deliberately NOT called during
                // FadingOut — this provides input blocking.
                break;

            case TransitionState::SceneSwap:
                // Atomically swap the scene.
                if (transitionTargetScene_ != nullptr) {
                    setCurrentScene(transitionTargetScene_);
                    transitionTargetScene_ = nullptr;
                }
                // Re-initialise the effect for the fade-in phase.
                if (transitionEffect_ != nullptr) {
                    transitionEffect_->init(transitionType_,
                                            pixelroot32::graphics::TransitionDirection::In,
                                            transitionDuration_);
                }
                transitionState_ = TransitionState::FadingIn;
                // NOTE: scene.update(dt) is NOT called during SceneSwap.
                break;

            case TransitionState::FadingIn:
                // Advance the transition effect (fade-in phase).
                if (transitionEffect_ != nullptr) {
                    transitionEffect_->update(dt);
                    // When the effect completes, return to Idle.
                    if (!transitionEffect_->isActive()) {
                        transitionState_ = TransitionState::Idle;
                    }
                } else {
                    // No effect available — return to Idle immediately.
                    transitionState_ = TransitionState::Idle;
                }
                // NOTE: scene.update(dt) is deliberately NOT called during
                // FadingIn — this provides input blocking.
                break;
        }
    }

    void SceneManager::transitionToScene(Scene* newScene,
                                          pixelroot32::graphics::TransitionType type,
                                          unsigned long durationMs) {
        // Ignored if a transition is already running.
        if (transitionState_ != TransitionState::Idle) return;

        transitionTargetScene_ = newScene;
        transitionType_ = type;
        transitionDuration_ = durationMs;
        transitionState_ = TransitionState::FadingOut;

        // Initialise the effect for the fade-out phase.
        if (transitionEffect_ != nullptr) {
            transitionEffect_->init(type,
                                    pixelroot32::graphics::TransitionDirection::Out,
                                    durationMs);
        }
    }

    void SceneManager::draw(Renderer& renderer) {
        for (int i = 0; i < sceneCount; i++) {
            sceneStack[i]->draw(renderer);  // Draw all stacked scenes
        }
    }

    void SceneManager::adviseFramebufferBeforeBeginFrame(Renderer& renderer) {
        renderer.resetFramebufferClearSuppressionAdvice();
        for (int i = 0; i < sceneCount; ++i) {
            sceneStack[i]->adviseFramebufferBeforeBeginFrame(renderer);
        }
    }

    std::optional<Scene*> SceneManager::getCurrentScene() const {
        if (sceneCount > 0) {
            return sceneStack[sceneCount - 1];
        }
        return std::nullopt;
    }

    bool SceneManager::aggregateShouldRedrawFramebuffer() const {
        if (sceneCount <= 0) {
            return true;
        }
        for (int i = 0; i < sceneCount; ++i) {
            if (sceneStack[i]->shouldRedrawFramebuffer()) {
                return true;
            }
        }
        return false;
    }
}
