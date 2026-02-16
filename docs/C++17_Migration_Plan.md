# Plan de Trabajo por Fases: Migración a C++17 para PixelRoot32

Este documento detalla la estrategia para migrar el motor **PixelRoot32** de C++11 a C++17, optimizando para ESP32 y mejorando la seguridad de tipos y gestión de memoria.

## Fase 1: Infraestructura de Configuración (Build System) (Completado)

**Objetivo:** Habilitar el soporte de compilación C++17 y asegurar que el entorno sea estable.

### 1.1 Actualización de Estándar

- **Archivo:** `platformio.ini`
- **Acción:** Cambiar `-std=c++11` por `-std=gnu++17` en `build_flags` para todos los entornos (`env:esp32c3`, `env:esp32dev`, `env:native`).
- **Justificación:** `gnu++17` habilita las extensiones de C++17 junto con extensiones GNU útiles para embebidos, necesario para características como `if constexpr` y `std::string_view`.

### 1.2 Verificación de Librerías

- **Acción:** Compilar el proyecto "Clean" para verificar compatibilidad de:
  - `olikraus/U8g2`
  - `bodmer/TFT_eSPI`
  - `mathertel/OneButton`
  - `powerbroker2/SafeString`
- **Justificación:** Validar que ninguna dependencia externa rompa la compilación con el nuevo estándar antes de modificar el código propio.

## Fase 2: Estrategia de Eliminación de Macros (Completado)

**Objetivo:** Reemplazar macros de preprocesador por características del lenguaje seguras en tiempo de compilación.

### 2.1 Constantes de Configuración (Completado)

- **Archivos:** `include/platforms/EngineConfig.h`, `include/core/Scene.h`
- **Acción:** Convertir `#define` numéricos en `constexpr`.

    ```cpp
    // Antes
    #define MAX_ENTITIES 64
    // Después
    inline constexpr int MaxEntities = 64;
    ```

- **Justificación:** `constexpr` respeta el scope y tiene tipo fuerte, evitando errores de sustitución de texto sutiles.

### 2.2 Reemplazo de Compilación Condicional (`#ifdef`) (Completado)

- **Archivos:** `Renderer.cpp`, `NativeAudioScheduler.cpp`
- **Acción:** Reemplazar bloques `#ifdef PIXELROOT32_ENABLE_PROFILING` por `if constexpr`.

    ```cpp
    // Antes
    #ifdef PIXELROOT32_ENABLE_PROFILING
        updateMectrics();
    #endif
    
    // Después
    if constexpr (pixelroot32::platforms::config::EnableProfiling) {
        updateMetrics();
    }
    ```

- **Justificación:** `if constexpr` compila la rama solo si la condición es verdadera, manteniendo el binario limpio pero permitiendo validación de sintaxis en el código "desactivado".

## Fase 3: Modernización de Memoria (Específico ESP32) (Completado)

**Objetivo:** Reducir uso de RAM y fragmentación del Heap.

### 3.1 Adopción de `std::string_view` (Completado)

- **Archivos:** `UIButton.h/cpp`, `UILabel.h/cpp`, `UICheckbox.h/cpp`
- **Acción:** Cambiar constructores y métodos que reciben `std::string` por valor/referencia a `std::string_view`.

    ```cpp
    // Antes
    UILabel(std::string text, ...);
    // Después
    UILabel(std::string_view text, ...);
    ```

- **Justificación:** Evita copias innecesarias y asignaciones dinámicas de memoria para literales de cadena, crítico para el rendimiento y la estabilidad en ESP32.

### 3.2 Constantes en Flash (Completado)

- **Acción:** Asegurar que las grandes tablas de datos (si existen, como fuentes o sprites) o constantes globales usar `static constexpr` y atributos de sección si es necesario.
- **Justificación:** Garantizar que los datos constantes vivan en la Flash (PROGMEM) y no consuman RAM preciosa.

## Fase 4: Refactor de Arquitectura y Sintaxis

**Objetivo:** Mejorar legibilidad y seguridad del código.

### 4.1 Structured Bindings

- **Archivo:** `src/graphics/ui/UIAnchorLayout.cpp`
- **Acción:** Modernizar iteración de mapas/pares.

    ```cpp
    // Antes
    for (const auto& item : anchoredElements) {
        auto* element = item.first;
        auto anchor = item.second;
        ...
    }
    // Después
    for (const auto& [element, anchor] : anchoredElements) {
        ...
    }
    ```

- **Justificación:** Código más limpio y directo, eliminando `first` y `second` que carecen de semántica.

### 4.2 `std::optional` para Retornos Nulos

- **Archivos:** `SceneManager.cpp`, `UILayout` classes.
- **Acción:** Evaluar funciones que retornan `ObjectMapper*` o punteros crudos que pueden fallar.
  - *Nota:* Se usará con cautela en "rutas calientes" (update loop) por el ligero overhead, pero es ideal para lógica de inicialización o configuración.
- **Justificación:** Explicita la posibilidad de "ausencia de valor" en la firma de la función, obligando a quien llama a manejar el caso de error.

### 4.3 Range-based For Loops

- **Archivos:** `Scene.cpp` (`update`, `draw`)
- **Acción:** Cambiar bucles indexados por bucles de rango donde no se necesite el índice.
- **Justificación:** Mejora la legibilidad y previene errores "off-by-one".

## Fase 5: Seguridad y Estabilidad

**Objetivo:** Prevenir fugas de memoria y crashes.

### 5.1 Adopción de `std::make_unique`

- **Archivos:** Tests unitarios (`test_ui_layouts.cpp`, etc.) y `Scene` setup.
- **Acción:** Reemplazar `new` directo por `std::make_unique<T>()` donde se use `std::unique_ptr` o auto-gestión.
- **Justificación:** Garantiza seguridad ante excepciones (en tiempo de construcción) y previene fugas si se olvida el `delete` (cuando se combina con smart pointers).

### 5.2 Confirmación de No-Excepciones

- **Acción:** Verificar flag `-fno-exceptions`.
- **Justificación:** En microcontroladores, el soporte de excepciones añade un peso considerable al binario y overhead en tiempo de ejecución. El código debe diseñarse para no lanzar excepciones.
