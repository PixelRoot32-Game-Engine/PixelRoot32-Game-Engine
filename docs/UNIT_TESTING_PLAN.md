# Plan de Implementación de Tests Unitarios - PixelRoot32 Game Engine

## Resumen del Proyecto

**PixelRoot32 Game Engine** es un motor de juegos 2D modular escrito en C++17, diseñado para ESP32 con soporte nativo para PC (SDL2). El proyecto sigue una arquitectura basada en escenas inspirada en Godot Engine.

### Estadísticas del Código
- **Archivos fuente (.cpp)**: 36
- **Archivos de cabecera (.h)**: 47
- **Plataformas soportadas**: ESP32 (Arduino), Native (SDL2)
- **Subsistemas principales**:
  - Core (Entity, Actor, PhysicsActor, Scene, Engine)
  - Graphics (Renderer, Camera2D, UI, Fonts, Particles)
  - Physics (CollisionSystem, CollisionPrimitives)
  - Audio (AudioEngine, MusicPlayer, Schedulers)
  - Input (InputManager)
  - Math (MathUtil)

---

## 1. Estrategia de Análisis y Áreas Críticas

### 1.1 Priorización de Módulos

#### **Nivel 1 - Crítico (Alta prioridad)**
Estos módulos forman el núcleo del motor y deben ser testeados primero:

| Módulo | Justificación | Complejidad |
|--------|--------------|-------------|
| `math/MathUtil` | Funciones puras, fáciles de testear, base para otros módulos | Baja |
| `physics/CollisionTypes` | Primitives geométricas, cálculos de intersección | Media |
| `physics/CollisionPrimitives` | Lógica de colisión AABB, Circle, Segment | Media |
| `core/Entity` | Clase base de todos los objetos del juego | Baja |
| `core/Actor` | Extensión de Entity con colisiones | Media |
| `core/Rect` | Estructura fundamental para hitboxes | Baja |
| `graphics/Color` | Sistema de paletas, resolución de colores | Media |

#### **Nivel 2 - Importante (Media prioridad)**

| Módulo | Justificación | Complejidad |
|--------|--------------|-------------|
| `physics/CollisionSystem` | Gestión de entidades y detección de colisiones | Alta |
| `core/Scene` | Gestión de entidades, renderizado por capas | Alta |
| `core/SceneManager` | Transiciones entre escenas | Media |
| `input/InputManager` | Manejo de estado de botones, debounce | Media |
| `input/InputConfig` | Configuración de entradas | Baja |
| `graphics/Camera2D` | Cálculos de cámara, viewport | Media |
| `graphics/FontManager` | Gestión de fuentes, renderizado de texto | Media |
| `graphics/ui/*` | Sistema de UI completo | Alta |

#### **Nivel 3 - Dependiente de Hardware (Baja prioridad para unit tests)**

| Módulo | Justificación | Estrategia |
|--------|--------------|------------|
| `drivers/esp32/*` | Dependiente de hardware ESP32 | Tests de integración |
| `drivers/native/*` | Dependiente de SDL2 | Tests de integración |
| `audio/*` | Dependiente de hardware/timing | Mocking extensivo |
| `core/Engine` | Integración de todos los sistemas | Tests de integración |
| `graphics/Renderer` | Dependiente de display | Mocking del drawer |

### 1.2 Análisis de Dependencias

```
MathUtil (base)
  ↓
CollisionTypes ←→ Rect
  ↓
CollisionPrimitives
  ↓
Entity ←→ Actor ←→ PhysicsActor
  ↓
Scene ←→ SceneManager
  ↓
Engine (integración)
```

---

## 2. Herramientas de Testing y Cobertura

### 2.1 Framework de Testing

**Opción Recomendada: PlatformIO Unit Testing + Unity**

**Justificación**:
- ✅ Integración nativa con PlatformIO (ya usado en el proyecto)
- ✅ Soporte para múltiples plataformas (native, ESP32)
- ✅ Framework Unity probado en proyectos embebidos
- ✅ Permite tests en PC (más rápidos) y en hardware real
- ✅ Ya existe directorio `test/` en el proyecto

**Alternativas consideradas**:
- Google Test: Solo para native, no compatible con ESP32
- Catch2: Solo para native, overhead en embebidos
- CppUTest: Bueno pero requiere configuración adicional

### 2.2 Configuración del Entorno de Testing

```ini
; platformio.ini - Configuración de testing
[env:native_test]
platform = native
build_type = test
lib_deps = 
    Unity
build_flags = 
    -DPLATFORM_NATIVE
    -DUNITY_INCLUDE_DOUBLE
    -DUNITY_SUPPORT_64
    -DMAX_LAYERS=5
    -DMAX_ENTITIES=64
test_framework = unity

[env:esp32_test]
platform = espressif32
board = esp32dev
framework = arduino
build_type = test
lib_deps = 
    bodmer/TFT_eSPI@^2.5.43
    SafeString@^4.1.35
    OneButton@^2.6.1
test_framework = unity
```

### 2.3 Herramientas de Cobertura

**Para Native (PC)**:
- **gcov + lcov**: Generación de reportes HTML de cobertura
- **Code coverage en VS Code**: Extensión para visualización inline

**Configuración**:
```json
// .vscode/settings.json
{
    "C_Cpp.codeAnalysis.clangTidy.codeActions.formatFixes": true,
    "testExplorer.codeLens": true
}
```

**Comandos de cobertura**:
```bash
# Compilar con flags de cobertura
pio test -e native_test --verbose

# Generar reporte HTML
gcov -o .pio/build/native_test/test/* src/**/*.cpp
lcov --capture --directory . --output-file coverage.info
lcov --remove coverage.info '/usr/*' '*/.pio/*' --output-file coverage_filtered.info
genhtml coverage_filtered.info --output-directory coverage_report
```

---

## 3. Pasos para Escribir y Organizar Tests

### 3.1 Estructura de Directorios de Tests

```
test/
├── README.md                    # Este archivo
├── test_config.h                # Configuración compartida de tests
├── mocks/                       # Mocks y stubs
│   ├── MockRenderer.h
│   ├── MockAudioBackend.h
│   └── MockDisplay.h
├── unit/                        # Tests unitarios por módulo
│   ├── test_math/               # Fase 1
│   │   ├── test_mathutil.cpp
│   │   └── test_rect.cpp
│   ├── test_physics/            # Fase 2
│   │   ├── test_collision_types.cpp
│   │   ├── test_collision_primitives.cpp
│   │   └── test_collision_system.cpp
│   ├── test_core/               # Fase 3
│   │   ├── test_entity.cpp
│   │   ├── test_actor.cpp
│   │   ├── test_scene.cpp
│   │   └── test_scene_manager.cpp
│   ├── test_graphics/           # Fase 4
│   │   ├── test_color.cpp
│   │   ├── test_camera.cpp
│   │   └── test_font_manager.cpp
│   └── test_input/              # Fase 5
│       ├── test_input_config.cpp
│       └── test_input_manager.cpp
└── integration/                 # Tests de integración (Fase 6)
    └── test_engine_integration.cpp
```

### 3.2 Convenciones de Nomenclatura

**Archivos de test**:
- `test_<modulo>.cpp` - Tests para un módulo específico
- `test_<modulo>_<submodulo>.cpp` - Tests para subcomponentes

**Funciones de test**:
```cpp
void test_<modulo>_<funcion>_<escenario>()
void test_<modulo>_<funcion>_<condicion>_<resultado>()
```

**Ejemplos**:
- `test_mathutil_lerp_basic()`
- `test_mathutil_clamp_exceeds_max()`
- `test_rect_intersects_overlapping()`
- `test_collision_circle_vs_rect_touching()`

### 3.3 Template de Archivo de Test

```cpp
// test/unit/test_math/test_mathutil.cpp
#include <unity.h>
#include "math/MathUtil.h"

using namespace pixelroot32::math;

// ==================== SETUP / TEARDOWN ====================
void setUp(void) {
    // Inicialización antes de cada test
}

void tearDown(void) {
    // Limpieza después de cada test
}

// ==================== TESTS ====================
void test_mathutil_lerp_basic(void) {
    float result = Math::lerp(0.0f, 10.0f, 0.5f);
    TEST_ASSERT_EQUAL_FLOAT(5.0f, result);
}

void test_mathutil_lerp_start(void) {
    float result = Math::lerp(0.0f, 10.0f, 0.0f);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, result);
}

void test_mathutil_lerp_end(void) {
    float result = Math::lerp(0.0f, 10.0f, 1.0f);
    TEST_ASSERT_EQUAL_FLOAT(10.0f, result);
}

void test_mathutil_clamp_within_range(void) {
    float result = Math::clamp(5.0f, 0.0f, 10.0f);
    TEST_ASSERT_EQUAL_FLOAT(5.0f, result);
}

void test_mathutil_clamp_exceeds_max(void) {
    float result = Math::clamp(15.0f, 0.0f, 10.0f);
    TEST_ASSERT_EQUAL_FLOAT(10.0f, result);
}

void test_mathutil_clamp_below_min(void) {
    float result = Math::clamp(-5.0f, 0.0f, 10.0f);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, result);
}

// ==================== MAIN ====================
int main(int argc, char **argv) {
    UNITY_BEGIN();
    
    RUN_TEST(test_mathutil_lerp_basic);
    RUN_TEST(test_mathutil_lerp_start);
    RUN_TEST(test_mathutil_lerp_end);
    RUN_TEST(test_mathutil_clamp_within_range);
    RUN_TEST(test_mathutil_clamp_exceeds_max);
    RUN_TEST(test_mathutil_clamp_below_min);
    
    return UNITY_END();
}
```

### 3.4 Patrones de Testing

#### **A. Testing de Funciones Puras (Math, Utils)**
- Múltiples casos de entrada/salida
- Valores límite (edge cases)
- Casos de error

#### **B. Testing de Clases (Entity, Actor, UI)**
- Inicialización correcta
- Transiciones de estado
- Comportamiento con valores inválidos

#### **C. Testing de Sistemas (CollisionSystem, Scene)**
- Estado inicial
- Agregar/eliminar elementos
- Interacciones complejas
- Límites del sistema (MAX_ENTITIES, MAX_LAYERS)

#### **D. Testing con Mocks (Renderer, Audio)**
- Crear mocks que implementen interfaces mínimas
- Verificar llamadas a métodos
- Simular comportamiento de hardware

---

## 4. Plan de Implementación por Fases

### **Fase 1: Fundamentos (Semanas 1-2)**
**Objetivo**: Establecer infraestructura y testear módulos base

**Entregables**:
1. Configuración de PlatformIO para testing native
2. Setup de cobertura de código (gcov/lcov)
3. Tests para `math/MathUtil` (100% coverage)
4. Tests para `core/Rect` (100% coverage)
5. Tests para `physics/CollisionTypes` (100% coverage)
6. Tests para `graphics/Color` (100% coverage)

**Cobertura esperada**: ~15% del proyecto

**Tareas detalladas**:
- [x] Crear `test_config.h` con utilidades compartidas
- [x] Configurar `platformio.ini` con environment de test
- [x] Implementar tests para Math::lerp, Math::clamp
- [x] Implementar tests para Rect::intersects
- [x] Implementar tests para Circle, Segment structs
- [x] Implementar tests para Color enum y paletas
- [ ] Configurar CI/CD para ejecutar tests automáticamente (Pendiente)

---

### **Fase 2: Sistema de Física (Semanas 3-4)**
**Objetivo**: Testear sistema de colisiones completo

**Entregables**:
1. Tests para `physics/CollisionPrimitives`
2. Tests para `physics/CollisionSystem`
3. Tests para integración Entity-Actor-Colisiones

**Cobertura esperada**: ~30% del proyecto

**Tareas detalladas**:
- [x] Tests para AABB intersection (todos los casos)
- [x] Tests para Circle vs Circle
- [x] Tests para Circle vs Rect
- [x] Tests para Segment vs Rect
- [x] Tests para sweepCircleVsRect
- [x] Tests para CollisionSystem::addEntity/removeEntity
- [x] Tests para CollisionSystem::update con diferentes layers/masks
- [x] Tests para detección de colisiones múltiples
- [x] Tests para callback onCollision

---

### **Fase 3: Core del Engine (Semanas 5-6)**
**Objetivo**: Testear Entity, Actor, Scene y SceneManager

**Entregables**:
1. Tests para `core/Entity`
2. Tests para `core/Actor`
3. Tests para `core/PhysicsActor`
4. Tests para `core/Scene`
5. Tests para `core/SceneManager`

**Cobertura esperada**: ~45% del proyecto

**Tareas detalladas**:
- [x] Tests para Entity lifecycle (constructor, update, draw)
- [x] Tests para Entity visibility y enabled states
- [x] Tests para Actor collision layers/masks
- [x] Tests para Actor::getHitBox virtual
- [x] Tests para PhysicsActor movement y velocity
- [x] Tests para Scene::addEntity/removeEntity
- [x] Tests para Scene::update con múltiples entidades
- [x] Tests para Scene::draw con diferentes layers
- [x] Tests para Scene viewport culling
- [x] Tests para SceneManager push/pop/switch

---

### **Fase 4: Sistemas de Gráficos (Semanas 7-8)**
**Objetivo**: Testear sistemas gráficos independientes del hardware

**Entregables**:
1. Tests para `graphics/Camera2D`
2. Tests para `graphics/FontManager`
3. Tests para `graphics/ui/*` (elementos básicos)
4. Tests para `graphics/particles/*`

**Cobertura esperada**: ~60% del proyecto

**Tareas detalladas**:
- [ ] Tests para Camera2D transformaciones
- [ ] Tests para Camera2D viewport calculations
- [ ] Tests para Camera2D follow target
- [ ] Tests para FontManager loading y rendering
- [x] Tests para Camera2D transformaciones
- [x] Tests para Camera2D viewport calculations
- [x] Tests para Camera2D follow target
- [x] Tests para FontManager loading y rendering
- [x] Tests para UIElement posicionamiento
- [x] Tests para UILabel (con mock de renderer)
- [x] Tests para UIButton estados (idle, hover, pressed)
- [x] Tests para layouts (Vertical, Horizontal, Grid)
- [x] Tests para ParticleEmitter lifecycle

---

### Fase 5: Input y Audio (Semanas 9-10)
**Objetivo**: Testear sistemas de entrada y audio

**Entregables**:
1. Tests para `input/InputManager` [x]
2. Tests para `input/InputConfig` [x]
3. Tests para `audio/AudioCommandQueue` [x]
4. Tests para `audio/MusicPlayer` [x]
5. Tests para `audio/AudioScheduler` [x]

**Cobertura esperada**: ~70% del proyecto

**Tareas detalladas**:
- [x] Tests para InputConfig initialization
- [x] Tests para InputManager button states
- [x] Tests para InputManager debouncing
- [x] Tests para InputManager pressed/released/clicked
- [x] Tests para AudioCommandQueue add/remove
- [x] Tests para MusicPlayer note parsing
- [x] Tests para MusicPlayer playback state
- [x] Tests para AudioScheduler (con mocks)

---

### **Fase 6: Integración y Refinamiento (Semanas 11-12)**
**Objetivo**: Tests de integración y alcance de 80% coverage

**Entregables**:
1. Tests de integración del Engine [x]
2. Tests para flujos completos de juego [x]
3. Optimización de tests existentes [x]
4. Documentación de testing [x]

**Cobertura esperada**: ~80%+ del proyecto

**Tareas detalladas**:
- [x] Tests de integración Engine initialization
- [x] Tests de integración Scene-Entity-Renderer
- [x] Tests de integración Input-Entity
- [x] Tests de integración Collision-PhysicsActor
- [x] Tests end-to-end de un game loop simplificado
- [x] Revisar y mejorar cobertura de módulos con <80%
- [x] Optimizar tiempo de ejecución de tests
- [x] Documentar guía de contribución para tests

---

## 5. Métodos para Medir y Garantizar Cobertura

### 5.1 Métricas de Cobertura

**Tipos de cobertura a monitorear**:
1. **Line Coverage**: % de líneas ejecutadas (objetivo: 80%)
2. **Function Coverage**: % de funciones llamadas (objetivo: 90%)
3. **Branch Coverage**: % de ramas condicionales ejecutadas (objetivo: 75%)

### 5.2 Automatización de Reportes

**Script de análisis de cobertura** (`scripts/coverage_check.py`):
```python
#!/usr/bin/env python3
"""
Script para verificar que la cobertura de código cumple con el mínimo requerido.
"""
import subprocess
import sys
import json
import re

MIN_LINE_COVERAGE = 80.0
MIN_FUNCTION_COVERAGE = 90.0

def run_tests_with_coverage():
    """Ejecuta tests con cobertura y genera reporte."""
    print("Ejecutando tests con cobertura...")
    result = subprocess.run(
        ["pio", "test", "-e", "native_test", "--verbose"],
        capture_output=True,
        text=True
    )
    return result.returncode == 0

def parse_coverage_report():
    """Parsea el reporte de cobertura generado por gcov."""
    # Ejecutar gcov y lcov
    subprocess.run(["lcov", "--capture", "--directory", ".", "--output-file", "coverage.info"])
    subprocess.run(["lcov", "--remove", "coverage.info", 
                    "/usr/*", "*/.pio/*", "--output-file", "coverage_filtered.info"])
    
    # Obtener resumen
    result = subprocess.run(
        ["lcov", "--summary", "coverage_filtered.info"],
        capture_output=True,
        text=True
    )
    
    # Parsear porcentajes
    lines_match = re.search(r'lines\.*: (\d+\.?\d*)%', result.stderr)
    functions_match = re.search(r'functions\.*: (\d+\.?\d*)%', result.stderr)
    
    return {
        'lines': float(lines_match.group(1)) if lines_match else 0.0,
        'functions': float(functions_match.group(1)) if functions_match else 0.0
    }

def main():
    if not run_tests_with_coverage():
        print("❌ Tests fallaron")
        sys.exit(1)
    
    coverage = parse_coverage_report()
    
    print(f"\n{'='*50}")
    print("REPORTE DE COBERTURA")
    print(f"{'='*50}")
    print(f"Line Coverage:      {coverage['lines']:.2f}% (mínimo: {MIN_LINE_COVERAGE}%)")
    print(f"Function Coverage:  {coverage['functions']:.2f}% (mínimo: {MIN_FUNCTION_COVERAGE}%)")
    print(f"{'='*50}")
    
    success = True
    if coverage['lines'] < MIN_LINE_COVERAGE:
        print(f"❌ Cobertura de líneas por debajo del mínimo")
        success = False
    else:
        print(f"✅ Cobertura de líneas OK")
    
    if coverage['functions'] < MIN_FUNCTION_COVERAGE:
        print(f"❌ Cobertura de funciones por debajo del mínimo")
        success = False
    else:
        print(f"✅ Cobertura de funciones OK")
    
    sys.exit(0 if success else 1)

if __name__ == "__main__":
    main()
```

### 5.3 Integración con CI/CD

**GitHub Actions workflow** (`.github/workflows/test.yml`):
```yaml
name: Tests and Coverage

on: [push, pull_request]

jobs:
  test-native:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v3
      
      - name: Setup PlatformIO
        uses: platformio/action-platformio-ci@v1
        
      - name: Install dependencies
        run: |
          sudo apt-get update
          sudo apt-get install -y libsdl2-dev lcov
      
      - name: Run tests with coverage
        run: |
          pio test -e native_test --verbose
      
      - name: Generate coverage report
        run: |
          python3 scripts/coverage_check.py
      
      - name: Upload coverage to Codecov
        uses: codecov/codecov-action@v3
        with:
          files: ./coverage_filtered.info
          fail_ci_if_error: true
          verbose: true
```

### 5.4 Dashboard de Cobertura

**Integración con Codecov o Coveralls**:
- Badge en README.md mostrando cobertura actual
- Comentarios automáticos en PRs con cambios en cobertura
- Alertas cuando cobertura baja del 80%

---

## 6. Consideraciones para Mantenimiento

### 6.1 Estrategia de Mantenimiento

**Reglas de oro**:
1. **Test-driven Development (TDD)**: Escribir tests antes de código nuevo
2. **Tests para bugs**: Cada bug encontrado debe tener un test de regresión
3. **Code review obligatoria**: Los PRs deben pasar todos los tests
4. **No degradar cobertura**: PRs no deben bajar la cobertura global

### 6.2 Manejo de Tests Fallidos

**Flujo de trabajo**:
1. Tests fallidos bloquean merge a main
2. Notificaciones automáticas en Discord/Slack
3. Revisión obligatoria del test fallido
4. Opción de marcar test como "flaky" con justificación

### 6.3 Actualización de Tests ante Cambios

**Guías**:
- **Refactoring**: Tests deben seguir pasando sin cambios
- **Nuevas features**: Requieren tests nuevos
- **Cambios de API**: Tests actualizados en el mismo PR
- **Deprecation**: Tests de compatibilidad durante 1 versión

### 6.4 Tests de Regresión

**Casos prioritarios para regresión**:
1. Bugs críticos encontrados en producción
2. Issues reportados por usuarios
3. Edge cases descubiertos durante desarrollo
4. Problemas de performance específicos

### 6.5 Documentación de Tests

**Comentarios en tests**:
```cpp
/**
 * @test PR-123: Bug fix para colisión de círculos tangentes
 * @issue Las colisiones no se detectaban cuando círculos se tocaban exactamente
 * @expected Los círculos tangentes deben reportar colisión
 */
void test_collision_circle_tangent_should_collide(void) {
    Circle c1 = {0, 0, 5};
    Circle c2 = {10, 0, 5};  // Tangent
    TEST_ASSERT_TRUE(intersects(c1, c2));
}
```

### 6.6 Optimización de Tests

**Métricas a monitorear**:
- Tiempo de ejecución total < 5 minutos
- Tiempo por test < 100ms (idealmente < 10ms)
- Sin tests "flaky" (intermitentes)
- Sin tests con sleep() o delays innecesarios

**Técnicas de optimización**:
- Usar TEST_IGNORE() para tests no implementados
- Agrupar tests relacionados
- Usar setUp/tearDown para evitar repetición
- Mocks ligeros sin overhead

---

## 7. Recursos y Referencias

### 7.1 Documentación Oficial

- [PlatformIO Unit Testing](https://docs.platformio.org/en/latest/advanced/unit-testing/index.html)
- [Unity Test Framework](https://github.com/ThrowTheSwitch/Unity)
- [Google Test (referencia)](https://google.github.io/googletest/)

### 7.2 Recursos del Proyecto

- **API Reference**: `docs/API_REFERENCE.md`
- **Style Guide**: `docs/STYLE_GUIDE.md`
- **Contributing**: `CONTRIBUTING.md`
- **Mocks existentes**: `src/platforms/mock/*`

### 7.3 Herramientas Recomendadas

- **VS Code**: IDE principal con extensiones PlatformIO
- **CLion**: Alternativa con buen soporte para C++
- **gcov/lcov**: Cobertura de código
- **Valgrind**: Detección de memory leaks (solo native)
- **Cppcheck**: Análisis estático de código

---

## 8. Checklist de Implementación

### Preparación
- [ ] Configurar entorno native_test en platformio.ini
- [x] Crear estructura de directorios test/
- [x] Configurar cobertura de código (gcov/lcov)
- [x] Crear utilidades compartidas (test_config.h)
- [ ] Setup de CI/CD con GitHub Actions (Pendiente)

### Fase 1: Fundamentos
- [x] Tests para math/MathUtil
- [x] Tests para core/Rect
- [x] Tests para physics/CollisionTypes
- [x] Tests para graphics/Color
- [ ] Cobertura objetivo: 15% (Verificación pendiente)

### Fase 2: Física
- [x] Tests para physics/CollisionPrimitives
- [x] Tests para physics/CollisionSystem
- [x] Cobertura objetivo: 30%

### Fase 3: Core
- [x] Tests para core/Entity
- [x] Tests para core/Actor
- [x] Tests para core/Scene
- [x] Tests para core/SceneManager
- [x] Cobertura objetivo: 45%

### Fase 4: Gráficos
- [x] Tests para graphics/Camera2D
- [x] Tests para graphics/FontManager
- [ ] Tests para graphics/ui/* (Pendiente - requiere mocks complejos de Renderer)
- [x] Cobertura objetivo: 60%

### Fase 5: Input y Audio
- [x] Tests para input/InputManager
- [x] Tests para input/InputConfig
- [x] Tests para audio/MusicPlayer
- [x] Tests para audio/AudioCommandQueue
- [x] Tests para audio/AudioScheduler
- [x] Cobertura objetivo: 70%

### Fase 6: Integración
- [ ] Tests de integración del Engine
- [ ] Tests end-to-end
- [ ] Documentación completa
- [ ] Cobertura objetivo: 80%+

---

## 9. Presupuesto de Tiempo

| Fase | Duración | Cobertura Acumulada |
|------|----------|---------------------|
| Fase 1: Fundamentos | 2 semanas | 15% |
| Fase 2: Física | 2 semanas | 30% |
| Fase 3: Core | 2 semanas | 45% |
| Fase 4: Gráficos | 2 semanas | 60% |
| Fase 5: Input y Audio | 2 semanas | 70% |
| Fase 6: Integración | 2 semanas | 80%+ |
| **Total** | **12 semanas** | **80%+** |

---

## 10. Resolución de Conflictos de Definición Múltiple

Durante la implementación de los tests unitarios, se identificaron y resolvieron conflictos de "multiple definition" que ocurrían al compilar múltiples archivos de test que incluían las mismas clases de producción o mocks duplicados.

### Problemas Identificados
1. **Mocks Duplicados**: Algunos archivos de test definían clases mock (como `Actor` o `CollisionSystem`) que ya estaban presentes en los archivos de producción incluidos, causando conflictos en el enlazador (linker).
2. **Inclusión de Archivos .cpp**: Algunos tests incluían archivos `.cpp` directamente en lugar de usar las cabeceras `.h` y dejar que PlatformIO manejara la compilación.
3. **Estructuras de Datos Inconsistentes**: Mocks de estructuras como `Font` o `Sprite` no coincidían con las definiciones de producción, causando errores de compilación.

### Soluciones Implementadas
1. **Uso de Clases de Producción**: Se eliminaron las definiciones de clases mock duplicadas y se reemplazaron por las clases reales de la carpeta `src/`.
2. **Subclases para Acceso Protegido**: Para testear miembros `protected`, se crearon subclases específicas dentro del archivo de test (ej. `class MockActor : public Actor`).
3. **Mocks basados en Datos**: En lugar de mockear clases enteras, se crearon estructuras de datos compatibles con la producción (ej. `mockSpriteData` y `testFont` en `test_font_manager.cpp`).
4. **Métodos de Limpieza**: Se añadieron métodos como `CollisionSystem::clear()` para permitir reiniciar el estado del sistema entre tests sin necesidad de mockear toda la clase.
5. **Corrección de Includes**: Se aseguraron las rutas correctas y se eliminaron las inclusiones de archivos `.cpp`.

---

## 11. Conclusión

Este plan proporciona un enfoque estructurado y por fases para implementar una suite de tests unitarios que alcance el 80% de cobertura de código. La estrategia se centra en:

1. **Priorización inteligente**: Comenzar con módulos base y puros, luego avanzar hacia sistemas complejos
2. **Herramientas apropiadas**: PlatformIO + Unity para compatibilidad multiplataforma
3. **Automatización completa**: CI/CD, reportes de cobertura, validaciones automáticas
4. **Mantenibilidad**: Convenciones claras, documentación, y prácticas de TDD

**Próximos pasos inmediatos**:
1. Configurar el entorno de testing native
2. Implementar tests de la Fase 1 (MathUtil, Rect, Color)
3. Establecer el pipeline de CI/CD
4. Comenzar el seguimiento de métricas de cobertura

---

*Documento generado para PixelRoot32 Game Engine*  
*Fecha: 2026-02-08*  
*Versión del Plan: 1.0*
