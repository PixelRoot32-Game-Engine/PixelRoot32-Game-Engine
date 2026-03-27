# Description

Implementar un sistema de generación de números pseudoaleatorios (PRNG) dentro de `MathUtil` que sea:

* Determinístico (controlado por seed)
* Independiente de plataforma
* Eficiente en ESP32 (sin uso de librerías pesadas)
* Compatible con el sistema de tipos `Scalar` (`float` y `Fixed16`)

El sistema estará basado en un algoritmo ligero tipo Xorshift32 y proveerá funciones utilitarias para generación de valores aleatorios en distintos rangos y formatos.

---

# Requirements

### R1: PRNG determinístico basado en seed

El sistema debe generar secuencias reproducibles a partir de un seed inicial.

**Validation**

* Dado un mismo seed, la secuencia de valores generados debe ser idéntica entre ejecuciones
* Cambiar el seed debe producir una secuencia diferente

---

### R2: Implementación eficiente para ESP32

El PRNG debe evitar operaciones costosas y dependencias externas.

**Validation**

* No debe usar `std::rand`, `std::mt19937` u otras librerías estándar pesadas
* Debe estar implementado usando operaciones bitwise (`xor`, shifts)
* Debe compilar sin dependencias adicionales en entorno ESP32

---

### R3: Integración con tipo Scalar

Las funciones aleatorias deben ser compatibles con la abstracción `Scalar`.

**Validation**

* `rand01()` debe retornar un valor en rango [0,1] como `Scalar`
* Debe funcionar correctamente tanto con `float` como con `Fixed16`
* No debe romper la consistencia del tipo en operaciones matemáticas existentes

---

### R4: Generación de valores en rango

El sistema debe permitir generar valores dentro de un rango definido.

**Validation**

* `rand_range(min, max)` debe retornar valores dentro del rango inclusivo
* Debe soportar `Scalar` como entrada
* La distribución debe ser uniforme

---

### R5: Generación de enteros aleatorios

El sistema debe soportar generación de enteros en rango.

**Validation**

* `rand_int(min, max)` debe retornar valores enteros en rango inclusivo
* No debe generar valores fuera del rango especificado

---

### R6: Control de seed global

Debe existir una forma de inicializar o cambiar el seed global del sistema.

**Validation**

* `set_seed(seed)` debe reiniciar la secuencia
* Seed = 0 debe ser manejado correctamente (fallback seguro)

---

### R7: Extensibilidad futura

El diseño debe permitir extender el sistema (ej: RNG por instancia).

**Validation**

* La implementación debe permitir encapsular el estado del PRNG en una estructura (`Random`)
* No debe estar fuertemente acoplado a estado global

---

### R8: Funciones utilitarias adicionales (opcional)

El sistema puede incluir helpers comunes usados en gameplay.

**Validation**

* `rand_chance(p)` debe retornar boolean basado en probabilidad
* `rand_sign()` debe retornar -1 o 1 como `Scalar`

---

# Acceptance Criteria

* Todas las funciones (`rand01`, `rand_range`, `rand_int`, `set_seed`) están implementadas en `MathUtil`
* El PRNG utiliza un algoritmo tipo Xorshift32
* El código compila y ejecuta correctamente en ESP32
* La salida es determinística bajo mismo seed
* El sistema funciona correctamente con `float` y `Fixed16`
* No se utilizan librerías estándar de random
* El impacto en performance es mínimo (sin uso intensivo de float en modo fixed)
* El diseño permite futura expansión a RNG por instancia sin refactor mayor