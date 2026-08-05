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
                // After init(), the effect's centers are reset to -1, so we
                // must re-apply any stored directional iris centers.
                if (transitionEffect_ != nullptr) {
                    transitionEffect_->init(transitionType_,
                                            pixelroot32::graphics::TransitionDirection::In,
                                            transitionDuration_);
                    // Re-apply stored iris centers (no-op if all -1).
                    transitionEffect_->setIrisOutCenter(irisOutX_, irisOutY_);
                    transitionEffect_->setIrisInCenter(irisInX_, irisInY_);
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

        // Reset stored iris centers (no centers specified → use defaults).
        irisOutX_ = irisOutY_ = irisInX_ = irisInY_ = -1;

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

    void SceneManager::transitionToScene(Scene* newScene,
                                          pixelroot32::graphics::TransitionType type,
                                          unsigned long durationMs,
                                          int irisOutCx, int irisOutCy,
                                          int irisInCx, int irisInCy) {
        // Call the original overload to start the transition (init effect for Out).
        // This will reset stored iris centers to -1, so we must store AFTER.
        transitionToScene(newScene, type, durationMs);

        // Store the directional iris centers AFTER the 3-param overload reset.
        irisOutX_ = irisOutCx;
        irisOutY_ = irisOutCy;
        irisInX_ = irisInCx;
        irisInY_ = irisInCy;

        // Apply centers to the effect for the Out phase.
        if (transitionEffect_ != nullptr) {
            transitionEffect_->setIrisOutCenter(irisOutCx, irisOutCy);
            transitionEffect_->setIrisInCenter(irisInCx, irisInCy);
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
