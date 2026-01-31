# Plan de Implementación: Layouts de UI con Posición Fija (HUD/Overlays)

Este documento detalla el plan para permitir que los `UILayout` en el motor PixelRoot32 ignoren el desplazamiento de la `Camera2D`, facilitando la creación de HUDs, menús y capas de interfaz estáticas.

## 1. Análisis de la Situación Actual

Actualmente, el `Renderer` aplica globalmente un `xOffset` y `yOffset` (configurados por la cámara) a todas las operaciones de dibujo. Debido a que los elementos de UI heredan de `Entity` y se dibujan a través del `Renderer`, estos se ven afectados por el movimiento de la cámara en el mundo.

## 2. Objetivos

- Permitir que un `UILayout` (y todos sus hijos) se dibuje en coordenadas de pantalla puras.
- Mantener la compatibilidad con el sistema de capas de renderizado existente.
- Minimizar el impacto en el rendimiento, especialmente para plataformas como ESP32.

## 3. Cambios Propuestos

### A. Core Graphics (Renderer)

Modificar la clase `Renderer` para permitir la desactivación temporal del offset de cámara.

- **Archivo:** `include/graphics/Renderer.h` y `src/graphics/Renderer.cpp`
- **Cambios:**
  - Añadir miembro privado `bool useCameraOffset = true`.
  - Añadir métodos `setUseCameraOffset(bool enable)` y `getUseCameraOffset()`.
  - Actualizar todas las funciones de dibujo (`drawPixel`, `drawFilledRectangle`, `drawText`, `drawSprite`, etc.) para que utilicen el offset solo si `useCameraOffset` es `true`.

### B. Módulo de UI (UILayout)

Añadir la propiedad de posición fija a los layouts.

- **Archivo:** `include/graphics/ui/UILayout.h`
- **Cambios:**
  - Añadir miembro protegido `bool fixedPosition = false`.
  - Añadir métodos `setFixedPosition(bool fixed)` y `isFixedPosition()`.

### C. Implementación en Layouts Específicos

Actualizar el comportamiento de dibujado.

- **Archivos:** `src/graphics/ui/UIVerticalLayout.cpp`, `src/graphics/ui/UIHorizontalLayout.cpp`, etc.
- **Cambios en `draw(Renderer& renderer)`:**

    ```cpp
    void UIVerticalLayout::draw(Renderer& renderer) {
        if (!isVisible) return;
        
        // Guardar estado previo y desactivar offset si es fijo
        bool previousState = renderer.getUseCameraOffset();
        if (fixedPosition) renderer.setUseCameraOffset(false);
        
        // ... lógica de dibujado existente (clear, draw hijos) ...
        
        // Restaurar estado previo
        renderer.setUseCameraOffset(previousState);
    }
    ```

## 4. Consideraciones de Rendimiento

- **Impacto CPU:** Mínimo. La comprobación del flag en el `Renderer` es una operación de bajo costo.
- **Impacto Memoria:** Despreciable (1 byte por Renderer y 1 byte por Layout).

## 5. Casos de Uso

1. **HUD:** Elementos de estado (vida, energía) que siempre están visibles en la misma posición de la pantalla.
2. **Menús de Pausa:** Interfaces que bloquean el juego y deben estar centradas independientemente de dónde se encuentre la cámara.
3. **Overlays de Notificación:** Mensajes temporales que aparecen en pantalla.

## 6. Verificación

Se creará un ejemplo de prueba (`FixedUITest`) que consistirá en:

1. Una cámara que se mueve automáticamente de izquierda a derecha.
2. Un `UIVerticalLayout` con `fixedPosition = true` que contenga botones y etiquetas.
3. Verificación visual de que el Layout permanece estático mientras el fondo se desplaza.
