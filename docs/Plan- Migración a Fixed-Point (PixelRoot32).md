# 🧮 Plan Estratégico – Backend Numérico Multi-Arquitectura (PixelRoot32)

## 🎯 Objetivo

Implementar un backend numérico adaptable (**Math Policy Layer**) que seleccione automáticamente la representación numérica más eficiente según la arquitectura del hardware, optimizando tanto para variantes con FPU (ESP32, S3) como sin FPU (C2, C3, C6).

### 🌍 Contexto de Hardware

* **ESP32-Classic / ESP32-S3 / ESP32-P4**: Tienen FPU (Unidad de Punto Flotante). `float` es rápido (1 ciclo).
* **ESP32-C2 / ESP32-C3 / ESP32-C6 / ESP32-S2**: **NO tienen FPU**. `float` es emulado por software (lento).

**Meta:** Un solo codebase que compile a `float` nativo donde existe FPU, y a `Fixed16` optimizado donde no.

---

# 🏗 Arquitectura: Math Policy Layer

En lugar de "migrar a Fixed-Point", creamos una capa de abstracción.

### 1. Definición de `Scalar`

El tipo `Scalar` es el ciudadano de primera clase en el motor físico.

```cpp
// math/Scalar.h

#if defined(CONFIG_IDF_TARGET_ESP32) || defined(CONFIG_IDF_TARGET_ESP32S3)
    #define PR32_HAS_FPU 1
#endif

namespace pixelroot32::math {

#if defined(PR32_HAS_FPU)
    using Scalar = float;
#else
    using Scalar = Fixed16;
#endif

}
```

### 2. Implementación Fixed-Point (RISC-V Optimized)

Para las variantes C3/C2/C6, usamos un formato **Q16.16**:

* **Rango:** ±32768
* **Precisión:** ~0.000015
* **Storage:** `int32_t`
* **Math:** `int64_t` para multiplicaciones (costo razonable en RISC-V).

```cpp
struct Fixed16 {
    int32_t raw;
    static constexpr int FRACTION = 16;
    
    // Multiplicación optimizada
    constexpr Fixed16 operator*(const Fixed16& other) const {
        int64_t temp = static_cast<int64_t>(raw) * other.raw;
        return Fixed16(static_cast<int32_t>(temp >> FRACTION), true);
    }
};
```

### 3. Vector2 Adaptable

La clase `Vector2` se define en términos de `Scalar`.

```cpp
struct Vector2 {
    Scalar x, y;
    // Operaciones matemáticas usan Scalar
};
```

---

# 🚀 Estrategia de Optimización

### 🚫 Eliminar `sqrt` (Crítico)

La raíz cuadrada es costosa incluso en FPU, y devastadora en software.
**Regla:** Usar distancias al cuadrado para comparaciones.

**Incorrecto:**

```cpp
if (dist.length() < radius) { ... } // Llama a sqrt()
```

**Correcto:**

```cpp
if (dist.lengthSquared() < radius * radius) { ... } // Solo multiplicaciones
```

### 📊 Impacto Esperado en ESP32-C3

| Métrica | Soft-Float (Actual) | Fixed-Point (Meta) |
| :--- | :--- | :--- |
| **Collision Time** | ~9.0 ms | **< 2.0 ms** |
| **Physics Int** | Alto | **Bajo** |
| **FPS Stress Test** | ~24 FPS | **~40+ FPS** |

---

# 📅 Fases de Implementación

## ✅ Phase 1: Math Library (Completado)

Crear la estructura base de tipos matemáticos.

* [x] `math/Fixed16.h`: Implementación Q16.16.
* [x] `math/Scalar.h`: Lógica de selección de tipo.
* [x] `math/Vector2.h`: Vector basado en `Scalar`.

## 🟡 Phase 2: Physics Migration

Reemplazar `float` por `Scalar` en el núcleo de física.

* [ ] Modificar `PhysicsActor` para usar `Vector2` (del namespace `math`).
* [ ] Actualizar `CollisionSystem` para usar `Scalar`.
* [ ] Reemplazar llamadas directas a `sqrt` por `lengthSquared`.

## 🔵 Phase 3: Entity Integration

Propagar el uso de `Scalar` a la clase base `Entity`.

* [ ] `Entity::x`, `Entity::y` -> `Scalar`.
* [ ] `Rect` -> `Scalar`.
* [ ] Adaptadores para `Renderer` (convertir `Scalar` a `int` para dibujar).

## 🟢 Phase 4: Validation & Benchmarking

Comparar rendimiento en hardware real.

* [ ] Compilar para ESP32-C3 (debería usar Fixed16).
* [ ] Compilar para ESP32-S3 (debería usar float).
* [ ] Verificar FPS y tiempos de colisión.

---

# ⚠️ Reglas de Desarrollo

1. **Nunca usar `float` o `double` explícitamente** en lógica de juego/física. Usar `Scalar`.
2. **Literales:** Usar sufijos o casting explícito si es necesario, o helpers como `toScalar(0.5f)`.
3. **Renderer:** El renderer final (pantalla) trabaja en pixeles (`int`). La conversión de `Scalar` a `int` debe ocurrir lo más tarde posible (en el `draw`).
4. **Audio:** El audio sigue su propio camino (ya optimizado), no se ve afectado por `Scalar`.
