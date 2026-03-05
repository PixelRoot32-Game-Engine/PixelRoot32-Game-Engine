# Análisis Arquitectónico y Roadmap de Refactoring — Sistema de Colisiones PixelRoot32

## Estado Actual del Sistema

### Jerarquía de Clases

```
Entity (Entity.h)
  └── Actor (Actor.h)
        └── PhysicsActor (PhysicsActor.h)
              ├── StaticActor    — STATIC,    sin movimiento
              ├── KinematicActor — KINEMATIC, moveAndCollide / moveAndSlide
              └── RigidActor     — RIGID,     simulación completa (fuerzas, gravedad)
```

### Pipeline Actual

```
update()
  ├── detectCollisions()     — broadphase + narrowphase + generación de contactos
  ├── solveVelocity()        — impulsos iterativos (N iteraciones)
  ├── integratePositions()   — posición += velocidad * dt
  ├── solvePenetration()     — corrección posicional (Baumgarte)
  └── triggerCallbacks()     — onCollision() en ambos cuerpos
```

### Hallazgos Críticos del Análisis

---

## Problema 1 — Inconsistencia en Broadphase

El `SpatialGrid` solo recibe cuerpos dinámicos. Los `STATIC` se excluyen explícitamente en `CollisionSystem::detectCollisions()`:

```cpp
if (pa->getBodyType() == PhysicsBodyType::STATIC) continue;
grid.insert(actor);
```

Esto fuerza dos loops O(N²) adicionales: uno para STATIC (líneas 80–109) y otro para KINEMATIC (líneas 111–124). En un nivel con 200 tiles estáticos y 10 actores dinámicos, el loop STATIC ejecuta 2000 comparaciones por frame cuando el grid podría reducirlo a ~20.

**Impacto**: El rendimiento se degrada linealmente con la cantidad de tiles/plataformas. En ESP32, esto es crítico.

---

## Problema 2 — Orden del Pipeline

El pipeline actual es: **Detect → SolveVelocity → Integrate → SolvePenetration → Callbacks**.

Los callbacks se disparan **después** de la resolución física completa. Esto significa que cuando un collectible (moneda) llama a `onCollision()`, el jugador ya fue "empujado fuera" de la moneda. El objeto ya sufrió resolución de penetración, lo cual es incorrecto para sensores/triggers que no deberían producir respuesta física.

Además, `integratePositions()` solo mueve cuerpos `RIGID`:

```cpp
if (pa->getBodyType() != PhysicsBodyType::RIGID) continue;
pa->position = pa->position + vel * FIXED_DT;
```

El `RigidActor::update()` ya llama a `integrate()` (que aplica fuerzas/gravedad) **antes** del pipeline del `CollisionSystem`. Esto crea una doble integración implícita: gravedad en `RigidActor::update()` + posición en `CollisionSystem::integratePositions()`.

---

## Problema 3 — Ausencia de Sensores/Triggers

No existe concepto de "sensor" o "trigger" en el motor. Toda colisión produce respuesta física. Los collectibles usan `StaticActor`, lo cual genera resolución de penetración no deseada.

Un jugador que toca una moneda recibe un empujón físico antes de que el callback pueda marcarla como recolectada.

---

## Problema 4 — Valor Sentinel en Contact

La generación de contactos usa `penetration = -1` como sentinel para indicar "no hay colisión":

```cpp
contact.penetration = toScalar(-1.0f);  // Sentinel: no collision detected
// ...
if (contact.penetration >= -kEpsilon) {
    contacts.push_back(contact);
}
```

Este patrón es frágil. Si una función de narrowphase no modifica `penetration` por un bug, el contacto se descarta silenciosamente. Si un valor numérico cercano a cero se redondea incorrectamente en `Fixed16`, podría generar falsos positivos/negativos.

---

## Problema 5 — Inconsistencia Numérica en Rect

`Rect` mezcla `Vector2` (Scalar = float/Fixed16) para posición con `int` para dimensiones:

```cpp
struct Rect {
    Vector2 position;
    int width, height;
};
```

Esto genera conversiones implícitas `int → Scalar` en toda la lógica de colisión. En `generateAABBVsAABBContact`, se convierte constantemente con `toScalar(rectA.width / 2.0f)` — una operación que con `Fixed16` involucra multiplicación float intermedia.

---

## Problema 6 — Detección de Pares Duplicados

La deduplicación usa comparación de punteros:

```cpp
if (actorA >= actorB) continue;
```

Esto funciona dentro de un mismo frame pero depende de la estabilidad de direcciones de memoria. Si el allocator reordena objetos (arena allocator, pool) o se implementa hot-reloading, el comportamiento es indefinido. Además, `queryId` en `SpatialGrid` usa un `static int` global, no thread-safe y vulnerable si se instancian múltiples `CollisionSystem`.

---

## Problema 7 — Preparación para Tile Attributes

El sistema actual maneja tiles interactivos creando un `StaticActor` por tile con `userData` empaquetando coordenadas. No existe mecanismo para que el tipo de tile influya en el comportamiento de la colisión (sensor, one-way, damage). Todo tile colisionable produce la misma respuesta física.

---

# ROADMAP DE IMPLEMENTACIÓN POR FASES

---

## Fase 1 — Correcciones de Bajo Riesgo ✅ COMPLETADA

### Objetivos

- Eliminar el valor sentinel en generación de contactos
- Estandarizar las conversiones numéricas en `Rect`
- Mejorar la robustez del `queryId` en `SpatialGrid`
- Preparar infraestructura de IDs para pares

### Cambios Arquitectónicos

**1.1 — Narrowphase con retorno booleano**

Cambiar las funciones `generateCircleVsCircleContact`, `generateAABBVsAABBContact`, y `generateCircleVsAABBContact` para que retornen `bool` en lugar de mutar un `Contact` pre-inicializado con sentinel:

```cpp
// Antes:
void generateCircleVsCircleContact(Contact& contact);
// contact.penetration = -1 como sentinel

// Después:
bool generateCircleVsCircleContact(Contact& contact);
// retorna true si hay colisión, false si no. No se necesita sentinel.
```

`generateContact` cambia a:

```cpp
void generateContact(PhysicsActor* a, PhysicsActor* b) {
    Contact contact;
    contact.bodyA = a;
    contact.bodyB = b;
    // ...
    bool hit = false;
    if (shapeA == CIRCLE && shapeB == CIRCLE) {
        hit = generateCircleVsCircleContact(contact);
    } // ...
    if (hit) contacts.push_back(contact);
}
```

**1.2 — Añadir `entityId` a Actor**

Añadir un campo `uint16_t entityId` a `Actor`, asignado auto-incrementalmente por el `CollisionSystem` al hacer `addEntity`. Esto prepara la deduplicación de pares basada en IDs y es necesario para fases posteriores.

**1.3 — Pre-computar dimensiones Scalar en Rect (opcional)**

Añadir un método helper a `Rect` o crear un `ScalarRect` interno al collision system que almacene todas las dimensiones como `Scalar`. Esto elimina conversiones repetidas sin romper la API pública de `Rect`.

**1.4 — queryId como miembro de instancia de SpatialGrid**

Mover `s_queryId` de variable estática local a miembro de instancia de `SpatialGrid`, permitiendo múltiples instancias independientes.

### Archivos/Componentes Afectados

| Archivo | Cambio |
|---------|--------|
| `CollisionSystem.h` | Firmas de `generate*Contact` → `bool` |
| `CollisionSystem.cpp` | Eliminar sentinel, usar retorno booleano |
| `Actor.h` | Añadir `uint16_t entityId = 0` |
| `CollisionSystem.cpp` | Asignar IDs en `addEntity` |
| `SpatialGrid.h/cpp` | `queryId` como miembro de instancia |
| `Entity.h` | (Sin cambios en Rect público, helper interno) |

### Complejidad Estimada

**Baja** — Cambios mecánicos, sin restructuración.

### Riesgos

- La firma de `generate*Contact` cambia pero son funciones `private`. Riesgo cero de romper API pública.
- El `entityId` debe resetearse si un actor se remueve y re-agrega; definir política clara.

### Estrategia de Testing

- Tests unitarios existentes de `test_collision_system` deben seguir pasando sin cambios.
- Agregar test que verifica que `generateContact` con shapes que no se solapan **no** añade contact.
- Agregar test que verifica asignación secuencial de `entityId`.
- Verificar compilación en `native_test` y `esp32dev`.

### Checklist de implementación (Fase 1)

- [x] **1.1** — Narrowphase con retorno booleano: `generateContact`, `generateCircleVsCircleContact`, `generateAABBVsAABBContact`, `generateCircleVsAABBContact` retornan `bool`; eliminado sentinel `penetration = -1`.
- [x] **1.2** — Añadir `entityId` a `Actor` (`uint16_t entityId = 0`); asignación en `CollisionSystem::addEntity` con `nextEntityId`; wrap a 1 cuando overflow.
- [ ] **1.3** — Pre-computar dimensiones Scalar en Rect (opcional; pospuesto para Fase 6).
- [x] **1.4** — `queryId` como miembro de instancia de `SpatialGrid`; eliminada variable estática `s_queryId` en `getPotentialColliders`.
- [x] **Tests** — Suite `test_collision_system`, `test_collision_primitives`, `test_collision_types` ejecutada en `native`; todos los tests pasan.

---

## Fase 2 — Refactor de Broadphase ✅ COMPLETADA

### Objetivos

- Insertar TODOS los cuerpos (STATIC, KINEMATIC, RIGID) en el `SpatialGrid`
- Eliminar los loops O(N²) para STATIC y KINEMATIC en `detectCollisions()`
- Unificar el filtrado de pares post-broadphase
- Mantener la optimización de que STATIC no se inserta cada frame (solo cuando cambia)

### Cambios Arquitectónicos

**2.1 — Inserción universal en el grid**

Modificar `detectCollisions()` para insertar todos los `Actor` que sean `PhysicsBody`, sin importar su tipo:

```cpp
for (auto e : entities) {
    if (e->type != EntityType::ACTOR) continue;
    Actor* actor = static_cast<Actor*>(e);
    grid.insert(actor);
}
```

**2.2 — Capa de filtrado post-broadphase**

Después de obtener pares candidatos del grid, aplicar filtrado unificado:

```cpp
bool shouldGenerateContact(PhysicsActor* a, PhysicsActor* b) {
    // STATIC vs STATIC → nunca
    if (a->isStatic() && b->isStatic()) return false;
    // KINEMATIC vs KINEMATIC → nunca (cada uno resuelve por su cuenta)
    if (a->isKinematic() && b->isKinematic()) return false;
    // Layer/mask check
    if (!(a->mask & b->layer) && !(b->mask & a->layer)) return false;
    return true;
}
```

**2.3 — Deduplicación por entityId**

Reemplazar `actorA >= actorB` por `actorA->entityId < actorB->entityId`. Esto es determinístico independientemente del layout de memoria:

```cpp
if (actorA->entityId >= actorB->entityId) continue;
```

**2.4 — Dirty flag para STATIC**

Dado que los cuerpos STATIC no se mueven, se pueden insertar una sola vez y re-insertar solo cuando su posición cambie. Añadir un flag `gridDirty` a `PhysicsActor` o mantener un registro separado de estáticos en el grid.

Alternativa más simple para Fase 2: re-insertar todo cada frame (ya que `grid.clear()` + insert es O(N) y el grid es un array estático). Optimizar dirty-tracking en Fase 6.

### Archivos/Componentes Afectados

| Archivo | Cambio |
|---------|--------|
| `CollisionSystem.cpp` | `detectCollisions()` — reescritura mayor |
| `SpatialGrid.h/cpp` | Posible aumento de `kMaxEntitiesPerCell` |
| `EngineConfig.h` | Ajustar `SPATIAL_GRID_MAX_ENTITIES_PER_CELL` default |
| `Actor.h` | Usar `entityId` para deduplicación |

### Complejidad Estimada

**Media** — El cambio principal está concentrado en `detectCollisions()` (~90 líneas), pero requiere validación cuidadosa del filtrado.

### Riesgos

- **Aumento de entidades por celda**: Con STATIC en el grid, cada celda podría tener más entidades. El default de 24 podría necesitar subir a 32–48 para niveles con muchos tiles.
- **Memoria estática**: `cells[kMaxCells][kMaxEntitiesPerCell]` es un array estático global. Si `kMaxEntitiesPerCell` sube de 24 a 32, con un grid de ~64 celdas: 64 × 32 × 4 bytes = 8KB. En ESP32 esto es aceptable pero debe monitorearse.
- **CCD**: El path de CCD para circle vs static AABB debe re-testearse ya que el loop cambia.

### Estrategia de Testing

- Test de regresión: todos los tests existentes de `test_collision_system` deben pasar sin cambios.
- **Nuevo test**: Verificar que STATIC vs STATIC nunca genera contacto.
- **Nuevo test**: Verificar que RIGID vs STATIC detecta colisión vía grid (no loop separado).
- **Test de performance**: Comparar tiempo de `detectCollisions()` con N=100 estáticos antes y después.
- **Test en hardware**: Medir frame time en ESP32 con un nivel real.

### Checklist de implementación (Fase 2)

- [x] **2.1** — Inserción universal en el grid: todos los `Actor` con `isPhysicsBody()` se insertan en el grid (incl. STATIC).
- [x] **2.2** — Filtrado post-broadphase unificado: STATIC vs STATIC y KINEMATIC vs KINEMATIC se descartan; layer/mask aplicado; CCD solo para RIGID circle vs STATIC AABB.
- [x] **2.3** — Deduplicación por `entityId`: reemplazado `actorA >= actorB` por `actorA->entityId >= actorB->entityId` (continue para procesar cada par una sola vez).
- [ ] **2.4** — Dirty flag para STATIC (pospuesto para Fase 6; se re-inserta todo cada frame).
- [x] **Tests** — Suite `test_collision_system` (incl. `test_collision_system_swept_circle_vs_aabb_ccd`) ejecutada en `native`; todos los tests pasan.

---

## Fase 3 — Sistema de Sensores / Trigger Colliders ✅ COMPLETADA

### Objetivos

- Introducir el concepto de `isSensor` en el sistema de colisiones
- Los sensores generan contactos y callbacks pero NO producen respuesta física
- Soportar casos de uso: collectibles, checkpoints, damage zones, triggers de tile
- Mantener backward compatibility total: comportamiento default = sólido

### Cambios Arquitectónicos

**3.1 — Flag `isSensor` en PhysicsActor**

```cpp
// PhysicsActor.h
protected:
    bool sensor = false;

public:
    void setSensor(bool s) { sensor = s; }
    bool isSensor() const { return sensor; }
```

**3.2 — Flag `isSensor` en Contact**

```cpp
// CollisionSystem.h
struct Contact {
    PhysicsActor* bodyA = nullptr;
    PhysicsActor* bodyB = nullptr;
    Vector2 normal;
    Vector2 contactPoint;
    Scalar penetration = toScalar(0);
    Scalar restitution = toScalar(0);
    bool isSensorContact = false;  // <<< NUEVO
};
```

**3.3 — Generación de contactos sensores**

En `generateContact`, si cualquiera de los dos cuerpos es sensor, se marca el contacto como `isSensorContact = true`:

```cpp
void generateContact(PhysicsActor* a, PhysicsActor* b) {
    // ...narrowphase...
    if (hit) {
        contact.isSensorContact = a->isSensor() || b->isSensor();
        contacts.push_back(contact);
    }
}
```

**3.4 — Excluir sensores de resolución física**

En `solveVelocity()` y `solvePenetration()`, skip contactos sensor:

```cpp
void CollisionSystem::solveVelocity() {
    for (auto& contact : contacts) {
        if (contact.isSensorContact) continue;
        // ...resolución normal...
    }
}
```

**3.5 — Callbacks diferenciados**

En `triggerCallbacks()`, usar `onCollision` existente. El juego consulta `isSensor()` del otro cuerpo para decidir comportamiento. En Fase 4 se puede añadir `onSensorEnter/onSensorExit` como métodos virtuales opcionales.

**3.6 — SensorActor (subclase opcional)**

Para conveniencia del usuario, crear un `SensorActor` análogo a `StaticActor`:

```cpp
class SensorActor : public StaticActor {
public:
    SensorActor(Scalar x, Scalar y, int w, int h)
        : StaticActor(x, y, w, h) {
        setSensor(true);
    }
};
```

### Archivos/Componentes Afectados

| Archivo | Cambio |
|---------|--------|
| `PhysicsActor.h` | Añadir `sensor` flag + getter/setter |
| `CollisionSystem.h` | Añadir `isSensorContact` a `Contact` |
| `CollisionSystem.cpp` | Skip sensores en `solveVelocity`, `solvePenetration` |
| `SensorActor.h` (NUEVO) | Subclase de conveniencia |
| `SensorActor.cpp` (NUEVO) | Constructor mínimo |

### Complejidad Estimada

**Baja-Media** — Los cambios son aditivos. Un `bool` + una condición `if` en dos funciones.

### Riesgos

- **Tamaño de PhysicsActor**: +1 byte (bool). Con padding, probablemente +4 bytes. En ESP32 con 100 actores = 400 bytes adicionales. Aceptable.
- **Backward compatibility**: El default `sensor = false` significa que todo el código existente se comporta idéntico.
- **Edge case**: Un sensor KINEMATIC que usa `moveAndCollide` necesita que el broadphase lo detecte pero no lo resuelva. Verificar que `moveAndCollide` (que usa `checkCollision`, no el pipeline principal) no sea afectado.

### Estrategia de Testing

- **Test unitario**: Crear dos actores, uno sensor. Verificar que `solveVelocity` no modifica sus velocidades.
- **Test unitario**: Verificar que el callback `onCollision` SÍ se dispara para contactos sensor.
- **Test unitario**: `SensorActor` se crea con `isSensor() == true`.
- **Test de integración**: Simular recolección de moneda. Sensor coin + rigid player → callback fired, player no empujado.
- **Test de regresión**: Todos los tests existentes pasan (sensor=false por default).

### Checklist de implementación (Fase 3)

- [x] **3.1** — Flag `sensor` en `PhysicsActor` (protected, default false); `setSensor(bool)`, `isSensor()`.
- [x] **3.2** — Flag `isSensorContact` en struct `Contact` (CollisionSystem.h).
- [x] **3.3** — En `generateContact`: marcar `contact.isSensorContact = a->isSensor() || b->isSensor()` antes de push; en path CCD también.
- [x] **3.4** — En `solveVelocity()` y `solvePenetration()`: `if (contact.isSensorContact) continue;` al inicio del bucle.
- [x] **3.5** — Callbacks: se mantiene `onCollision`; el juego puede consultar `other->isSensor()` (sin cambios en triggerCallbacks).
- [x] **3.6** — `SensorActor` (include/physics/SensorActor.h, src/physics/SensorActor.cpp) hereda de StaticActor y llama `setSensor(true)` en constructores.
- [x] **Tests** — Suite `test_collision_system`, `test_collision_primitives`, `test_collision_types`, `test_physics_actor` ejecutada en `native`; todos pasan.

---

## Fase 4 — Mejoras al Pipeline de Colisión

### Objetivos

- Optimizar el orden del pipeline para callbacks fiables
- Separar eventos de enter/exit para sensores
- Resolver la doble integración de RigidActor
- Mejorar la estabilidad numérica

### Cambios Arquitectónicos

**4.1 — Reordenar el pipeline**

Pipeline propuesto:

```
update()
  ├── integrateForces()          — RigidActor: gravedad, fuerzas → velocidad (SIN mover posición)
  ├── detectCollisions()         — broadphase + narrowphase
  ├── separateContacts()         — separar sensores de sólidos
  ├── solveVelocity()            — impulsos iterativos (solo contactos sólidos)
  ├── integratePositions()       — posición += velocidad * dt (solo RIGID)
  ├── solvePenetration()         — corrección posicional (solo contactos sólidos)
  └── triggerCallbacks()         — onCollision + onSensorEnter/Exit
```

El cambio clave es mover la integración de fuerzas (gravedad) al inicio del pipeline y fuera de `RigidActor::update()`. Esto elimina la doble integración y centraliza la lógica en `CollisionSystem`.

**4.2 — Tracking de estado sensor (enter/stay/exit)**

Para detectar `onSensorEnter` y `onSensorExit`, el sistema necesita recordar qué pares sensor estaban activos el frame anterior:

```cpp
// En CollisionSystem:
struct SensorPair {
    uint16_t idA;
    uint16_t idB;
};
static SensorPair previousSensorPairs[kMaxSensorPairs];
static int previousSensorCount = 0;
```

Comparar con el frame actual para generar enter/exit. Dado que los IDs de entidad ya existen (Fase 1), esto es un diff de dos arrays ordenados.

**4.3 — Mover integración de fuerzas al CollisionSystem**

Actualmente `RigidActor::update()` llama `integrate(FIXED_DT)` que aplica gravedad y actualiza velocidad. Y luego `CollisionSystem::integratePositions()` mueve la posición.

Propuesta: El `CollisionSystem` debe ser el dueño completo de la simulación física. `RigidActor::update()` solo debería acumular fuerzas externas (input del jugador, etc.), NO integrar.

```cpp
// CollisionSystem::update()
void update() {
    integrateForces();       // Aplica gravedad/fuerzas → velocidad
    detectCollisions();
    solveVelocity();
    integratePositions();    // velocidad → posición
    solvePenetration();
    triggerCallbacks();
}

void integrateForces() {
    for (auto e : entities) {
        // ... solo RIGID ...
        RigidActor* rigid = static_cast<RigidActor*>(pa);
        rigid->integrate(FIXED_DT);  // Solo fuerzas→vel, sin mover posición
    }
}
```

Esto requiere que `RigidActor::integrate()` NO se llame desde `RigidActor::update()`. El `update()` del actor queda para lógica de juego, no física.

**4.4 — Callbacks con información de contacto**

Extender `onCollision` para recibir opcionalmente datos del contacto:

```cpp
// Actor.h
virtual void onCollision(Actor* other) = 0;
// Nuevo, opcional:
virtual void onCollision(Actor* other, const Contact& contact) { onCollision(other); }
virtual void onSensorEnter(Actor* other) { }
virtual void onSensorExit(Actor* other) { }
```

Backward compatible: la versión sin Contact llama a la existente.

### Archivos/Componentes Afectados

| Archivo | Cambio |
|---------|--------|
| `CollisionSystem.h/cpp` | Reordenar pipeline, añadir `integrateForces()`, sensor tracking |
| `RigidActor.h/cpp` | Remover llamada a `integrate()` de `update()` |
| `Actor.h` | Añadir `onSensorEnter/Exit` virtuales (default vacío) |
| `PhysicsActor.h` | Posible ajuste de `onCollision` signature |

### Complejidad Estimada

**Media-Alta** — Cambiar la responsabilidad de integración es un cambio semántico que afecta a todos los juegos que usen `RigidActor`. Requiere migración cuidadosa.

### Riesgos

- **Breaking change**: Si `RigidActor::update()` deja de integrar, juegos existentes que llaman `actor.update()` manualmente dejarán de tener gravedad. **Mitigación**: Añadir un flag `managedByCollisionSystem` que, cuando es false, mantiene el comportamiento legacy.
- **Sensor enter/exit memory**: El array de pares previos consume memoria estática. Con `kMaxSensorPairs = 64`, son 256 bytes. Aceptable en ESP32.
- **Complejidad de testing**: Sensor enter/exit requiere tests multi-frame.

### Estrategia de Testing

- **Test de regresión**: Verificar que `RigidActor` con `managedByCollisionSystem = true` se comporta idéntico al sistema actual.
- **Test de pipeline**: Crear escenario donde un RIGID cae por gravedad, colisiona con STATIC. Verificar que el orden detect→solve→integrate→correct produce posición estable.
- **Test de sensor enter/exit**: Mover un kinematic hacia un sensor, verificar enter en frame N, stay en frame N+1, exit en frame N+2.
- **Test de regresión de frames**: 60 frames simulados, verificar convergencia de posición.

---

## Fase 5 — Integración de Tile Attributes ✅ COMPLETADA

### Objetivos

- Permitir que el tipo de tile (collectible, damage, one-way, trigger, destructible) determine el comportamiento de colisión
- Integrar el sistema de sensores (Fase 3) con tiles interactivos
- Soportar one-way platforms a nivel de tile
- Permitir que tiles desaparezcan tras colisión

### Cambios Arquitectónicos

**5.1 — Enum TileCollisionBehavior**

Definir un enum de atributos de tile que el motor entienda:

```cpp
// TileAttributes.h (NUEVO)
enum class TileCollisionBehavior : uint8_t {
    SOLID       = 0,  // Colisión normal con resolución física
    SENSOR      = 1,  // Trigger sin respuesta física (collectibles, checkpoints)
    ONE_WAY_UP  = 2,  // Solo bloquea desde arriba (plataformas)
    DAMAGE      = 3,  // Sensor que causa daño
    DESTRUCTIBLE = 4  // Sólido que se destruye al ser golpeado
};
```

**5.2 — Encoding de tile metadata en userData**

Extender el encoding actual de `userData` (que empaqueta coordenadas x,y) para incluir el `TileCollisionBehavior`:

```cpp
// 32-bit encoding:
// bits [0-9]:   x coordinate (max 1023 tiles)
// bits [10-19]: y coordinate (max 1023 tiles)
// bits [20-23]: TileCollisionBehavior (16 valores posibles)
// bits [24-31]: reservados (tipo de tile, tile index, etc.)
```

**5.3 — Configuración automática de sensor según atributo**

Cuando `TilemapCollisionBuilder` crea un `StaticActor` para un tile, el atributo del tile determina si se configura como sensor:

```cpp
void createTileCollider(int x, int y, TileCollisionBehavior behavior) {
    StaticActor* actor = ...;
    
    switch (behavior) {
        case SOLID:
            actor->setSensor(false);
            break;
        case SENSOR:
        case DAMAGE:
            actor->setSensor(true);
            break;
        case ONE_WAY_UP:
            actor->setSensor(false);
            actor->setOneWay(true);  // Nuevo flag (ver 5.4)
            break;
        case DESTRUCTIBLE:
            actor->setSensor(false);
            break;
    }
    
    actor->setUserData(reinterpret_cast<void*>(packTileData(x, y, behavior)));
}
```

**5.4 — One-Way Platforms**

Implementar one-way platforms requiere modificar la generación de contactos. Un contacto con un one-way platform solo se acepta si la normal apunta en la dirección "up" y el cuerpo se mueve hacia abajo:

```cpp
// En generateContact, después de narrowphase:
if (hit && isOneWay(b)) {
    if (contact.normal.y >= 0 || bodyA->getVelocity().y < 0) {
        hit = false;  // Descartar contacto
    }
}
```

Añadir flag `oneWay` a `PhysicsActor` (1 bit, se puede empaquetar con `sensor`).

**5.5 — Tile Destruction**

Para tiles destructibles, el callback `onCollision` debe poder marcar un tile como destruido. El sistema ya tiene `setTileActive(x, y, false)` en el tilemap y `setEnabled(false)` en el actor. Solo falta que el callback tenga la información necesaria (coordenadas del tile), que ya está en `userData`.

La lógica de destrucción es responsabilidad del juego, no del motor. El motor provee:

- `getUserData()` → desempaquetar coordenadas
- `setSensor(true)` / `setEnabled(false)` para desactivar el colisionador
- `tilemap.setTileActive(x, y, false)` para el render

### Archivos/Componentes Afectados

| Archivo | Cambio |
|---------|--------|
| `TileAttributes.h` (NUEVO) | Enum `TileCollisionBehavior`, helpers de pack/unpack |
| `PhysicsActor.h` | Añadir flag `oneWay` |
| `CollisionSystem.cpp` | Lógica one-way en `generateContact` |
| `StaticActor.h` | Posible conveniencia `setOneWay()` |
| `SensorActor.h` | (ya existe de Fase 3) |

### Complejidad Estimada

**Media** — Los conceptos están bien definidos por las fases anteriores. La mayor complejidad está en one-way platforms (corner cases con velocidades diagonales).

### Riesgos

- **One-way edge cases**: Un actor que se mueve horizontalmente sobre una one-way platform no debe caer a través. La condición de aceptación del contacto debe ser robusta. Recomendación: usar tanto la velocidad del actor como la posición relativa (pies del actor por encima de la plataforma en el frame anterior).
- **Encoding de userData**: El cambio de 16+16 bits a 10+10+4 bits reduce el rango máximo de coordenadas. Para mapas grandes (>1024 tiles), necesitaría un esquema alternativo.
- **Interacción sensor + one-way**: Una plataforma one-way con behavior SENSOR no tiene sentido. Validar combinaciones.

### Estrategia de Testing

- **Test**: Tile SENSOR — jugador lo toca, callback fired, sin empujón.
- **Test**: Tile SOLID — jugador colisiona normalmente.
- **Test**: Tile ONE_WAY_UP — jugador salta a través desde abajo, aterriza desde arriba.
- **Test**: Tile ONE_WAY_UP — jugador camina sobre ella horizontalmente sin caer.
- **Test**: Tile DESTRUCTIBLE — golpe destruye tile, colisionador se desactiva.
- **Test**: Tile DAMAGE — sensor, callback con información de daño.
- **Test**: Pack/unpack de coordenadas + behavior con valores límite.

### Checklist de implementación (Fase 5)

- [x] **5.1** — `TileAttributes.h` creado con `TileCollisionBehavior` (SOLID, SENSOR, ONE_WAY_UP, DAMAGE, DESTRUCTIBLE).
- [x] **5.2** — Helpers `packTileData` / `unpackTileData` (bits 0–9: x, 10–19: y, 20–23: behavior); `packCoord` / `unpackCoord` legacy (16+16) mantenidos.
- [x] **5.3** — Configuración según behavior: el juego/TilemapCollisionBuilder usa `setSensor()` / `setOneWay()`; el motor solo expone los flags.
- [x] **5.4** — Flag `oneWay` en `PhysicsActor` (`setOneWay(bool)`, `isOneWay()`); en `generateContact` y en path CCD, filtrar contacto one-way (aceptar solo cuando normal apunta "arriba" y el cuerpo móvil se mueve hacia abajo).
- [x] **5.5** — Tile destruction: sin cambios en motor; el juego usa `getUserData()`, `setEnabled(false)`, tilemap, etc.
- [x] **Tests** — `test_collision_system_one_way_platform_land_from_above`, `test_collision_system_one_way_platform_jump_through_from_below`; `test_physics_actor_one_way_*`, `test_tile_attributes_pack_unpack`, `test_tile_attributes_behavior_enum`.

---

## Fase 6 — Performance y Estabilidad ✅ COMPLETADA

### Objetivos

- Optimizar el `SpatialGrid` para minimizar re-inserciones de STATIC
- Estandarizar tipos numéricos en el sistema de colisión
- Profiling y tuning para ESP32
- Consideraciones de estabilidad a largo plazo

### Cambios Arquitectónicos

**6.1 — Dirty-tracking para STATIC en SpatialGrid**

Separar el grid en dos capas: una estática (se construye una vez al cargar el nivel) y una dinámica (se reconstruye cada frame):

```cpp
class SpatialGrid {
    // Grid estático: se llena una vez, se invalida con dirtyStatic flag
    static Actor* staticCells[kMaxCells][kMaxStaticPerCell];
    static int staticCellCounts[kMaxCells];
    bool staticDirty = true;
    
    // Grid dinámico: se llena cada frame
    static Actor* dynamicCells[kMaxCells][kMaxDynamicPerCell];
    static int dynamicCellCounts[kMaxCells];
    
    void rebuildStaticGrid();  // Solo cuando staticDirty
    void clearDynamic();       // Cada frame
    void insertDynamic(Actor*);
    void getPotentialColliders(...); // Merge estático + dinámico
};
```

Esto reduce drásticamente el costo per-frame cuando hay muchos tiles estáticos.

**6.2 — ScalarRect para cálculos internos**

Crear un tipo interno `ScalarRect` donde width/height son `Scalar`:

```cpp
struct ScalarRect {
    Scalar x, y, w, h;
    
    static ScalarRect from(const Rect& r) {
        return {r.position.x, r.position.y, toScalar(r.width), toScalar(r.height)};
    }
};
```

Usar `ScalarRect` en todas las funciones de narrowphase internamente, convirtiendo desde `Rect` una sola vez al inicio del cálculo. Esto elimina las conversiones `toScalar(rect.width / 2.0f)` repetidas.

**Nota**: No cambiar el tipo público de `Rect` — eso rompería demasiado código. `Rect` con `int` width/height es correcto para rendering (pixels enteros). La conversión a `ScalarRect` solo ocurre en el narrowphase.

**6.3 — Pool de contactos con tamaño fijo**

Reemplazar `std::vector<Contact> contacts` por un array estático:

```cpp
static constexpr int kMaxContacts = 128;
Contact contacts[kMaxContacts];
int contactCount = 0;
```

Esto elimina allocaciones dinámicas en el hot path. En ESP32, `std::vector::push_back` puede causar fragmentación de heap.

**6.4 — Profiling hooks**

Añadir instrumentación condicional (`PIXELROOT32_ENABLE_PROFILING`) para medir el tiempo de cada etapa del pipeline:

```cpp
void update() {
    PROFILE_BEGIN(Physics_DetectCollisions);
    detectCollisions();
    PROFILE_END(Physics_DetectCollisions);
    // ...
}
```

**6.5 — Validación de estabilidad numérica con Fixed16**

Crear un test suite que ejecute escenarios de colisión idénticos en `float` y `Fixed16`, comparando resultados. Esto detecta casos donde la precisión de `Fixed16` causa divergencia (por ejemplo, objetos que se atascan o vibran).

### Archivos/Componentes Afectados

| Archivo | Cambio |
|---------|--------|
| `SpatialGrid.h/cpp` | Separación estático/dinámico |
| `CollisionSystem.h` | Contacts como array estático, ScalarRect |
| `CollisionSystem.cpp` | Conversión a ScalarRect en narrowphase |
| `EngineConfig.h` | Nuevos defines para `kMaxContacts`, `kMaxStaticPerCell` |

### Complejidad Estimada

**Media-Alta** — La separación del grid es un cambio significativo. El array estático de contacts requiere handling de overflow. El profiling es mecánico.

### Riesgos

- **Memoria estática dual grid**: Duplicar el grid estático/dinámico podría consumir 16KB+ en ESP32. **Mitigación**: Reducir `kMaxStaticPerCell` dado que los estáticos no cambian — se puede ajustar a 8 si la geometría del nivel es predecible.
- **Contact overflow**: Si hay más de `kMaxContacts` contactos, se pierden colisiones. **Mitigación**: Log de warning en debug, priorizar contactos por penetración.
- **Regresión Fixed16**: Cambiar cálculos internos a `ScalarRect` podría alterar el orden de operaciones y producir resultados ligeramente diferentes en Fixed16.

### Estrategia de Testing

- **Benchmark**: Medir `detectCollisions()` en ESP32 con 200 tiles antes y después del dirty-tracking.
- **Test de overflow**: Crear escenario con >128 contactos, verificar que no crashea.
- **Test Float vs Fixed16**: Ejecutar 10 escenarios en ambos modos, verificar convergencia.
- **Test de estabilidad**: 1000 frames de simulación, verificar que no hay drift de energía.
- **Memory profiling**: Medir RAM total del sistema de colisión en ESP32 con configuración máxima.

### Checklist de implementación (Fase 6)

- [x] **6.1** — SpatialGrid con capas estático/dinámico: `rebuildStaticIfNeeded(entities)`, `clearDynamic()`, `insertDynamic(actor)`, `markStaticDirty()`; `getPotentialColliders` fusiona estático + dinámico. Config `SpatialGridMaxStaticPerCell` y `SpatialGridMaxDynamicPerCell`.
- [x] **6.2** — `ScalarRect` interno en narrowphase (`ScalarRect::from(Rect)`); uso en `generateAABBVsAABBContact` y `generateCircleVsAABBContact`.
- [x] **6.3** — Pool de contactos fijo: `Contact contacts[kMaxContacts]`, `contactCount`; overflow manejado. Config `PHYSICS_MAX_CONTACTS` (128).
- [x] **6.4** — Hooks de profiling: `PIXELROOT32_PROFILE_BEGIN` / `PIXELROOT32_PROFILE_END` en `CollisionSystem::update()` para cada etapa.
- [x] **6.5** — Tests existentes pasan en native.

---

## Resumen del Roadmap

| Fase | Objetivo | Complejidad | Riesgo Breaking | Prioridad |
|------|----------|-------------|-----------------|-----------|
| **1** | ~~Eliminar sentinel, añadir entityId, robustez~~ ✅ | **Baja** | Ninguno | ~~Inmediata~~ **Completada** |
| **2** | ~~Broadphase unificado~~ ✅ | **Media** | Bajo (interno) | ~~Alta~~ **Completada** |
| **3** | ~~Sensores / triggers~~ ✅ | **Baja-Media** | Ninguno (aditivo) | ~~Alta~~ **Completada** |
| **4** | Pipeline + enter/exit + integración centralizada | **Media-Alta** | Medio (requiere migración) | Media |
| **5** | ~~Tile attributes integration~~ ✅ | **Media** | Bajo | ~~Media~~ **Completada** |
| **6** | ~~Performance, memoria, estabilidad~~ ✅ | **Media-Alta** | Bajo | ~~Tras validación~~ **Completada** |

### Dependencias entre Fases

```
Fase 1 ──→ Fase 2 ──→ Fase 3 ──→ Fase 4
                         │            │
                         └────→ Fase 5 ←──┘
                                  │
                              Fase 6
```

- **Fase 1** es prerequisito de Fase 2 (entityId para deduplicación).
- **Fase 2** es prerequisito de Fase 3 (sensores deben estar en el grid).
- **Fase 3** es prerequisito de Fase 5 (tiles sensor necesitan el flag).
- **Fase 4** puede ejecutarse en paralelo con Fase 5 pero debería completarse antes de Fase 6.
- **Fase 6** es la última ya que optimiza el sistema final.

### Estimación Total

Con un developer trabajando exclusivamente en esto:

- **Fase 1**: 1–2 días
- **Fase 2**: 2–3 días
- **Fase 3**: 1–2 días
- **Fase 4**: 3–4 días
- **Fase 5**: 2–3 días
- **Fase 6**: 3–4 días

**Total estimado**: 12–18 días de desarrollo + testing.

---

# ANÁLISIS DE IMPACTO CONTRA v1.0.0 ESTABLE (PRODUCCIÓN)

## Contexto de la Versión en Producción

La **v1.0.0** es la primera release estable del motor, publicada con las siguientes características consolidadas en el sistema de física (**Flat Solver 1.0**):

- Broadphase con grid uniforme (32px cells) y buffers estáticos compartidos
- `KinematicActor` con `moveAndSlide` / `moveAndCollide` (binary search)
- Stacking estable (Baumgarte correction, iteraciones de velocidad)
- API estilo Godot: `KinematicCollision`, tipos `Static`/`Kinematic`/`Rigid`
- Abstracción numérica `Scalar` (float en ESP32-S3 / Fixed16 en C3/S2/C6)

Plataformas en producción:

| Plataforma | Resolución lógica | Scalar | Notas |
|------------|-------------------|--------|-------|
| `esp32c3` (DFRobot Beetle) | 72×40 | Fixed16 (sin FPU) | OLED, U8G2 |
| `esp32dev` | 240×240 | float (FPU) | TFT SPI, TFT_eSPI |
| `native` (SDL2) | 240×240 | float | Desktop testing |

### Superficie de API Pública v1.0.0 (Física)

Los siguientes elementos son parte de la API pública que los juegos consumen directamente:

| Clase/Struct | Métodos públicos consumidos por juegos |
|--------------|---------------------------------------|
| `Actor` | `onCollision()`, `getHitBox()`, `layer`, `mask`, `isPhysicsBody()` |
| `PhysicsActor` | `setVelocity()`, `getVelocity()`, `setMass()`, `setRestitution()`, `setShape()`, `setRadius()`, `setUserData()`, `getUserData()`, `setBodyType()`, `getBodyType()`, `bounce` |
| `StaticActor` | Constructor `(x, y, w, h)` |
| `KinematicActor` | `moveAndCollide()`, `moveAndSlide()`, `is_on_floor()`, `is_on_ceiling()`, `is_on_wall()` |
| `RigidActor` | `applyForce()`, `applyImpulse()`, `integrate()` |
| `CollisionSystem` | `addEntity()`, `removeEntity()`, `update()`, `checkCollision()`, `clear()` |
| `Rect` | `position` (Vector2), `width` (int), `height` (int), `intersects()` |
| `Contact` | (usado internamente, pero expuesto en header público) |
| `KinematicCollision` | `collider`, `normal`, `position`, `travel`, `remainder` |
| Collision Layers | `CollisionLayer` (uint16_t), `DefaultLayers::kNone`, `DefaultLayers::kAll` |

---

## Evaluación de Impacto por Fase

### Fase 1 — Correcciones de Bajo Riesgo

| Criterio | Evaluación |
|----------|------------|
| **Rompe API pública** | **NO** |
| **Rompe ABI** | **NO** (campos nuevos al final) |
| **Afecta comportamiento observable** | **NO** |
| **Requiere cambios en juegos** | **NO** |
| **Riesgo en ESP32C3 (Fixed16)** | **Ninguno** — eliminar sentinel mejora la robustez numérica en Fixed16 |
| **Riesgo en ESP32dev (float)** | **Ninguno** |
| **Impacto en memoria** | +2 bytes por Actor (entityId uint16_t). Con 64 entidades max = 128 bytes |

**Detalle de cambios internos:**

| Cambio | Visibilidad | Impacto en juegos existentes |
|--------|------------|------------------------------|
| `generate*Contact` retorna `bool` | `private` | Cero — función interna |
| Eliminar sentinel `penetration = -1` | Interno | Cero — resultado idéntico |
| Añadir `entityId` a `Actor` | `public` (campo nuevo) | Cero — campo aditivo, default 0 |
| `queryId` como miembro de instancia | Interno a `SpatialGrid` | Cero |

**Veredicto: SAFE para v1.0.x (patch)**

Se puede publicar como **v1.0.1** sin riesgo. No requiere guía de migración.

---

### Fase 2 — Refactor de Broadphase

| Criterio | Evaluación |
|----------|------------|
| **Rompe API pública** | **NO** |
| **Rompe ABI** | **NO** |
| **Afecta comportamiento observable** | **POTENCIALMENTE** — el orden de detección de contactos puede cambiar |
| **Requiere cambios en juegos** | **NO** |
| **Riesgo en ESP32C3 (Fixed16)** | **Bajo** — misma lógica de narrowphase, solo cambia broadphase |
| **Riesgo en ESP32dev (float)** | **Bajo** |
| **Impacto en memoria** | `kMaxEntitiesPerCell` podría subir de 24 a 32. Impacto en ESP32C3: +2KB aprox. |

**Análisis de riesgo detallado:**

1. **Orden de contactos**: Al unificar el broadphase, el orden en que se generan los contactos puede cambiar. Si un juego depende implícitamente del orden de callbacks `onCollision()`, podría observar diferencias. **Probabilidad**: Baja — los juegos no deberían depender del orden de callbacks.

2. **CCD path**: El CCD (`sweptCircleVsAABB`) actualmente tiene un path dedicado dentro del loop de STATIC. Al unificar, el CCD debe funcionar con cualquier par detectado por el grid. **Riesgo**: Bajo si se mantiene la condición `needsCCD()`.

3. **Memoria en ESP32C3**: El grid actual con 72×40 lógico y 32px de celda = (72/32+1)×(40/32+1) = 3×2 = 6 celdas. Con `kMaxEntitiesPerCell = 32`: 6 × 32 × 4 = 768 bytes. **Aceptable**.

4. **Memoria en ESP32dev**: 240×240 con 32px = 8×8 = 64 celdas. Con `kMaxEntitiesPerCell = 32`: 64 × 32 × 4 = 8KB. **Aceptable** en ESP32 con ~320KB DRAM.

5. **Performance**: Se elimina O(N²) por O(N) grid. En el peor caso (200 tiles), se pasa de ~2000 comparaciones a ~20. **Mejora neta en ESP32**.

**Veredicto: SAFE para v1.1.0 (minor)**

Cambio interno que mejora rendimiento. Publicar como **v1.1.0** con nota de que el broadphase fue unificado. No requiere guía de migración.

**Recomendación**: Publicar junto con Fase 1 como un único **v1.1.0**.

---

### Fase 3 — Sistema de Sensores / Trigger Colliders

| Criterio | Evaluación |
|----------|------------|
| **Rompe API pública** | **NO** — todo es aditivo |
| **Rompe ABI** | **SÍ (menor)** — tamaño de `PhysicsActor` y `Contact` cambia |
| **Afecta comportamiento observable** | **NO** — default `sensor = false` preserva comportamiento |
| **Requiere cambios en juegos** | **NO** para existentes. **SÍ** para usar la nueva funcionalidad |
| **Riesgo en ESP32C3** | **Ninguno** |
| **Riesgo en ESP32dev** | **Ninguno** |
| **Impacto en memoria** | +4 bytes por PhysicsActor (bool + padding). +1 byte por Contact |

**Análisis de compatibilidad:**

| Elemento | Antes (v1.0.0) | Después | Compatible |
|----------|----------------|---------|------------|
| `PhysicsActor::sensor` | No existe | `false` por default | ✅ Aditivo |
| `Contact::isSensorContact` | No existe | `false` por default | ✅ Aditivo |
| `SensorActor` | No existe | Nueva clase | ✅ Nuevo archivo |
| `solveVelocity()` | Procesa todo | Skip si `isSensorContact` | ✅ Nunca true en código legacy |
| `solvePenetration()` | Procesa todo | Skip si `isSensorContact` | ✅ Nunca true en código legacy |

**Impacto en juegos existentes que usan `StaticActor` para collectibles:**

Los juegos actuales seguirán funcionando exactamente igual (monedas con `StaticActor` producen empujón físico, como antes). Para obtener el nuevo comportamiento sensor, el juego debe migrar explícitamente:

```cpp
// Antes (v1.0.0) — sigue funcionando
StaticActor* coin = new StaticActor(x, y, 8, 8);

// Después (v1.2.0) — nueva opción
SensorActor* coin = new SensorActor(x, y, 8, 8);
// O alternativamente:
StaticActor* coin = new StaticActor(x, y, 8, 8);
coin->setSensor(true);
```

**Veredicto: SAFE para v1.2.0 (minor)**

Feature aditivo puro. Publicar como **v1.2.0**. Documentar `SensorActor` en changelog y API reference.

---

### Fase 4 — Mejoras al Pipeline de Colisión

| Criterio | Evaluación |
|----------|------------|
| **Rompe API pública** | **POTENCIALMENTE** — si se mueve la integración fuera de `RigidActor::update()` |
| **Rompe ABI** | **SÍ** — nuevos métodos virtuales en Actor |
| **Afecta comportamiento observable** | **SÍ** — el orden de callbacks cambia, integración cambia |
| **Requiere cambios en juegos** | **SÍ** — juegos con `RigidActor` deben adaptarse |
| **Riesgo en ESP32C3** | **Medio** — el reorden puede afectar convergencia en Fixed16 |
| **Riesgo en ESP32dev** | **Bajo** |
| **Impacto en memoria** | +256 bytes para sensor pair tracking |

**Análisis de breaking changes:**

1. **`RigidActor::update()` ya no integra**: Esto es el cambio más disruptivo. Si un juego llama `actor->update(dt)` esperando que aplique gravedad, dejará de funcionar.

   **Mitigación propuesta**: Flag `managedByCollisionSystem`:
   - `true` (nuevo default): El `CollisionSystem` es dueño de la integración. `RigidActor::update()` es no-op para física.
   - `false` (legacy): `RigidActor::update()` sigue llamando `integrate()` como en v1.0.0.

   Con esta mitigación, el juego existente puede setear `managedByCollisionSystem = false` para mantener compatibilidad. Pero esto **debe documentarse**.

2. **Nuevos métodos virtuales** (`onSensorEnter`, `onSensorExit`): Aditivos con default vacío. **No breaking**.

3. **`onCollision` con Contact**: Overload adicional con default que llama al existente. **No breaking**.

4. **Orden de callbacks**: En v1.0.0, callbacks se disparan después de toda la resolución. En el nuevo pipeline, siguen al final pero con sensor enter/exit diferenciado. Juegos que dependan del timing preciso de callbacks podrían notar diferencias sutiles.

**Tabla de impacto por plataforma:**

| Plataforma | Riesgo | Motivo |
|------------|--------|--------|
| ESP32C3 (Fixed16) | **Medio** | El reorden del pipeline puede alterar la convergencia numérica. En Fixed16, el orden de operaciones afecta el resultado por truncamiento. Requiere testing exhaustivo. |
| ESP32dev (float) | **Bajo** | Float es más tolerante a reorden. La diferencia sería sub-pixel. |
| Native (SDL2) | **Bajo** | Misma precisión que ESP32dev. Ideal para testing. |

**Veredicto: REQUIERE v2.0.0 (major) o flag de compatibilidad**

Si se implementa el flag `managedByCollisionSystem` con default `false` (legacy), puede ser **v1.3.0** sin breaking change. Si el default es `true` (nuevo comportamiento), es un **breaking change** que exige **v2.0.0**.

**Recomendación**: Implementar como **v1.3.0** con `managedByCollisionSystem = false` por default. En **v2.0.0** cambiar el default a `true` y deprecar el modo legacy.

---

### Fase 5 — Integración de Tile Attributes

| Criterio | Evaluación |
|----------|------------|
| **Rompe API pública** | **NO** — todo es aditivo |
| **Rompe ABI** | **SÍ (menor)** — nuevo campo `oneWay` en PhysicsActor |
| **Afecta comportamiento observable** | **NO** — default `oneWay = false` |
| **Requiere cambios en juegos** | **NO** para existentes. **SÍ** para usar tile attributes |
| **Riesgo en ESP32C3** | **Bajo** |
| **Riesgo en ESP32dev** | **Bajo** |
| **Impacto en memoria** | +1 byte por PhysicsActor, nuevo header TileAttributes.h |

**Impacto en el encoding de userData existente:**

| Encoding | v1.0.0 | Fase 5 propuesta | Compatible |
|----------|--------|------------------|------------|
| Bits 0-15 | X coordinate | Bits 0-9: X | ⚠️ **Incompatible** |
| Bits 16-31 | Y coordinate | Bits 10-19: Y, 20-23: behavior | ⚠️ **Incompatible** |

**Riesgo**: Los juegos que ya usan `userData` con el encoding 16+16 de `IMPLEMENTATION_PLAN_PhysicsActor_UserData.md` deberán migrar al nuevo encoding 10+10+4.

**Mitigación**: Proporcionar ambos helpers:

```cpp
// Legacy (v1.0.0 compatible)
uintptr_t packCoord(uint16_t x, uint16_t y);
void unpackCoord(uintptr_t packed, uint16_t& x, uint16_t& y);

// Nuevo (v1.4.0+)
uintptr_t packTileData(uint16_t x, uint16_t y, TileCollisionBehavior behavior);
void unpackTileData(uintptr_t packed, uint16_t& x, uint16_t& y, TileCollisionBehavior& behavior);
```

Los juegos que no necesiten tile attributes pueden seguir usando `packCoord`.

**Veredicto: SAFE para v1.4.0 (minor)**

Feature aditivo. El encoding nuevo es opcional. Documentar ambas opciones de packing en la guía de migración.

---

### Fase 6 — Performance y Estabilidad

| Criterio | Evaluación |
|----------|------------|
| **Rompe API pública** | **POTENCIALMENTE** — si se reemplaza `std::vector<Contact>` por array estático |
| **Rompe ABI** | **SÍ** — layout de `CollisionSystem` cambia |
| **Afecta comportamiento observable** | **SÍ** — overflow de contacts puede perder colisiones |
| **Requiere cambios en juegos** | **NO** — cambios internos |
| **Riesgo en ESP32C3** | **Medio** — dual grid consume más memoria estática |
| **Riesgo en ESP32dev** | **Bajo** |
| **Impacto en memoria** | Dual grid: +8KB en ESP32dev, +768 bytes en ESP32C3. Array de contacts: depende de kMaxContacts |

**Análisis de memoria ESP32C3 (crítico):**

El ESP32C3 tiene ~320KB SRAM total, con ~200KB disponible para la aplicación tras RTOS y stack.

| Componente | v1.0.0 | Fase 6 | Delta |
|------------|--------|--------|-------|
| Grid único | 6 × 24 × 4 = 576B | — | — |
| Grid estático | — | 6 × 8 × 4 = 192B | +192B |
| Grid dinámico | — | 6 × 16 × 4 = 384B | +384B |
| Contacts (vector) | ~512B heap | — | — |
| Contacts (array) | — | 128 × 40B = 5KB | +4.5KB |
| Sensor pairs | — | 64 × 4B = 256B | +256B |
| **Total delta** | — | — | **~+5.3KB** |

En ESP32C3 con resolución 72×40, el grid es muy pequeño. El impacto principal es el array estático de contacts (5KB). **Esto es aceptable** pero debe monitorearse contra el presupuesto total de RAM.

Para ESP32dev (240×240), el grid dual pasa de 8KB a ~12KB. El array de contacts suma 5KB. **Total: ~17KB para el sistema de colisión**. Aceptable con 320KB DRAM.

**Veredicto: SAFE para v1.5.0 (minor)**

Cambios internos de optimización. El único riesgo es overflow de contacts que debe manejarse con warning en debug.

---

## Matriz de Impacto Consolidada

| Fase | Versión Sugerida | API Breaking | ABI Breaking | Memoria ESP32C3 | Memoria ESP32dev | Requiere Migración |
|------|------------------|-------------|-------------|-----------------|-----------------|-------------------|
| 1 | **v1.0.1** (patch) | No | No | +128B | +128B | No |
| 2 | **v1.1.0** (minor) | No | No | +192B | +2KB | No |
| 3 | **v1.2.0** (minor) | No | Sí (menor) | +256B | +256B | No (opt-in) |
| 4 | **v1.3.0** (minor)¹ | Potencial | Sí | +256B | +256B | Parcial |
| 5 | **v1.4.0** (minor) | No | Sí (menor) | +64B | +64B | No (opt-in) |
| 6 | **v1.5.0** (minor) | No | Sí | +5.3KB | +9KB | No |
| **Acumulado** | — | — | — | **~+6.2KB** | **~+11.7KB** | — |

¹ Solo si `managedByCollisionSystem` default es `false`. Si default es `true`, sería **v2.0.0**.

---

## Recomendaciones para Proteger la Estabilidad de v1.0.0

### 1. Estrategia de Branching

```
main (v1.0.0 estable)
  │
  ├── hotfix/1.0.x          ← Solo bugfixes críticos
  │
  └── develop
       ├── feat/phase-1      ← Merge rápido (bajo riesgo)
       ├── feat/phase-2      ← Merge tras tests completos
       ├── feat/phase-3      ← Merge tras tests completos
       ├── feat/phase-4      ← Merge con feature flag
       ├── feat/phase-5      ← Merge tras Fase 3
       └── feat/phase-6      ← Merge final
```

### 2. Criterios de Merge por Fase

| Fase | Criterio para merge a develop |
|------|-------------------------------|
| 1 | Tests unitarios pasan en native + esp32dev |
| 2 | Tests unitarios + benchmark de performance en esp32dev. No regresión >5% frame time |
| 3 | Tests unitarios + test de integración sensor. Verificar que tests legacy pasan idénticos |
| 4 | Tests unitarios + tests multi-frame + validación en hardware ESP32C3 (Fixed16). Flag legacy verificado |
| 5 | Tests unitarios + test de one-way platform + test de tile destruction |
| 6 | Benchmark completo en ESP32C3 y ESP32dev. Memory profiling. Test Float vs Fixed16 |

### 3. Tests de Regresión Obligatorios

Antes de cada merge, ejecutar la suite completa existente:

- `test_collision_system` — Pipeline completo
- `test_collision_primitives` — Intersecciones geométricas
- `test_collision_types` — Tipos de colisión
- `test_physics_actor` — Propiedades de PhysicsActor
- `test_kinematic_actor` — moveAndCollide / moveAndSlide
- `test_rect` — Rect::intersects
- `test_tile_collection` — Integración con tiles

### 4. Feature Flags de Protección

Para Fase 4 (la más riesgosa), implementar compilación condicional:

```cpp
// EngineConfig.h
#ifndef PR32_COLLISION_PIPELINE_V2
#define PR32_COLLISION_PIPELINE_V2 0  // 0 = legacy (v1.0.0), 1 = nuevo pipeline
#endif
```

Esto permite que un juego en producción compile con `PR32_COLLISION_PIPELINE_V2=0` y obtenga el comportamiento exacto de v1.0.0, mientras otros juegos experimentan con el nuevo pipeline.

### 5. Orden de Release Recomendado

```
v1.0.0 ── (actual, producción)
  │
  v1.0.1 ── Fase 1: sentinel removal, entityId, queryId fix
  │
  v1.1.0 ── Fase 2: broadphase unificado
  │
  v1.2.0 ── Fase 3: sensores / triggers
  │
  v1.3.0 ── Fase 4: pipeline v2 (opt-in via flag, legacy por default)
  │
  v1.4.0 ── Fase 5: tile attributes
  │
  v1.5.0 ── Fase 6: performance & stability
  │
  v2.0.0 ── Pipeline v2 como default, deprecar modo legacy
```

---

## Conclusión

El refactoring propuesto es **viable sin romper la v1.0.0 estable**, siempre que:

1. **Fases 1–3** se implementen como features aditivos (sin breaking changes).
2. **Fase 4** use un feature flag con default legacy para proteger juegos en producción.
3. **Fases 5–6** sean aditivas y opcionales.
4. El impacto acumulado en memoria (~6KB en ESP32C3, ~12KB en ESP32dev) se mantenga dentro del presupuesto de RAM.
5. Cada fase pase la suite completa de tests de regresión en las tres plataformas antes de merge.

El mayor riesgo es la **Fase 4** (pipeline reorder + integración centralizada), que debería tratarse como un cambio opt-in hasta que se valide en todos los proyectos dependientes. El salto a **v2.0.0** solo debería ocurrir cuando el nuevo pipeline haya sido probado en al menos un juego completo en hardware.
