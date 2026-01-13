#pragma once
#include "core/SceneManager.h"
#include "graphics/Renderer.h"
#include "input/InputConfig.h"
#include "input/InputManager.h"
#include "graphics/DisplayConfig.h"


class EDGE {
public:
    EDGE(const DisplayConfig& displayConfig, const InputConfig& inputConfig);
    EDGE(const DisplayConfig& displayConfig);
    ~EDGE();

    void init();
    void run();

    unsigned long getDeltaTime() const { return deltaTime; }
    
    // Scene management via SceneManager
    void setScene(Scene* newScene);
    Scene* getCurrentScene() const { return sceneManager.getCurrentScene(); }
    
    void setRenderer(Renderer& newRenderer) { renderer = newRenderer; }
    Renderer& getRenderer();
    InputManager& getInputManager() { return inputManager; }

private:
    SceneManager sceneManager;
    Renderer renderer;
    InputManager inputManager;

    unsigned long previousMillis;
    unsigned long deltaTime;

    void update();
    void draw();
};
