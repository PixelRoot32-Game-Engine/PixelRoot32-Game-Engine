#pragma once
#include "Scene.h"
#include "graphics/Renderer.h"

#define MAX_SCENES 5  // Adjust based on needs

class SceneManager {
public:
    SceneManager();

    void setCurrentScene(Scene* newScene);  // Replaces current scene
    void pushScene(Scene* newScene);        // Adds a new scene on top
    void popScene();                        // Removes top scene

    void update(unsigned long dt);
    void draw(Renderer& renderer);
    Scene* getCurrentScene() const;

private:
    Scene* sceneStack[MAX_SCENES] = {nullptr};  // Fixed-size stack
    int sceneCount = 0;
};
