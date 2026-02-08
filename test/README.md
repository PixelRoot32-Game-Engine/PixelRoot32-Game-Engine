# Tests Unitarios - PixelRoot32 Game Engine

Este directorio contiene la suite de tests unitarios para el motor de juegos PixelRoot32.

## Estructura de Directorios

```
test/
├── test_config.h                 # Configuración y utilidades compartidas
├── unit/                         # Tests unitarios organizados por módulo
│   ├── test_math/               # Tests matemáticos
│   ├── test_physics/            # Tests de física (Collision, Actor, etc.)
│   ├── test_core/               # Tests del núcleo (Entity, Scene, etc.)
│   ├── test_graphics/           # Tests de gráficos (Color, Camera, Font)
│   ├── test_input/              # Tests de entrada
│   └── test_audio/              # Tests de audio
├── test_engine_integration/      # Tests de integración del motor
└── test_game_loop/               # Tests de flujo completo (End-to-End)
```

## Ejecución de Tests

### Requisitos

- [PlatformIO](https://platformio.org/) instalado
- (Opcional) lcov para generación de reportes de cobertura

### Comandos

```bash
# Ejecutar todos los tests
pio test -e native_test

# Ejecutar con salida detallada
pio test -e native_test --verbose

# Ejecutar con cobertura de código
python scripts/coverage_check.py

# Generar reporte HTML de cobertura
python scripts/coverage_check.py --report
```

### Scripts de Ayuda

#### Linux/Mac
```bash
# Ejecutar tests
./scripts/run_tests.sh

# Ejecutar con cobertura
./scripts/run_tests.sh --coverage
```

#### Windows
```batch
REM Ejecutar tests
scripts\run_tests.bat

REM Ejecutar con cobertura
scripts\run_tests.bat --coverage
```

## Convenciones de Testing

### Nomenclatura de Tests

Los tests siguen la convención:
```cpp
void test_<modulo>_<funcion>_<escenario>()
```

Ejemplos:
- `test_mathutil_lerp_basic()`
- `test_rect_intersects_overlapping()`
- `test_color_black()`

### Estructura de un Archivo de Test

```cpp
#include <unity.h>
#include "modulo/Header.h"
#include "../test_config.h"

using namespace pixelroot32::modulo;

void setUp(void) {
    // Inicialización antes de cada test
}

void tearDown(void) {
    // Limpieza después de cada test
}

void test_modulo_funcion_escenario(void) {
    // Arrange
    // Act
    // Assert
    TEST_ASSERT_EQUAL(expected, actual);
}

int main(int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(test_modulo_funcion_escenario);
    return UNITY_END();
}
```

## Cobertura de Código

### Objetivos

- **Líneas**: Mínimo 80%
- **Funciones**: Mínimo 90%

### Módulos Cubiertos (Fase 1)

| Módulo | Archivos | Tests | Cobertura |
|--------|----------|-------|-----------|
| math/MathUtil | MathUtil.h | test_mathutil.cpp | 100% |
| core/Rect | Entity.h | test_rect.cpp | 100% |
| physics/CollisionTypes | CollisionTypes.h | test_collision_types.cpp | 100% |
| graphics/Color | Color.h | test_color.cpp | 100% |

## Framework de Testing

Utilizamos [Unity](https://github.com/ThrowTheSwitch/Unity) integrado con PlatformIO.

### Asserts Disponibles

- `TEST_ASSERT_EQUAL(expected, actual)`
- `TEST_ASSERT_EQUAL_FLOAT(expected, actual)`
- `TEST_ASSERT_TRUE(condition)`
- `TEST_ASSERT_FALSE(condition)`
- `TEST_ASSERT_NULL(pointer)`
- `TEST_ASSERT_NOT_NULL(pointer)`

Y muchos más. Ver [documentación de Unity](https://github.com/ThrowTheSwitch/Unity/blob/master/docs/UnityAssertionsReference.md).

## Contribuyendo

Al agregar nuevos tests:

1. Crear archivo en el directorio correspondiente bajo `test/unit/`
2. Seguir la convención de nomenclatura
3. Incluir todos los casos de borde (edge cases)
4. Documentar con comentarios de doxygen si es necesario
5. Verificar que los tests pasan: `pio test -e native_test`
6. Verificar cobertura: `python scripts/coverage_check.py`

## Plan de Testing

Ver [UNIT_TESTING_PLAN.md](../docs/UNIT_TESTING_PLAN.md) para el plan completo de implementación de tests.

### Fases Completadas

- ✅ **Fase 1**: Fundamentos (Math, Rect, CollisionTypes, Color)

### Próximas Fases

- 🔄 **Fase 2**: Sistema de Física (CollisionPrimitives, CollisionSystem)
- ⏳ **Fase 3**: Core del Engine (Entity, Actor, Scene)
- ⏳ **Fase 4**: Sistemas de Gráficos (Camera2D, UI, Fonts)
- ⏳ **Fase 5**: Input y Audio
- ⏳ **Fase 6**: Integración y Cobertura 80%+

## Recursos

- [PlatformIO Unit Testing](https://docs.platformio.org/en/latest/advanced/unit-testing/index.html)
- [Unity Test Framework](https://github.com/ThrowTheSwitch/Unity)
- [Guía de Contribución](../CONTRIBUTING.md)
