# Plan Optimizado de Diagnóstico – Distorsión Audio ESP32

## 🎯 Objetivo

Determinar si la distorsión proviene de:

* Saturación digital en el mixer
* Falta de headroom
* Problema en backend (I2S o DAC)

Sin rediseñar el sistema antes de confirmar la causa.

---

# Fase 1 — Test de Aislamiento (10–15 min)

### 1️⃣ Probar con **1 solo canal activo**

* Desactivar 3 canales.
* masterVolume = 1.0
* Escuchar en:

  * SDL2
  * I2S
  * DAC

**Resultado esperado:**

* Si suena limpio → el problema es saturación por suma.
* Si ya suena distorsionado → problema en backend.

---

### 2️⃣ Bajar masterVolume a 0.3

Sin cambiar nada más.

* Si mejora radicalmente → confirmación de clipping digital.
* Si no mejora → revisar backend.

---

# Fase 2 — Medición Real (15 min)

### 3️⃣ Instrumentar pico máximo

Agregar medición simple:

```cpp
if (abs(acc) > peak) peak = abs(acc);
```

Loggear:

* Pico con 1 canal
* Pico con 2
* Pico con 4

Si los valores se acercan a ±32767 constantemente → confirmación de saturación.

---

# Fase 3 — Corrección del Mixer (30 min)

### 4️⃣ Cambiar estrategia de mezcla

* Acumular en `float acc`
* NO clipear por canal
* Aplicar headroom fijo:

```cpp
acc *= 0.25f;  // para 4 canales
acc *= masterVolume;
```

* Clampear una sola vez al final

Volver a probar.

Si se corrige → el problema estaba en la mezcla.

---

# Fase 4 — Backend (solo si aún falla)

### 5️⃣ Verificación rápida I2S

* Confirmar formato I2S estándar (Philips)
* Confirmar 16-bit MSB
* Confirmar mono correctamente duplicado si necesario

### 6️⃣ Verificación rápida DAC

* Confirmar mapeo correcto:

  * `(sample >> 8) + 128`
* Confirmar que no hay overflow antes del shift

---

# Fase 5 — Validación Final

### 7️⃣ Comparación cruzada

* Generar WAV desde SDL2
* Comparar forma de onda con buffer enviado a I2S

Si las formas son iguales → problema es físico (DAC / amplificador).
Si no → problema es digital.

---

# 🔎 Resultado Esperado

En 80–90% de los casos como este, la causa es:

> Falta de headroom + clipping acumulativo

Y se soluciona en Fase 3.

---

# 🧠 Filosofía del Plan

* Cambiar una sola variable a la vez
* Confirmar antes de rediseñar
* No agregar compresión, dithering ni filtros hasta validar el mixer