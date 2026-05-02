# Entities & scene tutorial

> **Not an `examples/` project** — This page is a **didactic walkthrough** (bouncing entities, `Entity` subclass, scene wiring). There is **no** matching folder under [`examples/`](../../examples/README.md) with this code. For a **real, minimal** PlatformIO project, start with [`hello_world`](../../examples/hello_world/) and its README.

The snippets below illustrate fundamentals: scene lifecycle, custom **`Entity`** subclasses, input, and drawing — useful once you have already opened **`hello_world`** in the repo.

## Complete Source

```cpp
#include <Arduino.h>
#include <Engine.h>
#include <Scene.h>
#include <Renderer.h>
#include <Entity.h>
#include <InputManager.h>

using namespace pixelroot32;

// ============================================================================
// Custom Entity: A bouncing ball
// ============================================================================
class Ball : public core::Entity {
    math::Scalar velocityX;
    math::Scalar velocityY;
    graphics::Color color;
    int screenWidth;
    int screenHeight;
    
public:
    Ball(int x, int y, int screenW, int screenH) 
        : Entity(math::toScalar(x), math::toScalar(y), 8, 8, core::EntityType::GENERIC),
          screenWidth(screenW),
          screenHeight(screenH),
          color(graphics::Color::RED) {
        
        // Random velocity using math::Scalar
        velocityX = math::toScalar(random(50, 150));
        velocityY = math::toScalar(random(50, 150));
        
        if (random(2) == 0) velocityX = -velocityX;
        if (random(2) == 0) velocityY = -velocityY;
    }
    
    void update(unsigned long deltaTime) override {
        using namespace math;
        
        // Convert deltaTime to seconds as Scalar
        Scalar dt = toScalar(deltaTime) / toScalar(1000);
        
        // Update position
        position.x += velocityX * dt;
        position.y += velocityY * dt;
        
        // Bounce off walls
        if (position.x <= toScalar(0) || 
            position.x >= toScalar(screenWidth - width)) {
            velocityX = -velocityX;
            color = static_cast<graphics::Color>(random(1, 16));
        }
        
        if (position.y <= toScalar(0) || 
            position.y >= toScalar(screenHeight - height)) {
            velocityY = -velocityY;
            color = static_cast<graphics::Color>(random(1, 16));
        }
        
        // Clamp to screen
        if (position.x < toScalar(0)) position.x = toScalar(0);
        if (position.y < toScalar(0)) position.y = toScalar(0);
        if (position.x > toScalar(screenWidth - width)) 
            position.x = toScalar(screenWidth - width);
        if (position.y > toScalar(screenHeight - height)) 
            position.y = toScalar(screenHeight - height);
    }
    
    void draw(graphics::Renderer& r) override {
        r.drawFilledRectangle(
            static_cast<int>(position.x),
            static_cast<int>(position.y),
            width,
            height,
            color
        );
    }
};

// ============================================================================
// Game Scene
// ============================================================================
class BouncingBallsScene : public core::Scene {
    static constexpr int NUM_BALLS = 10;
    std::unique_ptr<Ball> balls[NUM_BALLS];
    unsigned long spawnTimer = 0;
    int ballsActive = 0;
    int screenWidth;
    int screenHeight;
    
public:
    void init() override {
        // Nothing to do - balls spawned in first update
        screenWidth = engine->getRenderer().getLogicalWidth();
        screenHeight = engine->getRenderer().getLogicalHeight();
    }
    
    void update(unsigned long deltaTime) override {
        auto& input = engine->getInputManager();
        
        // Spawn balls gradually
        spawnTimer += deltaTime;
        if (spawnTimer > 500 && ballsActive < NUM_BALLS) {  // Every 500ms
            spawnTimer = 0;
            
            balls[ballsActive] = std::make_unique<Ball>(
                screenWidth / 2,
                screenHeight / 2,
                screenWidth,
                screenHeight
            );
            addEntity(balls[ballsActive].get());
            ballsActive++;
        }
        
        // Reset with button A
        if (input.isButtonJustPressed(input::ButtonName::A)) {
            resetBalls();
        }
        
        // Update all entities (calls Ball::update)
        Scene::update(deltaTime);
    }
    
    void draw(graphics::Renderer& r) override {
        // Clear handled by beginFrame, draw background
        r.drawFilledRectangle(0, 0, screenWidth, screenHeight, graphics::Color::BLACK);
        
        // Draw all entities (balls)
        Scene::draw(r);
        
        // Draw UI
        r.drawText("Balls: " + std::to_string(ballsActive), 5, 5, graphics::Color::WHITE, 1);
        r.drawText("Press A to reset", 5, 15, graphics::Color::GRAY, 1);
    }
    
private:
    void resetBalls() {
        // Remove existing balls
        for (int i = 0; i < ballsActive; ++i) {
            removeEntity(balls[i].get());
            balls[i].reset();
        }
        ballsActive = 0;
        spawnTimer = 0;
    }
};

// ============================================================================
// Main Setup
// ============================================================================
void setup() {
    // Seed random
    randomSeed(analogRead(0));
    
    // Display configuration
    graphics::DisplayConfig displayConfig(240, 240);
    
    // Input configuration
    input::InputConfig inputConfig;
    inputConfig.addButton(input::ButtonName::A, 0);  // BOOT button
    
    // Create engine
    core::Engine engine(std::move(displayConfig), inputConfig);
    
    // Create and set scene
    BouncingBallsScene scene;
    engine.setScene(&scene);
    
    // Initialize and run
    engine.init();
    engine.run();
}

void loop() {
    // Empty - engine.run() contains the game loop
}
```

## Key Concepts Demonstrated

### 1. Entity Creation

```cpp
class Ball : public core::Entity {
public:
    Ball(int x, int y, int w, int h) 
        : Entity(x, y, w, h, EntityType::GENERIC) {
        // Initialize
    }
    
    void update(unsigned long deltaTime) override;
    void draw(graphics::Renderer& r) override;
};
```

- Extend `Entity` for game objects
- Implement `update()` for logic
- Implement `draw()` for rendering

### 2. Frame-Rate Independent Movement

```cpp
void update(unsigned long deltaTime) override {
    // Convert to seconds
    math::Scalar dt = math::toScalar(deltaTime) / math::toScalar(1000);
    
    // Move at constant speed regardless of FPS
    position.x += velocityX * dt;
}
```

Always use `deltaTime` for time-based calculations.

### 3. Entity Management

```cpp
// Create
balls[i] = std::make_unique<Ball>(...);
addEntity(balls[i].get());  // Scene manages the pointer

// Remove
removeEntity(balls[i].get());  // Remove from scene
balls[i].reset();               // Free memory
```

Use smart pointers for automatic memory management.

### 4. Input Handling

```cpp
auto& input = engine->getInputManager();

// Button pressed this frame (edge trigger)
if (input.isButtonJustPressed(ButtonName::A)) {
    // Trigger action once
}

// Button held (continuous)
if (input.isButtonPressed(ButtonName::LEFT)) {
    // Continuous action
}
```

### 5. Scene Structure

```cpp
class MyScene : public core::Scene {
public:
    void init() override;                          // Setup
    void update(unsigned long deltaTime) override; // Logic
    void draw(graphics::Renderer& r) override;      // Rendering
};
```

## Build Configuration

```ini
; platformio.ini
[env:esp32dev]
platform = espressif32
board = esp32dev
framework = arduino

; C++17 required
build_unflags = -std=gnu++11
build_flags =
    -std=gnu++17
    -fno-exceptions

; Library dependencies
lib_deps = 
    https://github.com/PixelRoot32-Game-Engine/PixelRoot32-Game-Engine.git
```

## Running the Example

1. **Create project**:
   ```bash
   mkdir bouncing_balls
   cd bouncing_balls
   ```

2. **Copy code** to `src/main.cpp`

3. **Create** `platformio.ini` with configuration above

4. **Build and upload**:
   - Open in VS Code with PlatformIO
   - Select environment (ESP32 or Native)
   - Build and upload

## Expected Behavior

- Balls spawn one at a time every 500ms
- Balls bounce off screen edges
- Ball color changes on each bounce
- Press button A to reset
- Counter shows active ball count

## Variations

### Add Physics

```cpp
// Change to physics-enabled actor
class Ball : public physics::KinematicActor {
public:
    Ball(...) : KinematicActor(x, y, w, h) {
        setCollisionLayer(DefaultLayers::kEnvironment);
    }
    
    void onCollision(Actor* other) override {
        // Bounce with physics response
        velocityX = -velocityX;
    }
};
```

### Add Audio

```cpp
#if PIXELROOT32_ENABLE_AUDIO
void onCollision(Actor* other) override {
    engine->getAudioEngine().playSFX(sound_bounce);
}
#endif
```

### Touch Control

```cpp
#if PIXELROOT32_ENABLE_TOUCH
void onUnconsumedTouchEvent(const input::TouchEvent& event) override {
    if (event.type == input::TouchEventType::CLICK) {
        spawnBallAt(event.x, event.y);
    }
}
#endif
```

## Next Steps

- **[Samples index](../../examples/README.md)** — Real folders under `examples/`
- **[Physics (`physics`)](../../examples/physics/README.md)** — Collision and actors
- **[Sprites (`sprites`)](../../examples/sprites/README.md)** — Sprite graphics
- **Audio** — See [`snake`](../../examples/snake/README.md), [`tic_tac_toe`](../../examples/tic_tac_toe/README.md), [`music_demo`](../../examples/music_demo/README.md)
