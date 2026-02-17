# 🧮 Plan Estratégico – Soporte Fixed-Point para variantes sin FPU

## 📋 Actualización Importante – C++17

> **Este plan fue actualizado para aprovechar C++17.** La migración desde C++11 permite:
> 
> - Reducir ~50% del boilerplate con `if constexpr`
> - Validación de constantes en compile-time
> - Unificación del codebase dual sin macros
> - CTAD para sintaxis más limpia

---

## 🎯 Objetivo

Evaluar e implementar soporte opcional Fixed-Point **solo si existe ganancia real medible** en ESP32-C3/C2/C6, minimizando:

* Complejidad arquitectónica
* Deuda técnica
* Sobrecarga de mantenimiento
* Riesgo en precisión y bugs


### Contexto específico – PixelRoot32 Game Engine

- El engine actual está escrito en C++17 y usa **float** como tipo real en:
  - Física (`PhysicsActor`, primitivas de colisión, utilidades de `math/MathUtil`).
  - Coordenadas lógicas de entidades y UI (`Entity`, `Actor`, layouts).
  - Parte de la API de render (p. ej. escalado `float` en sprites).
- Audio ya dispone de un camino optimizado para ESP32 sin FPU basado en enteros + LUT, independiente de esta migración.
- El sistema de build real del repositorio es **PlatformIO**; los ejemplos con CMake de este documento son conceptuales y deben traducirse a `build_flags` y configuración de PlatformIO/CI.

Por tanto, cualquier migración a fixed-point debe:

- Tratar la **capa de posiciones lógicas** (Entity/Actor/Physics/Collision) como un todo coherente.
- Mantener Renderer y Audio lo más simples posible, limitando cambios a las interfaces necesarias.
- Respetar siempre los datos empíricos de rendimiento en hardware real antes de avanzar fases.

---

# 🟢 PHASE 0 – Profiling Real (ANTES de tocar código)

> 🔴 Esta fase es obligatoria. No se modifica arquitectura todavía.

### Objetivo

Determinar si el float es realmente un bottleneck en C3.

### Acciones

1. Instrumentar timing por subsistema:
   
   * PhysicsActor::integrate
   * CollisionSystem
   * Renderer
   * Audio
   * Frame total

2. Medir en:
   
   * ESP32-S3 (baseline)
   * ESP32-C3 (soft-float real)

3. Obtener métricas:
   
   * % de tiempo que consume Physics
   * ms por frame
   * FPS real

4. Registrar también:
   * Tamaño de binario de la build actual.
   * Configuración exacta de compilación (flags de optimización, frecuencia de CPU).

> Nota práctica: en esta fase se puede instrumentar con contadores simples basados en `micros()`/`millis()` en ESP32 y logs en la simulación nativa, sin introducir aún ningún tipo nuevo.

### 🎯 Decision Gate #1

| Physics % del frame | Decisión                   |
| ------------------- | -------------------------- |
| < 20%               | ❌ No migrar                |
| 20–30%              | ⚠️ Evaluar micro-benchmark |
| > 30%               | ✅ Pasar a Phase 1          |

Si el resultado es `< 20%`, se recomienda:

- No introducir Fixed-Point.
- Adoptar únicamente mejoras de limpieza baratas (p. ej. centralizar helpers numéricos), sin generalizar todo el engine a plantillas.

---

# 🟡 PHASE 1 – Numeric Abstraction Layer (Siempre recomendable)

> Esta fase mejora arquitectura aunque nunca uses fixed.
> **Actualizado para C++17:** Usar `if constexpr` y templates en vez de macros.

### Objetivo

Desacoplar el engine del tipo numérico concreto.

### Implementación con C++17

Crear `Numeric.hpp`:

```cpp
#pragma once
#include <type_traits>

namespace pr32 {

// Selección de tipo basada en flag de compilación
#ifdef PR32_USE_FIXED
    using default_real = FixedPoint<16, int32_t>;
#else
    using default_real = float;
#endif

// Helper constexpr para conversión type-safe
template<typename T>
constexpr auto real(T value) {
    if constexpr (std::is_same_v<default_real, float>) {
        return static_cast<float>(value);
    } else {
        return default_real(value);
    }
}

// Literal user-defined (C++17)
constexpr auto operator""_fp(long double val) {
    return FixedPoint<16, int32_t>(static_cast<float>(val));
}

} // namespace pr32
```

Aplicar templates dual-mode:

```cpp
template<typename Real = pr32::default_real>
class Vector2 {
public:
    Real x, y;

    constexpr Vector2(Real x = Real{}, Real y = Real{}) : x(x), y(y) {}

    constexpr auto length() const {
        if constexpr (std::is_same_v<Real, float>) {
            return std::sqrt(x*x + y*y);
        } else {
            return fast_sqrt(x*x + y*y); // Implementación fixed
        }
    }
};
```

Y reemplazar en:

* Vector2
* Transform
* PhysicsActor
* CollisionSystem

En el contexto del engine actual:

- `Entity` y `Actor` también deberán alinearse con `pr32::default_real` para posiciones y tamaños lógicos, evitando mezclar `float` en actores base y `FixedPoint` solo en derivados.
- Donde Renderer o UI necesiten enteros de píxel, se realizará la conversión explícita desde `default_real` a `int` en puntos bien definidos (p. ej. en la capa de presentación), minimizando conversiones dispersas.

### Reglas estrictas

* ❌ Nada de `float` directo en el engine
* ❌ Nada de literales 0.5f sueltos
* ❌ Nada de macros para selección de tipo (usar `if constexpr`)
* ✔️ Usar `pr32::default_real` o templates `<typename Real>`
* ✔️ Crear helpers: `pr32::real(0.5)` o `0.5_fp`
* ✔️ Preferir CTAD: `Vector2 pos(10, 20);` en vez de `Vector2<float>`

### Beneficios C++17

* **Sin macros:** `if constexpr` selecciona código en compile-time
* **Type safety:** El compilador verifica ambos paths
* **Sin overhead:** Código no usado se elimina en compile-time
* **Testing simple:** Mismo binario puede testear ambos tipos

### Costo estimado

2–4 días (reducido gracias a C++17)

> Importante: no es obligatorio completar esta fase al 100 % del código existente antes de seguir explorando rendimiento; puede aplicarse primero a física/colisión, dejando otros subsistemas (como UI) para iteraciones posteriores si es necesario.

---

# 🟠 PHASE 2 – Micro-benchmark Sintético

> No se migra el engine completo todavía.
> **C++17 permite:** Unificar benchmark con templates.

Crear **test independiente** (ejecutable o test de rendimiento fuera del loop principal) que simule:

* 10,000 integraciones físicas
* 10,000 operaciones de colisión
* 1 frame simulado

Implementación C++17:

```cpp
template<typename Real>
class PhysicsBenchmark {
public:
    void run() {
        std::vector<PhysicsActor<Real>> actors(10000);
        // ... benchmark code
    }
};

// Comparar ambos en el mismo ejecutable
void run_benchmarks() {
    PhysicsBenchmark<float>().run();
    PhysicsBenchmark<FixedPoint<16>>().run();
}
```

Comparar:

* float (soft-float en C3)
* fixed Q16.16

Medir:

* ciclos
* ms por frame
* tamaño binario
* **C++17 extra:** Static asserts para validar rangos

```cpp
constexpr FixedPoint<16> TEST_VALUE = 100.0f;
static_assert(TEST_VALUE < FixedPoint<16>(1000.0f), 
              "Test value within safe range");
```

---

### 🎯 Decision Gate #2

| Ganancia en C3 | Decisión                    |
| -------------- | --------------------------- |
| < 10%          | ❌ No vale la pena           |
| 10–20%         | ⚠️ Solo Physics             |
| > 20%          | ✅ Implementar Fixed parcial |

Además:

- Si la ganancia está por debajo del 10 %, se recomienda mantener únicamente la capa de abstracción numérica donde aporte claridad, pero sin introducir `FixedPoint` en el código de producción.
- Si la ganancia está en el rango 10–20 %, limitar Fixed-Point a Physics/Collision y evitar extenderlo a Renderer, UI o Audio salvo necesidades muy concretas.

---

# 🔵 PHASE 3 – Fixed SOLO en Physics (Si se justifica)

> No hacer dual-mode total todavía.
> **C++17 permite:** Implementación gradual con `if constexpr`.

Aplicar fixed únicamente en:

* PhysicsActor
* Collision
* Integración
* Velocidades

Renderer puede seguir usando float si no es hot path.

En el estado actual del engine, esto implica:

- Mantener las APIs externas de Renderer y drivers mayoritariamente en enteros y/o floats según corresponda.
- Usar `pr32::default_real` solo en la capa de lógica física y tipos geométricos asociados.

### Implementación C++17

```cpp
template<typename Real = pr32::default_real>
class PhysicsActor {
public:
    Vector2<Real> position;
    Vector2<Real> velocity;
    Real mass;

    void integrate(Real dt) {
        // Código único, selección automática en compile-time
        position += velocity * dt;

        if constexpr (!std::is_same_v<Real, float>) {
            // Validaciones específicas para fixed
            constexpr auto MAX_POS = pr32::real(10000.0);
            if (position.x > MAX_POS) [[unlikely]] {
                handle_overflow();
            }
        }
    }

private:
    void handle_overflow() {
        // Solo existe en builds fixed
        velocity = Vector2<Real>{};
    }
};
```

### Ventajas C++17

* **Un solo archivo:** No duplicar código con #ifdef
* **Validación early:** `static_assert` detecta problemas en compile-time
* **Optional para safety:** `std::optional<Real>` para operaciones riesgosas

```cpp
#include <optional>

template<typename Real>
std::optional<Real> safe_divide(Real a, Real b) {
    if (b == Real{}) return std::nullopt;
    return a / b;
}
```

---

# 🟣 PHASE 4 – Soporte Dual Build (Simplificado con C++17)

Solo si Phase 3 demuestra beneficio real.

Agregar flags:

```cmake
# CMakeLists.txt
option(PR32_USE_FIXED "Use fixed-point arithmetic" OFF)

if(PR32_USE_FIXED)
    target_compile_definitions(engine PRIVATE PR32_USE_FIXED)
endif()
```

**No se necesitan macros de selección** – los templates e `if constexpr` manejan todo.

CI debe compilar ambas variantes:

```bash
cmake -DPR32_USE_FIXED=OFF .. && make
./test_float
cmake -DPR32_USE_FIXED=ON .. && make  
./test_fixed
```

En el repositorio actual (PlatformIO), el mismo concepto se implementará mediante:

- Definición de `PR32_USE_FIXED` en `build_flags` de `platformio.ini` para las plataformas donde se quiera activar Fixed-Point.
- Configuración de la CI para construir y ejecutar tests en al menos dos variantes:
  - Build estándar (float).
  - Build con `PR32_USE_FIXED` activo para ESP32-C3/C2/C6.

---

# ⚠️ Ajustes críticos al plan original

## 1️⃣ C++17 elimina la necesidad de "dual backend completo"

Con `if constexpr` y templates, puedes tener **un solo codebase** que compile a ambos backends:

```cpp
// Un solo archivo, dos binarios
template<typename Real>
class Engine {
    void update() {
        if constexpr (use_fixed<Real>) {
            // Path fixed
        } else {
            // Path float
        }
    }
};
```

Esto reduce drásticamente:

* Duplicación de código
* Riesgo de divergencia
* Complejidad mental

## 2️⃣ Validación en Compile-Time

C++17 permite detectar problemas antes de ejecutar:

```cpp
// Validar que constantes no overflow
constexpr FixedPoint<16> GRAVITY = 9.8f;
static_assert(GRAVITY > 0_fp && GRAVITY < 100_fp, 
              "Gravity in valid range");

// Asegurar que estructuras sean POD cuando es necesario
static_assert(std::is_trivially_copyable_v<Vector2<FixedPoint<16>>>,
              "Vector2 must be trivially copyable for DMA");
```

## 3️⃣ Structured bindings para código más limpio

```cpp
// División con remainder
auto [quotient, remainder] = divmod(position.x, velocity.x);

// Desempaquetar resultados de colisión
auto [collision, normal, penetration] = 
    check_collision(actor_a, actor_b);
```

## 4️⃣ Diferenciar S3 vs C3 (Sin cambios)

El ESP32-C3 sí sufre más por soft-float.

El ESP32-S3 no es comparable en impacto real.

No diseñes el engine pensando en el peor caso si el 80% usará S3.

---

# 🧠 Estrategia Inteligente para PixelRoot32

La mejor jugada con C++17:

> **Un solo codebase con templates + `if constexpr`, activar fixed solo si datos lo justifican.**

### Beneficios C++17 específicos

| Aspecto           | C++11 (Plan Original) | C++17 (Actualizado)          |
| ----------------- | --------------------- | ---------------------------- |
| Selección de tipo | Macros + #ifdef       | `if constexpr`               |
| Código dual       | Duplicado             | Unificado con templates      |
| Validación        | Runtime asserts       | `static_assert` compile-time |
| Sintaxis          | Verbosamente          | CTAD + literals              |
| Mantenimiento     | Alto riesgo           | Bajo riesgo                  |

Eso mantiene:

* Simplicidad (mejorada)
* Performance razonable
* Bajo costo mental (reducido con C++17)
* Credibilidad técnica
* **Menos código duplicado**

Además, aplicado al repositorio actual:

- Physics y Collision se consideran siempre candidatos prioritarios para Fixed-Point.
- Renderer y Audio se mantienen en su diseño actual, salvo adaptaciones puntuales necesarias para integrarse con `pr32::default_real`.
- El motor no se rediseñará pensando en el peor caso de hardware si los datos muestran que la mayoría de usuarios utilizan variantes con FPU (p. ej. ESP32-S3).

---

# 🏁 Conclusión Final

### Haz seguro:

* Phase 0 (Profiling – siempre necesario)
* Phase 1 (Abstracción con templates + `if constexpr`)

En el contexto de PixelRoot32 Game Engine esto se traduce en:

- Medir siempre en hardware real (S3 y C3) antes de cambiar tipos numéricos.
- Introducir `pr32::default_real` primero en la capa de física/colisión y tipos geométricos asociados, manteniendo el resto del código estable mientras se evalúan resultados.

### Haz solo si datos lo exigen:

* Phase 2 (Benchmark unificado con templates)
* Phase 3 (Implementación parcial con `if constexpr`)
* Phase 4 (Dual build con CMake flags)

### No hagas:

* ❌ Migración completa preventiva
* ❌ Duplicar código con macros #ifdef
* ❌ Ignorar `static_assert` para validaciones

### Haz con C++17:

* ✅ Usar `if constexpr` para selección compile-time
* ✅ Aplicar CTAD para sintaxis limpia
* ✅ Validar con `static_assert` en compile-time
* ✅ Usar `std::optional` para operaciones riesgosas
* ✅ Structured bindings para código más legible

---

## 📊 Comparativa: C++11 vs C++17

| Métrica                       | Plan C++11                        | Plan C++17   | Mejora          |
| ----------------------------- | --------------------------------- | ------------ | --------------- |
| Líneas de código dual         | ~200%                             | ~120%        | -40%            |
| Macros necesarias             | 15-20 | 2-3          | -85%            |
| Tiempo implementación Phase 1 | 3-5 días                          | 2-4 días     | -30%            |
| Riesgo de divergencia         | Alto                              | Bajo         | -60%            |
| Validación overflow           | Runtime                           | Compile-time | Early detection |

**Recomendación:** Con C++17, el plan puede ser más agresivo en la implementación dual mientras mantiene bajo el costo de mantenimiento.
