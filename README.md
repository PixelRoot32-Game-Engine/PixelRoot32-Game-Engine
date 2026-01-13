# THIS IS A WORK IN PROGRESS!!! <!-- omit in toc -->

Things can break at any update.

---

# Table of content <!-- omit in toc -->

- [The ESP Device Game Engine (EDGE)](#the-esp-device-game-engine-edge)
  - [Features](#features)
  - [How It Works](#how-it-works)
    - [Scenes](#scenes)
    - [Entities](#entities)
    - [Renderer \& DisplayConfig](#renderer--displayconfig)
  - [Collision System](#collision-system)
  - [Project Structure](#project-structure)
  - [Setup \& Installation](#setup--installation)
    - [Hardware Requirements](#hardware-requirements)
    - [Library Dependencies](#library-dependencies)
    - [Include EDGE in Your Project](#include-edge-in-your-project)
  - [Usage](#usage)
    - [Initializing the Engine](#initializing-the-engine)
    - [Customizing DisplayConfig](#customizing-displayconfig)
    - [Creating a Scene](#creating-a-scene)
    - [Customizing the Renderer](#customizing-the-renderer)
    - [Example Projects](#example-projects)
  - [Customization Guide](#customization-guide)
  - [Contributing](#contributing)
  - [License](#license)
  - [Credits](#credits)
- [Troubleshooting](#troubleshooting)
  - [No Serial Output](#no-serial-output)


---
<img width="4096" height="2048" alt="New Piskel (17)" src="https://github.com/user-attachments/assets/dc352f8a-3511-43f0-bb6e-8bc75d2875a6" />

## The ESP Device Game Engine

**EDGE** is a lightweight, **entity-based game engine** designed for **embedded devices** with **small displays**. It features **scene management**, **entity handling**, and is **hardware-agnostic**, working with **any U8g2-compatible display**.

https://github.com/user-attachments/assets/466554af-29f4-4d50-976b-3db33ba87524

---

## Features
- **Entity-based architecture** for modular game objects.
- **Scene management** to handle game flow.
- **Customizable renderer & input system** (supporting different devices).
- **Optimized for embedded systems** (memory-efficient, no heap fragmentation).
- **Supports any device with a U8g2-compatible display**.
- **Auto-adjusting display offsets** based on screen size.

---

## How It Works
EDGE is built around **scenes** and **entities**:

### Scenes
- A **scene** represents a game state (e.g., main menu, gameplay, pause screen).
- Each scene contains multiple **entities** (e.g., a player, obstacles, UI).
- The **SceneManager** handles scene transitions.

### Entities
- Each **entity** is an object in the game (player, enemy, particle, etc.).
- Entities **update every frame** and **draw themselves**.

### Renderer & DisplayConfig
- The **Renderer** handles all drawing operations with `U8g2`.
- Display offsets (`xOffset`, `yOffset`) are automatically calculated based on screen dimensions defined in `DisplayConfig`.

---

## Collision System

EDGE now includes a **Collision System** to handle interactions between entities in a centralized way. This keeps collision logic clean and modular.

### How it Works

- Each **Entity** can have a **HitBox** (rectangle or circle) defining its collision area.
- The **CollisionSystem** is responsible for:
  - Registering entities that participate in collisions.
  - Checking intersections between all registered entities.
  - Calling the `onCollision(Entity* other)` method on each colliding entity.

## Project Structure
```
EDGE/
├── src/
│   ├── EDGE.h / EDGE.cpp              # Core engine logic
│   ├── Scene.h / Scene.cpp            # Scene management
│   ├── SceneManager.h / SceneManager.cpp  # Handles multiple scenes
│   ├── Renderer.h / Renderer.cpp      # U8g2-based graphics rendering
│   ├── InputManager.h / InputManager.cpp  # Button/input handling
│   ├── DisplayConfig.h                # Configuration for display settings
├── examples/
│   ├── BouncingBallScene/             # Example: Simple physics
│   ├── FlappyBirdScene/               # Example: Game using physics + input
├── README.md                          # Documentation
├── platformio.ini                     # PlatformIO project configuration
```

---

## Setup & Installation

### Hardware Requirements
- **ESP32/ESP8266/Arduino** (or any microcontroller with `U8g2` support).
- **Display:** SSD1306 OLED, SH1106, or other `U8g2`-compatible screens.
- **Buttons/Input Device** (can be GPIO buttons or a custom controller).
  - Note: the switch should pull the pin to ground on a button press. Pin 9 by default.

### Library Dependencies
Ensure you have the following libraries installed:
```sh
pio lib install "U8g2" "ArduinoQueue" "SafeString" "OneButton"
```

### Include EDGE in Your Project
In **PlatformIO** or Arduino IDE:
```cpp
#include "EDGE.h"
```

### Install as a Library
#### Arduino IDE
Git clone EDGE into the Arduino library directory
```sh
cd ~/Arduino/libraries
git clone http://https://github.com/nbourre/ESP32-Game-Engine
```

#### PlatformIO
Create a new project and git clone EDGE in the probject library 
directory.
```sh
cd ~/Documents/PlatformIO/Projects/<project name>/lib
git clone http://https://github.com/nbourre/ESP32-Game-Engine
```

### Run an Example
After setting up EDGE as a library, copy the example code.

#### Arduino IDE
Copy the example code to a new sketch:
```sh
cd ~/Arduino
cp -r libraries/ESP32-Game-Engine/examples/BouncingBall .
```
Open the BouncingBall sketch.

#### PlatformIO
- Copy the example code into the src directory of the project:
```sh
cd ~/Documents/PlatformIO/Projects/<project name>
cp lib/ESP32-Game-Engine/examples/BouncingBall/* src
```
- Remove the main.cpp file
- Configure the project:
  - Set the platform and board
  - Add the 4 libraries

---

## Usage

### Initializing the Engine
Create an instance of `EDGE` with `DisplayConfig` and set the first scene:
```cpp
#include "EDGE.h"
#include "MyGameScene.h"

#define I2C_SDA 5
#define I2C_SCL 6
#define SCREEN_WIDTH 72
#define SCREEN_HEIGHT 40

DisplayConfig config(SSD1306, I2C_SCL, I2C_SDA, U8X8_PIN_NONE, U8G2_R0, SCREEN_WIDTH, SCREEN_HEIGHT, true);
EDGE engine(config);
MyGameScene gameScene;

void setup() {
    engine.init();
    engine.setScene(&gameScene);
}

void loop() {
    engine.update();
    engine.draw();
}
```

### Customizing DisplayConfig
You can customize the display configuration if your display has different default dimensions:
```cpp
#define I2C_SDA 4
#define I2C_SCL 15
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

DisplayConfig config(SSD1306, I2C_SCL, I2C_SDA, U8X8_PIN_NONE, U8G2_R0, SCREEN_WIDTH, SCREEN_HEIGHT, true);
EDGE engine(config);
```

If you have a display with a resolution different from 128x64, you can adjust the `DEFAULT_DISPLAY_WIDTH` and `DEFAULT_DISPLAY_HEIGHT` in `DisplayConfig.h`.

This flexibility allows EDGE to work seamlessly with various display modules.

### Creating a Scene
A **scene** manages all game objects:
```cpp
#pragma once
#include "Scene.h"

class MyGameScene : public Scene {
public:
    void init() override;
    void update(unsigned long deltaTime) override;
    void draw(Renderer& renderer) override;
};
```

**Scene Implementation:**
```cpp
#include "MyGameScene.h"
#include "EDGE.h"

extern EDGE engine;

void MyGameScene::init() {
    // Initialize scene
}

void MyGameScene::update(unsigned long deltaTime) {
    // Game logic
}

void MyGameScene::draw(Renderer& renderer) {
    renderer.drawText(10, 10, "Hello EDGE!");
}
```

### Customizing the Renderer
The `Renderer` is generalized to support different displays. Offsets are automatically calculated based on the display size from `DisplayConfig`.

```cpp
Renderer::Renderer(const DisplayConfig& config) : config(config) {
    u8g2 = new U8G2_SSD1306_128X64_NONAME_F_HW_I2C(config.rotation, U8X8_PIN_NONE, config.clockPin, config.dataPin);
    xOffset = config.xOffset;
    yOffset = config.yOffset;
}
```

### Example Projects
- **Bouncing Ball** (Basic physics test)
- **Flappy Bird** (Button-controlled physics)
- **DVD SCREENSAVER** (Image support test)
- **Moonlander** (More complex game)
---

## Customization Guide
- **Different Screens?**    Modify `DisplayConfig` for your screen dimensions.
- **Different Controls?**    Customize `InputManager.cpp`.
- **New Scenes?**    Extend `Scene` and implement `update()` and `draw()`.
- **Custom Images?**    Convert your image to a `.XBM` file and place it in `BitmapInit.h` as a `static unsigned char` then call `.drawXBM` in your scene
---

## Contributing
- **Fork the repo** and submit **pull requests**.
- **Report bugs or suggest features** via GitHub issues.
- **Share your own scenes/games** using EDGE.

---

## License
EDGE is **open-source** under the **MIT License**.

---

## Credits
Developed by **Nicolas Bourré** as a modular **game engine for embedded systems**.

---

## Contributers
Jmwanderer for work on input support and general usability
SLDTyp0 for custom image support and work on input support

---

# Troubleshooting

## No Serial Output
On some ESP32 boards, enable **USB CDC On Boot** in the Arduino IDE settings to get Serial output working.

