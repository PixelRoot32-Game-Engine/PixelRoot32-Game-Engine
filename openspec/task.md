# Description

Extender y mejorar el sistema PRNG actualmente implementado en `MathUtil` para aumentar su calidad estadística, eficiencia en plataformas sin FPU (ESP32), y mantenibilidad interna, sin romper la API existente.

La mejora se enfoca en:

* Eliminar sesgos en generación de enteros (`rand_int`)
* Optimizar `rand01` para evitar uso de `float` en modo `Fixed16`
* Unificar la lógica del PRNG para evitar duplicación entre RNG global e instanciado
* Definir claramente el comportamiento del estado global (limitaciones y uso recomendado)

La API pública actual debe mantenerse compatible.

---

# Requirements

### R1: Corrección de sesgo en `rand_int`

La implementación actual basada en módulo debe ser reemplazada por una solución sin sesgo.

**Validation**

* No se debe usar directamente `r % range` sin corrección
* Debe implementarse rejection sampling
* La distribución debe ser uniforme para cualquier rango

---

### R2: Optimización de `rand01` en modo Fixed16

La generación de valores en rango [0,1] debe evitar operaciones en punto flotante cuando `Scalar` es `Fixed16`.

**Validation**

* En modo `Fixed16`, no debe usarse división float en runtime
* Debe utilizarse conversión basada en representación interna (`fromRaw` o equivalente)
* El resultado debe mantenerse dentro del rango [0,1]

---

### R3: Unificación del core PRNG

La lógica del algoritmo Xorshift32 debe centralizarse para evitar duplicación.

**Validation**

* Debe existir una única función base del algoritmo (ej: `xorshift32(uint32_t&)`)
* RNG global y `Random` deben reutilizar esta función
* No debe existir duplicación de lógica bitwise

---

### R4: Consistencia entre RNG global e instanciado

Ambas variantes deben comportarse de manera equivalente.

**Validation**

* Dado el mismo seed, ambos deben generar la misma secuencia
* Las funciones (`rand01`, `rand_range`, etc.) deben tener el mismo comportamiento
* No debe haber diferencias en distribución

---

### R5: Definición explícita del estado global

El comportamiento del RNG global debe ser claramente definido.

**Validation**

* Debe documentarse como no thread-safe
* Debe indicarse que no es seguro para ISR o concurrencia
* Debe recomendarse uso de `Random` en sistemas críticos

---

### R6: Validación del estado interno del PRNG

El sistema debe evitar estados inválidos o degenerados.

**Validation**

* El estado no debe permanecer en 0
* `set_seed(0)` debe usar fallback válido
* El PRNG debe continuar generando valores correctamente en todo momento

---

### R7: Mantenimiento de compatibilidad de API

Las mejoras no deben romper la API pública existente.

**Validation**

* Las funciones actuales (`rand01`, `rand_range`, `rand_int`, etc.) deben conservar su firma
* El comportamiento externo debe mantenerse consistente
* No se requieren cambios en código cliente existente

---

### R8: Optimización para ESP32 sin FPU

El sistema debe minimizar operaciones costosas en plataformas sin FPU.

**Validation**

* No debe haber uso innecesario de `float` en caminos críticos
* Debe priorizar operaciones enteras y bitwise
* Debe ser eficiente en arquitectura RISC-V

---

### R9: Mejora opcional de seed no determinístico

El sistema puede permitir inicialización con fuente de entropía externa en runtime.

**Validation**

* Debe poder integrarse con fuente como `esp_random()` sin romper determinismo opcional
* Debe ser opcional (no obligatorio en core)

---

# Acceptance Criteria

* `rand_int` utiliza un método sin sesgo (rejection sampling)
* `rand01` no usa operaciones float en modo `Fixed16`
* Existe una única implementación del algoritmo Xorshift32 reutilizada internamente
* RNG global y `Random` comparten lógica y comportamiento
* El estado global está documentado como no thread-safe
* El sistema evita estados inválidos (ej: seed = 0)
* La API pública existente no cambia
* El código sigue compilando y funcionando en ESP32
* El rendimiento mejora o se mantiene respecto a la implementación actual
* El sistema sigue siendo determinístico bajo mismo seed

