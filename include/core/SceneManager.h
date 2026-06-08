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

#include "Scene.h"
#include "graphics/Renderer.h"
#include "graphics/TransitionEffect.h"
#include "platforms/EngineConfig.h"

#include <optional>

namespace pixelroot32::core {

/**
 * @enum TransitionState
 * @brief State machine for scene transitions.
 *
 * Idle → FadingOut → SceneSwap → FadingIn → Idle.
 * During FadingOut/FadingIn the current scene's update() is skipped
 * (input blocking). Draw() still runs so the framebuffer has content
 * for the transition effect to post-process.
 */
enum class TransitionState : uint8_t {
    Idle = 0,       ///< No active transition; normal operation.
    FadingOut,      ///< Fade/Iris Out — current scene being hidden.
    SceneSwap,      ///< One-tick state that atomically swaps scenes.
    FadingIn        ///< Fade/Iris In — new scene being revealed.
};

/**
 * @class SceneManager
 * @brief Manages the stack of active scenes.
 *
 * The SceneManager allows for scene transitions (replacing scenes) and
 * stacking scenes (push/pop), which is useful for pausing or menus.
 */
class SceneManager {
public:
    SceneManager();

    /**
     * @brief Replaces the current scene with a new one.
     * @param newScene The new scene to switch to.
     */
    void setCurrentScene(Scene* newScene);

    /**
     * @brief Pushes a new scene onto the stack, pausing the previous one.
     * @param newScene The new scene to become active.
     */
    void pushScene(Scene* newScene);

    /**
     * @brief Removes the top scene from the stack, resuming the previous one.
     */
    void popScene();

    /**
     * @brief Updates the currently active scene.
     * @param dt Delta time in ms.
     */
    void update(unsigned long dt);

    /**
     * @brief Draws the currently active scene.
     * @param renderer The renderer to use.
     */
    void draw(pixelroot32::graphics::Renderer& renderer);

    /**
     * @brief Lets stacked scenes advertise framebuffer prep (runs before Renderer::beginFrame).
     */
    void adviseFramebufferBeforeBeginFrame(pixelroot32::graphics::Renderer& renderer);

    /**
     * @brief Gets the currently active scene.
     * @return Optional pointer to the top scene on the stack.
     */
    std::optional<Scene*> getCurrentScene() const;

    /**
     * @brief True if any scene on the stack needs a framebuffer pass this iteration.
     *
     * Used by Engine to skip draw+present only when every stacked scene returns false from
     * Scene::shouldRedrawFramebuffer().
     */
    bool aggregateShouldRedrawFramebuffer() const;

    /**
     * @brief Gets the number of scenes in the stack.
     * @return The number of scenes.
     */
    int getSceneCount() const { return sceneCount; }

    /**
     * @brief Checks if the scene stack is empty.
     * @return True if there are no scenes.
     */
    bool isEmpty() const { return sceneCount == 0; }

    // =========================================================================
    // Scene Transition API
    // =========================================================================

    /**
     * @brief Start a transition from the current scene to a new one.
     * @param newScene The target scene to transition to.
     * @param type Fade, Iris, or DiagonalWipe transition effect.
     * @param durationMs Duration of each phase (Out and In) in ms.
     *
     * Ignored if a transition is already running (state != Idle).
     * The full cycle is: FadingOut (durationMs) → SceneSwap → FadingIn (durationMs) → Idle.
     */
    void transitionToScene(Scene* newScene,
                           pixelroot32::graphics::TransitionType type,
                           unsigned long durationMs);

    /**
     * @brief Start a transition with direction-specific iris centers.
     * @param newScene The target scene to transition to.
     * @param type Fade, Iris, or DiagonalWipe transition effect.
     * @param durationMs Duration of each phase (Out and In) in ms.
     * @param irisOutCx Iris center X for Out (closing) phase.
     * @param irisOutCy Iris center Y for Out (closing) phase.
     * @param irisInCx Iris center X for In (opening) phase.
     * @param irisInCy Iris center Y for In (opening) phase.
     *
     * Stores the centers and re-applies them after each effect.init()
     * (which resets centers to -1). Only meaningful for Iris transitions.
     */
    void transitionToScene(Scene* newScene,
                           pixelroot32::graphics::TransitionType type,
                           unsigned long durationMs,
                           int irisOutCx, int irisOutCy,
                           int irisInCx, int irisInCy);

    /**
     * @brief Whether a scene transition is currently active.
     * @return true when TransitionState != Idle.
     */
    bool isTransitioning() const { return transitionState_ != TransitionState::Idle; }

    /**
     * @brief Get the current transition state.
     * @return The active TransitionState.
     */
    TransitionState getTransitionState() const { return transitionState_; }

    /**
     * @brief Provide a pointer to the Engine-owned TransitionEffect instance.
     * @param effect Non-owning pointer to the TransitionEffect.
     *
     * Called by Engine::init(). The Engine owns the TransitionEffect;
     * SceneManager only drives it (init, update) during transitions.
     */
    void setTransitionEffect(pixelroot32::graphics::TransitionEffect* effect) {
        transitionEffect_ = effect;
    }

private:
    Scene* sceneStack[pixelroot32::platforms::config::MaxScenes] = {nullptr};  ///< Fixed-size stack for scenes.
    int sceneCount = 0; ///< Current number of scenes in the stack.

    // -------------------------------------------------------------------------
    // Transition state
    // -------------------------------------------------------------------------
    TransitionState transitionState_ = TransitionState::Idle;   ///< Current transition phase.
    Scene* transitionTargetScene_ = nullptr;                     ///< Scene to swap to (set by transitionToScene).
    pixelroot32::graphics::TransitionEffect* transitionEffect_ = nullptr; ///< Engine-owned effect (non-owning ptr).
    pixelroot32::graphics::TransitionType transitionType_ = pixelroot32::graphics::TransitionType::Fade; ///< Cached effect type.
    unsigned long transitionDuration_ = 0;                       ///< Cached effect duration per phase.

    // Direction-specific iris centers (stored for re-apply after effect.init resets to -1)
    int irisOutX_ = -1;   ///< Stored Out iris center X (-1 = not set).
    int irisOutY_ = -1;   ///< Stored Out iris center Y (-1 = not set).
    int irisInX_ = -1;    ///< Stored In iris center X (-1 = not set).
    int irisInY_ = -1;    ///< Stored In iris center Y (-1 = not set).
};

}
