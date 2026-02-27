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
#include <optional>

namespace pixelroot32::core {

#define MAX_SCENES 5  // Adjust based on needs

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
     * @brief Gets the currently active scene.
     * @return Optional pointer to the top scene on the stack.
     */
    std::optional<Scene*> getCurrentScene() const;

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

private:
    Scene* sceneStack[MAX_SCENES] = {nullptr};  ///< Fixed-size stack for scenes.
    int sceneCount = 0; ///< Current number of scenes in the stack.
};

}
