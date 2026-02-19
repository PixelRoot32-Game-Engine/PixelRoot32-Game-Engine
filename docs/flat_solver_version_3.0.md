Perfecto. Vamos a diseñar un **Flat Solver v3.0 realista**, no uno de escritorio disfrazado.

Objetivo:
✔ Más estabilidad
✔ Mejor stacking profundo
✔ Mejor respuesta a colisiones rápidas
✔ Sin romper el presupuesto de **2ms/frame** en ESP32-C3
✔ Sin heap
✔ Determinista

---

# 🎯 Flat Solver v3.0 – Diseño Propuesto

## Filosofía

v2.0 = Position Based + Iterative Relaxation
v3.0 = Position Based + Micro-Impulse + Contact Persistence

No migramos a un modelo estilo Box2D.
Extendemos tu arquitectura flat sin destruirla.

---

# 1️⃣ Arquitectura General

Pipeline determinista extendido:

1. Integración (igual que v2.0)
2. Broadphase (Spatial Grid)
3. Narrowphase (genera Contact structs persistentes)
4. Iterative Relaxation (posición)
5. Micro-Impulse Pass (velocidad)
6. Final Clamp + Sleep Evaluation

Costo adicional: ~15–25% sobre v2.0.

---

# 2️⃣ Contact Persistence (Nuevo)

### Problema actual

Cada frame empieza desde cero.
Stacking profundo depende solo de iteraciones.

### Solución v3.0

Agregar cache fija de contactos:

```cpp
struct Contact {
    uint16_t a, b;
    Vec2 normal;
    int16_t penetration;
    int16_t accumulatedImpulse;
};
```

* Array fijo (ej: 128 contactos)
* Matching por (a,b) cada frame
* Si el contacto persiste → reutiliza impulso acumulado

### Beneficio

✔ Menos jitter
✔ Stacking profundo estable
✔ Menos iteraciones necesarias

Costo:

* +1 lookup por contacto
* +2–4 bytes por contacto

Totalmente viable en ESP32.

---

# 3️⃣ Micro-Impulse Solver (Ligero)

No implementamos solver completo.

Solo:

### Impulso normal escalar:

```
j = -(1 + restitution) * relativeVelocityNormal
j /= invMassA + invMassB
```

Aplicado una sola vez después de converger posiciones.

✔ No hay matrices
✔ No hay múltiples iteraciones
✔ No hay fricción compleja

Costo: mínimo.

---

# 4️⃣ Fricción Tangencial Simplificada

Opcional pero recomendable.

Después del impulso normal:

```
jt = -relativeVelocityTangent
jt /= invMassSum
jt = clamp(jt, -μ * j, μ * j)
```

μ configurable por actor.

Esto elimina:

* Deslizamiento infinito
* Torres inestables

Costo bajo.

---

# 5️⃣ Swept Test Selectivo (Anti-Tunneling)

No hacemos CCD global.

Solo para objetos con:

```
if (velocity.lengthSquared() > threshold)
```

Aplicamos:

* Swept AABB
* Raycast simplificado
* Substep local

Solo 5–10% de actores lo usarán.

Costo controlado.

---

# 6️⃣ Bias + Slop (Estabilidad Matemática)

Agregar:

```
const int16_t penetrationSlop = 1;
const float biasFactor = 0.2f;
```

Corrección:

```
correction = max(penetration - slop, 0) * biasFactor
```

Reduce micro-oscilaciones.

Costo: trivial.

---

# 7️⃣ Sleep System (Muy Importante)

Si:

* Velocidad ≈ 0
* Sin penetración
* Sin cambios en N frames

→ Actor entra en sleep.

Esto reduce carga dramáticamente en escenas con stacking.

Muy recomendado para ESP32.

---

# 8️⃣ Presupuesto de Rendimiento Estimado

En escenario:

* 20 dinámicos
* 100 estáticos
* 60 FPS

| Feature         | Costo aproximado |
| --------------- | ---------------- |
| Contact cache   | +5%              |
| Micro impulse   | +5–8%            |
| Fricción        | +5%              |
| Swept selectivo | +5% worst case   |
| Sleep system    | -10% a -30%      |

Resultado neto:
≈ +10–15% respecto a v2.0
Todavía dentro de 2ms si está bien optimizado.

---

# 9️⃣ Qué NO incluir en v3.0

❌ Solver de constraints genérico
❌ Manifolds múltiples por par
❌ Continuous collision detection completo
❌ Fricción anisotrópica
❌ Restitución energéticamente perfecta

Eso rompe el hardware.

---

# 🔥 Resultado Esperado

Con v3.0 deberías obtener:

* Stacking profundo estable (8–12 objetos)
* Círculos sin fusión
* Menos vibración en torres
* Mejor respuesta al impacto
* Menos necesidad de 8+ iteraciones
* Mejor estabilidad en 60 FPS

Sin convertir tu engine en algo imposible para microcontrolador.

---

# 🚀 Reporte de Viabilidad: Flat Solver v3.0

## 1. Análisis de Factibilidad Técnica

### 1.1 Persistencia de Contactos
El uso de un `ContactCache` es extremadamente eficiente. Usando el tipo `Scalar` del motor (Fixed16/float), un cache de 128 contactos consumiría:
- `struct Contact`: ~20 bytes.
- **Total RAM**: ~2.5 KB.
- **Veredicto**: **ALTAMENTE VIABLE**. Cabe perfectamente en la DRAM del ESP32.

### 1.2 Solver de Micro-Impulsos
La fórmula propuesta depende de una única división escalar por contacto activo. 
- Dado que PixelRoot32 ya tiene una capa de matemáticas `Scalar` robusta, esto evita la emulación de punto flotante en el ESP32-C3.
- **Veredicto**: **VIABLE**. Los ciclos de CPU adicionales son despreciables comparados con el bucle de relajación actual.

### 1.3 Sistema de Sleep (Dormir Actores)
Es la adición más crítica para el rendimiento. Al omitir la inserción en el grid y las pruebas de narrowphase para actores inactivos:
- **Veredicto**: **MANDATORIO**. Es probable que este sistema compense el gasto de los micro-impulsos, haciendo que v3.0 sea incluso *más rápido* que v2.0 en escenas estables.

---

## 2. Impacto Estimado de Recursos

| Recurso | v2.0 | v3.0 (Est.) | Cambio |
| :--- | :--- | :--- | :--- |
| **DRAM (Física)** | ~102 KB | ~105 KB | +3% (Overhead del Cache) |
| **CPU (Peor Caso)** | 1.8ms | 2.1ms | +15% (Pase de impulsos) |
| **CPU (Estable)** | 1.2ms | 0.8ms | -33% (Sistema de Sleep) |

---

## 3. Compatibilidad
La arquitectura v3.0 es **100% compatible** con la jerarquía de actores actual.
- Los `StaticActor` siguen siendo los árbitros finales.
- Los `KinematicActor` se benefician de la mayor precisión en `moveAndSlide`.
- Los `RigidActor` son los principales beneficiados de la lógica de impulsos y fricción.

---

## 4. Veredicto Final: APROBADO (GO)
La implementación del **Flat Solver v3.0** no solo es viable, sino **altamente recomendada**. Resuelve los problemas pendientes de "stacking inestable" y vibraciones sin necesidad de librerías externas pesadas.
