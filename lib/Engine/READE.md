# ESP32 Game Engine

Un motor de juegos ligero y modular desarrollado en **C++** diseñado específicamente para microcontroladores **ESP32**, con una capa de abstracción que permite la simulación nativa en PC mediante **SDL2**.

> **Créditos:** Esta librería está 100% inspirada en el proyecto [ESP32-Game-Engine](https://github.com/nbourre/ESP32-Game-Engine) de nbourre.

## 🚀 Estructura del Proyecto

La arquitectura del motor separa la lógica de alto nivel del hardware (HAL), permitiendo un desarrollo multiplataforma eficiente.

```text
Engine/
├── include/                # Cabeceras públicas (.h)
│   ├── core/               # Núcleo: Actor.h, Entity.h, Scene.h
│   ├── graphics/           
│   │   ├── ui/             # NUEVO: Componentes de Interfaz
│   │   │   ├── UIElement.h # Clase base para UI
│   │   │   ├── UILabel.h   # Etiquetas de texto
│   │   │   └── UIButton.h  # Botones interactivos
│   │   ├── Renderer.h      # API unificada de dibujo
│   │   └── ...
│   ├── input/              # InputManager.h
│   └── physics/            # CollisionSystem.h
├── src/                    
│   ├── core/               # Implementación de lógica de base
│   ├── graphics/           
│   │   ├── ui/             # NUEVO: Implementación de lógica de UI
│   │   │   ├── UILabel.cpp
│   │   │   └── UIButton.cpp
│   │   └── Renderer.cpp
│   └── ...
```

## 🏗️ Componentes Principales

- Core: Controla el `SceneManager` y el ciclo de actualización (update/draw).

- Renderer: API unificada de dibujo. En ESP32, utiliza `TFT_eSprite` para implementar Double Buffering, eliminando el parpadeo de pantalla.

- InputManager: Abstrae los botones físicos (GPIO) y las teclas del PC en comandos lógicos (UP, DOWN, A, B).

- CollisionSystem: Provee detección de colisiones AABB (cajas) y soporte para movimientos basados en rejilla (Grid-based).

## ⚡ Optimizaciones de Alto Rendimiento

El motor implementa técnicas avanzadas para exprimir la potencia del ESP32:

Sistema de Partículas (Pooled Memory)
Para manejar explosiones y efectos visuales sin degradar los FPS:

- Memory Pooling: Uso de arrays estáticos para reutilizar partículas, evitando la fragmentación de memoria (heap fragmentation) causada por new y delete.

- Trigonometría Pre-calculada: Los cálculos de dispersión circular se realizan en el momento de la explosión, manteniendo los updates posteriores en aritmética escalar simple.

- Auto-clipping: Gestión automática del ciclo de vida de las entidades cuando salen de los límites de la pantalla.

### Renderizado Asíncrono vía DMA

Implementación de Direct Memory Access para desacoplar la CPU del bus SPI:

- Non-blocking Transfers: Mediante pushImageDMA, el motor inicia el envío del frame a la pantalla y libera la CPU inmediatamente.

- Paralelismo Real: La lógica de físicas (update) se ejecuta simultáneamente mientras el hardware SPI transfiere los datos de imagen.

- Zero Tearing: Uso de barreras de sincronización (dmaWait) para garantizar la integridad del buffer antes de iniciar un nuevo ciclo de dibujo.

## 🎨 Sistema de Interfaz de Usuario (UI)

El motor incluye un sistema de UI jerárquico que hereda de Entity, lo que permite que los elementos de interfaz se gestionen automáticamente dentro del SceneManager.

### Jerarquía de Clases
- **UIElement**: Clase base que añade propiedades de control de interfaz como `isVisible` y `isEnabled`.

- **UILabel**: Componente especializado en renderizado de texto con soporte para alineación dinámica.

- **UIButton**: (Próximamente) Elemento interactivo que responde a eventos del `InputManager`.

### Características de UILabel

- **Auto-centrado:** Método centerX(int regionWidth) para posicionamiento automático basado en la longitud del texto y el tamaño de la fuente.

- **Gestión de Visibilidad:** Control binario mediante setVisible(bool) para renderizado condicional (ej: mensajes de "Game Over" o "Blinking Start").

- **Eficiencia en Microcontroladores:** Utiliza el motor de fuentes nativo del Renderer para evitar sobrecarga de memoria.

### Ejemplo de Implementación en Escena

```c++
#include "graphics/ui/UILabel.h"

class GameScene : public Scene {
    UI::UILabel* lblStart;

    void init() override {
        // Inicialización: Texto, X, Y, Color, Tamaño
        lblStart = new UI::UILabel("PRESS A TO START", 0, 150, COLOR_WHITE, 1);
        lblStart->centerX(SCREEN_WIDTH); // Centrado automático
        addEntity(lblStart);             // Registro en el motor
    }

    void update(unsigned long deltaTime) override {
        // Lógica de parpadeo o visibilidad
        if (gameStarted) {
            lblStart->setVisible(false);
        }
        Scene::update(deltaTime); // Importante para procesar lógica de UI
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