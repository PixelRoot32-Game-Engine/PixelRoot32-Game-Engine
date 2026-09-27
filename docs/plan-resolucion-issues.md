# Plan de resolución de issues abiertos

> Fuente: https://github.com/PixelRoot32-Game-Engine/PixelRoot32-Game-Engine/issues
> Fecha de análisis: 2026-09-27
> Issues abiertos analizados: 7 (#240 – #246)

## 1. Resumen ejecutivo

6 de 7 issues pertenecen al **subsistema de física** y fueron encontrados en la misma revisión (preparación de la demo Lunar Pool). 1 issue (#240) pertenece al **sistema de transiciones** y es totalmente independiente.

No hay dependencia circular. El orden recomendado es:

1. Corregir lo que **miente o falla en silencio** (#243, #242, #240-parte API/docs).
2. Aterrizar lo **barato y desbloqueante** (#244, #245).
3. Añadir comportamiento nuevo de física (#246).
4. Dejar #241 (segmentos) para el final, porque está **explícitamente bloqueado** por la demo externa.

## 2. Inventario y clasificación por prioridad

| # | Título | Tipo | Subsistema | Prioridad | Motivo |
|---|--------|------|------------|-----------|--------|
| #243 | Physics drops bodies and contacts silently when a capacity limit is reached | bug | Física / broad-phase + contactos | **P0 — crítica** | Pérdida silenciosa de colisiones. El síntoma (atravesar paredes) no apunta a la causa. Afecta a cualquier juego con muchos cuerpos, no solo al pool. |
| #242 | PIXELROOT32_VELOCITY_DAMPING and PIXELROOT32_MAX_VELOCITY have no effect | bug + documentation | Física / config | **P0 — crítica** | Flags documentados que compilan pero no hacen nada. Tres `platformio.ini` (engine + 2 demos) los usan esperando un efecto. Erosiona confianza en `docs/api/config.md`. |
| #240 | Expose DiagonalWipe direction and sub-step through triggerTransition | enhancement + documentation | Transiciones (`Engine`, `SceneManager`, `TransitionEffect`) | **P0 — alta (aislada)** | El changelog de 1.6.0 anuncia una feature inalcanzable por API pública + guía `docs/guide/scenes.md` con firmas inexistentes. Paralelizable con todo lo de física. |
| #244 | Physics docs overstate determinism across hardware | documentation | Física / docs | **P1 — quick win** | Dos docs prometen determinismo cross-hardware que no existe. Riesgo hacia roadmap (ESP-NOW "deterministic state sync", replays, lockstep). Solo docs + 1 test de respaldo. |
| #245 | Make position correction (BIAS, SLOP) configurable | enhancement | Física / solver | **P1 — pequeña** | Cambio mecánico (2 flags estilo `PIXELROOT32_VELOCITY_ITERATIONS`), corrige guía de migración que pide editar headers del engine. Mejora directa para #241. |
| #246 | Let rigid bodies come to rest and report when they have stopped | enhancement | Física / `RigidActor`, `CollisionSystem` | **P2 — feature** | Necesaria para juegos por turnos (pool, golf, artillería). Requiere decidir qué hacer con `MIN_VELOCITY` (ver #242). Cambio de comportamiento: necesita default que preserve lo actual. |
| #241 | Support collision against line segments in the physics system | enhancement | Física / formas de colisión | **P3 — diferida / bloqueada** | La más grande. Decisión explícita del issue: probar primero en la demo Lunar Pool (Demo-Projects) y luego portar al engine. No empezar antes. |

Severidad alternativa vista como matriz:

- **Correctitud silenciosa:** #243 > #242 > #240.
- **Esfuerzo:** #244 (XS) < #245 (S) < #242 (S) < #243 (S/M) < #240 (M) < #246 (M) < #241 (L).
- **Riesgo de API:** #240 y #246 añaden API pública; #242 puede eliminarla; #241 la definirá desde lo aprendido en la demo.

## 3. Mapa de relaciones y dependencias

```
Cluster FÍSICA (todo relacionado, encontrado junto a #241)

  #241 segmentos (TRACKER / META)
   ├── cita como gaps separados, NO parte de su scope:
   │    ├── #245 corrección lenta de overlap (BIAS 20%/step)
   │    ├── #243 broad-phase pierde cuerpos (>12 por celda)
   │    ├── #246 sin umbral de reposo (fricción proporcional)
   │    ├── #242 flags muertos (VELOCITY_DAMPING / MAX_VELOCITY)
   │    └── #244 determinismo solo por build, no cross-hardware
   └── bloqueada por: demo Lunar Pool en Demo-Projects
        (física propia de bolas → validar → portar al engine)

  Dependencias internas del cluster:
   #242 ──decide MIN_VELOCITY──> #246
     (implementar vs. eliminar; MIN_VELOCITY se reutiliza o se elimina
      en el umbral de reposo)
   #243 ──necesaria para──> #241
     (un rack de 15 bolas ya supera el cap de 12/celda)
   #245 ──mejora a──> #241
     (separación visible durante varios frames con BIAS=0.2)
   #246 ──necesaria para──> #241
     (saber cuándo termina un turno en el pool)
   #244 ──informa a──> roadmap ESP-NOW + futuros #241/#246
     (qué garantía de determinismo se puede prometer)

Cluster TRANSICIONES (aislado, en paralelo)

  #240 ──desbloquea──> Demo-Projects graphics/scene_transitions#2
   (quitar subclase de Engine, subir floor de lib_deps)
   Sin relación con #241–#246.
```

Regla de orden: **#242 antes que #246** (decisión sobre `MIN_VELOCITY`). Todo lo demás del cluster física es paralelizable una vez fijado eso, salvo #241 que va última.

## 4. Plan de resolución por fases

### Fase 0 — Verdad documentada (0.5–1 día, sin cambios de comportamiento)

- #244: reescribir las dos frases (`docs/architecture/physics-subsystem.md:15`, `docs/api/physics.md:125`) a la garantía real: mismo build + mismos inputs en los mismos steps ⇒ mismo resultado. Añadir nota `PR32_FORCE_FIXED` solo como vía experimental pendiente de test.
- #242 (parte docs): marcar ambas flags como no-op en `docs/api/config.md` hasta que se resuelvan, para no seguir propagando el error.
- #240 (parte docs): corregir `docs/guide/scenes.md:335-379` (firmas reales, swap automático, ejemplo de iris por overload, cubrir `DiagonalWipe`).

Salida: los docs dejan de mentir aunque el código aún no cambie.

### Fase 1 — Correctitud silenciosa (1–2 días)

1. **#243.** En `PIXELROOT32_DEBUG_MODE`, reportar primer-hit por límite (tabla del issue: `PHYSICS_MAX_ENTITIES`, per-cell static/dynamic, candidatos 64, `PHYSICS_MAX_CONTACTS`), nombrando límite + flag que lo eleva. Hacer configurable el buffer de candidatos (64 hard-coded). Documentar consecuencia junto a cada límite y junto al consejo DRAM (`physics-subsystem.md:531`). Tests que saturan cada buffer. Release sin coste/comportamiento nuevo.
2. **#242.** Decidir implementar vs. eliminar:
   - Si se implementa: defaults no-op (sin damping, sin cap) + entrada de changelog, porque 0.999 por frame frenaría Pong/Brick Breaker.
   - Si se elimina: borrar constantes, flags, menciones en docs y en los 3 `platformio.ini`.
   - En ambos casos: resolver qué pasa con `MIN_VELOCITY` y dejarlo atado para #246.
   - Tests: cada flag documentada o cambia comportamiento o desaparece; pin de "defaults no cambian juegos existentes".

### Fase 2 — Transiciones, en paralelo con Fase 1 (1 día, otro autor posible)

3. **#240.** Añadir `TransitionConfig` (tipo, duración, `WipeDirection`, centros iris, `subStepMs`) + overload `triggerTransition(Scene*, const TransitionConfig&)`; overloads existentes reenvían. `SceneManager` reaplica la descripción completa en fases Out e In (evita herencia de dirección). Tests vía `Engine`/`SceneManager`, no solo `TransitionEffect` directo. Actualizar `scenes.md` y notificar a Demo-Projects para retirar la subclase de `Engine`.

### Fase 3 — Tuning + reposo (1–2 días)

4. **#245.** Exponer `BIAS`/`SLOP` como flags con defaults actuales (0.2 / 0.02). Actualizar migración v1.0.0 + referencia de física. Test: pasos hasta separar dos círculos con defaults vs. `BIAS` mayor.
5. **#246.** Umbral de reposo opt-in (default = comportamiento actual): bajo umbral + sin fuerza ⇒ velocidad exactamente cero. Queries `isAtRest()` / `allBodiesAtRest()` (nombres del issue, a confirmar). Tests en float y `Fixed16` + borde del umbral. Considerar fricción de deceleración constante en el mismo cambio solo si no crece el scope.

### Fase 4 — Segmentos, tras la demo (bloqueada externamente)

6. **#241.** Solo cuando la física de bolas de Lunar Pool sea jugable y testeada en Demo-Projects: portar forma estática segmento + círculo-vs-segmento (normal perpendicular) + endpoints como círculos de radio cero + ruta de impulso/restitución existente. Tests: hit perpendicular, muro 45°, joint de esquina. Criterio de cierre: la demo sustituye su física propia por la del engine sin cambiar gameplay.

## 5. Orden de ejecución sugerido (secuencial si un solo autor)

```
#244 (docs) → #243 → #242 → #240 → #245 → #246 → #241
  XS           S/M      S       M       S       M       L
```

Si hay dos autores: uno toma física (#244→#243→#242→#245→#246) y otro #240 en paralelo. #241 siempre última.

## 6. Riesgos y notas

- #242 mal resuelto (activar damping 0.999 por defecto) rompería juegos existentes. Exigir defaults no-op.
- #246 sin default conservador cambia el feel de todos los cuerpos deslizantes. El issue ya lo pide, mantenerlo.
- #243: no convertir el reporte debug en coste release. Primer-hit + `DEBUG_MODE` como propone el issue.
- #244: no prometer reproducibilidad cross-target con `PR32_FORCE_FIXED` hasta tener el test scripted nativo-vs-ESP32 en verde.
- #241: no diseñar la API antes de la demo. El issue lo prohíbe explícitamente; respetar el bloqueo.
