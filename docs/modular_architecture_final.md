# Arquitectura de Compilación Modular en PixelRoot32

## 1. Descripción General y Objetivos
La Arquitectura de Compilación Modular en PixelRoot32 permite compilar versiones especializadas y ligeras del motor de videojuegos de manera condicional. El objetivo principal es **reducir drásticamente el tamaño del firmware final y el uso de memoria RAM estática** (elementos fundamentales en hardware embebido como el ESP32). 

Esto se logra mediante la exclusión de subsistemas completos que no son utilizados por un juego particular (como la Interfaz de Usuario, Motor de Audio, Sistema Físico o Partículas), respetando la baja cohesión inicial del motor y centralizando todo el control en el archivo `platformio.ini`.

---

## 2. Detalle de Cambios Realizados

La refactorización respetó el encapsulamiento original de los módulos e inyectó un paradigma de "Mix-ins" en los scripts y un manejo tipado en C++.

### 2.1. Archivos Modificados y Creados
*   **`include/platforms/PlatformDefaults.h`**: Se definieron valores por defecto (en `1`) para todas las macros explícitas `PIXELROOT32_ENABLE_*` si la configuración de compilador no las inyecta. Esto garantiza que el core mantenga el comportamiento clásico "Full-Features" si no se invoca un perfil.
*   **`include/core/EngineModules.h` (NUEVO)**: Interfaz central y traductor universal. Convierte las directivas del preprocesador (`#define`) creadas en la cadena de construcción, a constantes verificadas por el compilador (`constexpr bool`) para C++17.
*   **`include/core/Engine.h` y `src/core/Engine.cpp`**: Desacoplamiento lógico y físico de la instanciación de *Audio* (`AudioEngine` y `MusicPlayer`).
*   **`include/core/Scene.h` y `src/core/Scene.cpp`**: Desacoplamiento lógico del subsistema de *Físicas* (`CollisionSystem`).
*   **Archivos de Interfaz (`graphics/ui/`) y Partículas (`graphics/particles/`)**: 26 archivos (`.h` y `.cpp`) fueron modificados para envolver la totalidad de su código debajo de una comprobación macro. Al desactivarse, los ficheros compilan literalmente en blanco.
*   **`platformio.ini`**: Modernización y seccionamiento de los `env` a múltiples herencias de perfiles funcionales. 

### 2.2. Semántica de Compuestos y el Enlazador (Linker)
*   **Direccionalidad Única**: Se utiliza obligatoriamente el formato explícito ENABLE binario (`-D PIXELROOT32_ENABLE_AUDIO=0`), desestimando las clásicas macros negativas (como "DISABLE_X").
*   **Dead Code Elimination agresivo**: Se forzaron directrices como `-ffunction-sections`, `-fdata-sections` de la mano con el _Link Flag_ `-Wl,--gc-sections` aislando los binarios y desechando el código no invocado. Además se incluyó `-fno-exceptions` y `-fno-rtti` para purgar más de 30KB en tablas irrelevantes de C++ dinámico para Embedded.

### 2.3. Lógica del Desacoplamiento Interno
*   **Subsistemas Centralizados (Audio y Físicas)**:
    Las clases orquestadoras ahora comprueban condiciones combinadas:
    *   **Cabeceras (`#if`)**: Aíslan la declaración física de los objetos para evitar consumir stack / BSS memory cuando no se construyen.
    *   **Lógica Funcional (`if constexpr`)**: Dentro del código fuente de `Engine::update` y similares, dirige lógicamente al compilador C++17 a ignorar invocaciones. Esto evita empapar las funciones largas con costuras de preprocesador (`#if/#endif`), manteniendo el código infinitamente más limpio y legible.
*   **Subsistemas Ad-Hoc (UI y Partículas)**:
    Ya que la instanciación de un `UIButton` lo efectúa el usuario directamente en el archivo final del juego en vez de reservarse memoria en el Engine, los subsistemas enteros se resguardan internamente mediante *Library Guard Macros*. 

---

## 3. Ejemplos de Implementación del Sistema

### Traductor Constante (C++17) en `EngineModules.h`
```cpp
#pragma once
#include "platforms/EngineConfig.h"

namespace pixelroot32::modules {
    constexpr bool Audio     = PIXELROOT32_ENABLE_AUDIO;
    constexpr bool Physics   = PIXELROOT32_ENABLE_PHYSICS;
    constexpr bool UI        = PIXELROOT32_ENABLE_UI_SYSTEM;
    constexpr bool Particles = PIXELROOT32_ENABLE_PARTICLES;
}
```

### Uso Práctico Combinado en `Engine.h` y `Engine.cpp`
```cpp
// 1. Cabecera (Reserva de Memoria Condicional)
#if PIXELROOT32_ENABLE_AUDIO
    pixelroot32::audio::AudioEngine audioEngine; 
#endif

// 2. Archivo Fuente (Rama de Ejecución Condicional sin costo Runtime)
void Engine::update() {
    if constexpr (pixelroot32::modules::Audio) {
        audioEngine.update(); // Módulo purgardo transparente por Constexpr
    }
}
```

### Exclusión Transparente de Submódulos (Library Guards en UI)
```cpp
#include "core/EngineModules.h"
#if PIXELROOT32_ENABLE_UI_SYSTEM

class UIButton : public UIElement {
    // Implementación real del botón que compila 
    // solo si se requirió la UI...
};

#endif // PIXELROOT32_ENABLE_UI_SYSTEM
```

---

## 4. Instrucciones de Compilación (Perfiles de Motor)

La definición modular permite configurar el motor "Armando tu propio SDK" en `platformio.ini` utilizando mix-ins.

1. **Definir el Perfil Funcional Deseado (Mix-in Lógico)**:
```ini
[profile_arcade]
build_flags = 
    -D PIXELROOT32_ENABLE_AUDIO=1 
    -D PIXELROOT32_ENABLE_PHYSICS=1 
    -D PIXELROOT32_ENABLE_PARTICLES=1 
    -D PIXELROOT32_ENABLE_UI_SYSTEM=0 ; Se descarta UI
```

2. **Crear el Binario Objetivo Integrado**:
El entorno final para compilar el juego fusionará las instrucciones del framework de hardware base (`base_esp32` u otro) junto con las limitantes del perfil de motor (`profile_arcade`).

```ini
[esp32_arcade]
extends = base_esp32, profile_arcade
build_flags = 
    ${base_esp32.build_flags}
    ${profile_arcade.build_flags}
```

3. **Ejecutar la compilación**:
En la línea de comandos, inyecta explícitamente el tipo de entorno unificado:
> `pio run -e esp32_arcade` 
(Opcionalmente con `pio run -t upload -e esp32_arcade`).

---

## 5. Compatibilidad y Recomendaciones de Mantenimiento

*   **Evitar Macros de Inhabilitación Directa:** A nivel proyecto principal y librerías externas que consuman clases de PixelRoot32, *siempre* se debe depender lógicamente de los valores `1` o `0` garantizados desde `PlatformDefaults.h`. No intente introducir convenciones arcaicas `PIXELROOT_DISABLE_AUDIO`.
*   **Recolección por Dead Code Elimination:** Si tu entorno usa una compilación `profile_full`, pero nunca llamas explícitamente al código de UI instanciando sus clases en el código del juego, el compilador purgará las funciones de igual manera. Aún así, es fundamental instruir explícitamente un perfil con `PIXELROOT32_ENABLE_x=0` si tu juego no lo usa para despresionar la compilación física y ahorrar tiempos masivos de indexación cruzada.
*   **Creación de Nuevos Componentes Físicos / UI**: Todo archivo **nuevo** que se añada a un bloque fuertemente descentralizado (es decir, a `graphics/ui/` o `graphics/particles/`) **debe envolverse** exhaustivamente en el bloque `#if PIXELROOT32_ENABLE_...`. Olvidarlo generara fallas de parseo C++ (Type mismatches o invocación no referenciada a `EngineModules`).
*   **Comportamiento del "Intellisense" (IDE):** Al desactivar un módulo completo vía PIO (por ejemplo, UI), el visualizador del código en C++ como _Clangd_ oscurecerá al instante el código correspondiente. Se recomienda diseñar las características principales en `esp32_full` o `native_full` durante el ciclo iterativo diario, y cambiar al perfil minificado únicamente en etapas de Release and Quality Assurance.
