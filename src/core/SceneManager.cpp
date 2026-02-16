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

namespace pixelroot32::core {

    using namespace pixelroot32::graphics;

    SceneManager::SceneManager() {
        for (int i = 0; i < MAX_SCENES; i++) {
            sceneStack[i] = nullptr;  // Initialize empty stack
        }
    }

    void SceneManager::setCurrentScene(Scene* newScene) {
        sceneCount = 0;  // Clear previous scenes
        sceneStack[sceneCount++] = newScene;
        newScene->init();
    }

    void SceneManager::pushScene(Scene* newScene) {
        if (sceneCount < MAX_SCENES) {
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
        if (sceneCount > 0) {
            sceneStack[sceneCount - 1]->update(dt);  // Only update top scene
        }
    }

    void SceneManager::draw(Renderer& renderer) {
        for (int i = 0; i < sceneCount; i++) {
            sceneStack[i]->draw(renderer);  // Draw all stacked scenes
        }
    }

    std::optional<Scene*> SceneManager::getCurrentScene() const {
        if (sceneCount > 0) {
            return sceneStack[sceneCount - 1];
        }
        return std::nullopt;
    }
}
