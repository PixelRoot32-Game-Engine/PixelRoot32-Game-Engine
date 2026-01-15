
## Setup & Installation

### Hardware Requirements
- **ESP32/ESP8266/Arduino** (or any microcontroller with `U8g2` support).
- **Display:** SSD1306 OLED, SH1106, or other `U8g2`-compatible screens.
- **Buttons/Input Device** (can be GPIO buttons or a custom controller).
  - Note: the switch should pull the pin to ground on a button press. Pin 9 by default.

### Library Dependencies
Ensure you have the following libraries installed:
```sh
pio lib install "ArduinoQueue" "SafeString" "OneButton"
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
git clone http://https://github.com/gperez88/PixelRoot32-Game-Engine
```

#### PlatformIO
Create a new project and git clone PixelRoot32-Game-Engine in the probject library 
directory.
```sh
cd ~/Documents/PlatformIO/Projects/<project name>/lib
git clone http://https://github.com/gperez88/PixelRoot32-Game-Engine
```

### Run an Example
After setting up PixelRoot32-Game-Engine as a library, copy the example code.

#### PlatformIO
- Copy the example code into the src directory of the project:
```sh
cd ~/Documents/PlatformIO/Projects/<project name>
cp lib/PixelRoot32-Game-Engine/examples/Pong/* src
```
- Remove the main.cpp file
- Configure the project:
  - Set the platform and board
  - Add the 4 libraries

---