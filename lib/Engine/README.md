# PixelRoot32 Game Engine

PixelRoot32 Game Engine es un motor de juegos 2D, ligero y modular, desarrollado en C++, diseñado específicamente para microcontroladores ESP32.

El motor adopta una arquitectura basada en nodos y escenas, inspirada en el flujo de trabajo de Godot Engine, y ofrece una capa de abstracción de hardware que permite simulación nativa en PC mediante SDL2, facilitando el desarrollo y depuración multiplataforma.

---

## Origen e Inspiración

PixelRoot32 nace como una evolución directa del proyecto:

ESP32-Game-Engine de nbourre  
https://github.com/nbourre/ESP32-Game-Engine

Sobre esta base sólida, PixelRoot32 expande el concepto original incorporando ideas inspiradas en Godot, tales como:

- Organización jerárquica mediante escenas y nodos
- Separación clara entre lógica, render y entrada
- Componentes reutilizables y desacoplados
- Flujo de actualización estructurado (update / draw)

Créditos: Este proyecto reconoce y agradece profundamente el trabajo original de nbourre, sobre el cual se construye y evoluciona PixelRoot32.

---

## Estructura del Proyecto

La arquitectura del motor separa la lógica de alto nivel del hardware (HAL), permitiendo un desarrollo eficiente tanto en ESP32 como en entorno desktop.

```
Engine/
├── include/
│   ├── core/               # Núcleo: Node, Scene, SceneManager
│   ├── graphics/
│   │   ├── ui/
│   │   │   ├── UIElement.h
│   │   │   ├── UILabel.h
│   │   │   └── UIButton.h
│   │   ├── Renderer.h
│   │   └── ...
│   ├── input/
│   └── physics/
├── src/
│   ├── core/
│   ├── graphics/
│   │   ├── ui/
│   │   │   ├── UILabel.cpp
│   │   │   └── UIButton.cpp
│   │   └── Renderer.cpp
│   └── ...

---

## Componentes Principales

Core  
Controla el SceneManager, el árbol de nodos y el ciclo principal de ejecución (update / draw).

Renderer  
API de renderizado unificada. En ESP32 utiliza TFT_eSprite con Double Buffering, eliminando parpadeos en pantalla.

InputManager  
Abstrae botones físicos (GPIO) y teclas de PC en comandos lógicos (UP, DOWN, A, B).

CollisionSystem  
Provee detección de colisiones AABB y soporte para movimientos basados en rejilla (grid-based).

---

## Optimizaciones de Alto Rendimiento

### Sistema de Partículas (Pooled Memory)

- Uso de arrays estáticos para reutilizar partículas y evitar fragmentación de memoria.
- Trigonometría pre-calculada para minimizar costos en el ciclo de actualización.
- Auto-clipping de entidades fuera de pantalla.

### Renderizado Asíncrono vía DMA

- Transferencias no bloqueantes mediante pushImageDMA.
- Paralelismo real entre lógica de juego y transferencia SPI.
- Sin tearing mediante sincronización con dmaWait.

---

## Sistema de Interfaz de Usuario (UI)

El sistema de UI es jerárquico y se integra al flujo normal de escenas, inspirado en el enfoque de nodos de Godot.

### Jerarquía de Clases

UIElement  
Clase base con control de visibilidad y estado.

UILabel  
Renderizado eficiente de texto con alineación dinámica.

UIButton  
Elemento interactivo conectado al InputManager (en desarrollo).

---

## Ejemplo de Uso en una Escena

```cpp
#include "graphics/ui/UILabel.h"

class GameScene : public Scene {
    UI::UILabel* lblStart;

    void init() override {
        lblStart = new UI::UILabel("PRESS A TO START", 0, 150, COLOR_WHITE, 1);
        lblStart->centerX(SCREEN_WIDTH);
        addEntity(lblStart);
    }

    void update(unsigned long deltaTime) override {
        if (gameStarted) {
            lblStart->setVisible(false);
        }
        Scene::update(deltaTime);
    }
};
```

### Notas técnicas para la implementación:
1. Optimización del Dibujo: Para evitar el efecto de "texto encimado" (ghosting) visible en la imagen que compartiste, el UILabel implementa una verificación interna:

```c++
void UILabel::draw(Renderer& renderer) {
    if (!isVisible) return; // Evita el redibujado de elementos ocultos
    renderer.drawText(text.c_str(), x, y, color, size);
}
```

2. Cálculo de Dimensiones: El ancho (`width`) del elemento se autocalcula en el constructor multiplicando el número de caracteres por el ancho de la fuente (`size * 6`), lo que permite que el sistema de colisiones o centrado funcione de forma precisa. ¿Te gustaría que redacte también la especificación técnica para el **UIButton**, incluyendo cómo detectaría el foco (focus) usando el `InputManager`?

## 🛠️ Configuración de Plataforma

El motor utiliza directivas de preprocesador para conmutar entre hardware y simulador:

Característica,ESP32 (Producción),Native (Desarrollo PC)
Gráficos,TFT_eSPI (SPI Bus),SDL2 (Window Manager)
Entrada,Botones Físicos (GPIO),Teclado (WASD / Flechas)
Tiempo,millis() Arduino,MockArduino (SDL_GetTicks)
Debug,Serial Monitor,Consola Estándar (stdout)

📝 Ejemplo de Implementación

```c++
#include "Scene.h"

class MainMenu : public Scene {
    void update(unsigned long deltaTime) override {
        if (engine.getInputManager().wasPressed(Input::BUTTON_A)) {
            // Cambiar de escena o iniciar juego
        }
    }
    
    void draw(Renderer& renderer) override {
        renderer.drawTextCentered("PRESS START", 120, COLOR_WHITE);
    }
};
```

## ⚙️ Requisitos
1. Entorno ESP32:
    - Framework Arduino para ESP32.
    - Librería TFT_eSPI (configurar User_Setup.h incluyendo definición de pin MISO para soporte DMA).

2. Entorno Native:
    - Compilador C++ (GCC/Clang).
    - Librería SDL2 instalada en el sistema.

Desarrollado para ser eficiente, rápido y fácil de extender.

## Filosofía

PixelRoot32 busca ofrecer:

- Arquitectura clara y extensible
- Rendimiento real en hardware limitado
- Flujo de trabajo moderno inspirado en engines de alto nivel
- Control total del hardware con una API simple

---
