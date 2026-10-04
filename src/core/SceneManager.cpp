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
        #if PIXELROOT32_ENABLE_GAMEPLAY_EVENTS
        // Drain the gameplay event bus at the SceneSwap funnel — this covers
        // both transitionToScene()'s SceneSwap arm and any direct call.
        // Deliberately NOT done in pushScene()/popScene() (design.md D7).
        if (eventBus_) eventBus_->clear();
        #endif

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
                // init() resets the timer and the iris centers (and leaves
                // wipe direction / sub-step untouched), so the stored full
                // description is re-applied here.
                if (transitionEffect_ != nullptr) {
                    transitionEffect_->init(transitionType_,
                                            pixelroot32::graphics::TransitionDirection::In,
                                            transitionDuration_);
                    // Re-apply stored description (no-op entries stay default).
                    transitionEffect_->setIrisOutCenter(irisOutX_, irisOutY_);
                    transitionEffect_->setIrisInCenter(irisInX_, irisInY_);
                    transitionEffect_->setWipeDirection(wipeDirection_);
                    transitionEffect_->setSubStepMs(subStepMs_);
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
        gfx::TransitionConfig config;
        config.type = type;
        config.durationMs = durationMs;
        transitionToScene(newScene, config);
    }

    void SceneManager::transitionToScene(Scene* newScene,
                                           pixelroot32::graphics::TransitionType type,
                                           unsigned long durationMs,
                                           int irisOutCx, int irisOutCy,
                                           int irisInCx, int irisInCy) {
        gfx::TransitionConfig config;
        config.type = type;
        config.durationMs = durationMs;
        config.irisOutCx = irisOutCx;
        config.irisOutCy = irisOutCy;
        config.irisInCx = irisInCx;
        config.irisInCy = irisInCy;
        transitionToScene(newScene, config);
    }

    void SceneManager::transitionToScene(Scene* newScene,
                                           const gfx::TransitionConfig& config) {
        // Ignored if a transition is already running.
        if (transitionState_ != TransitionState::Idle) return;

        // Store the whole description: a later transition never inherits
        // direction, sub-step or centers from this one (issue #240).
        transitionTargetScene_ = newScene;
        transitionType_ = config.type;
        transitionDuration_ = config.durationMs;
        irisOutX_ = config.irisOutCx;
        irisOutY_ = config.irisOutCy;
        irisInX_ = config.irisInCx;
        irisInY_ = config.irisInCy;
        wipeDirection_ = config.wipeDirection;
        subStepMs_ = config.subStepMs;
        transitionState_ = TransitionState::FadingOut;

        // Initialise the effect for the fade-out phase, then apply the
        // stored description (init resets centers but not direction/sub-step;
        // setting everything explicitly keeps both phases consistent).
        if (transitionEffect_ != nullptr) {
            transitionEffect_->init(config.type,
                                    pixelroot32::graphics::TransitionDirection::Out,
                                    config.durationMs);
            transitionEffect_->setIrisOutCenter(irisOutX_, irisOutY_);
            transitionEffect_->setIrisInCenter(irisInX_, irisInY_);
            transitionEffect_->setWipeDirection(wipeDirection_);
            transitionEffect_->setSubStepMs(subStepMs_);
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
