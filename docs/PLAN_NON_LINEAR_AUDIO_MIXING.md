# Plan de Optimización: Mezcla de Audio No-Lineal y Soporte Multi-Arquitectura (ESP32 / C3)

Este documento detalla el plan para mejorar el volumen general del sistema de audio y garantizar la compatibilidad eficiente con variantes de la ESP32 sin unidad de punto flotante (FPU), como la ESP32-C3.

## 1. El Problema Actual
El sistema utiliza una mezcla lineal con un factor de escala (headroom) de **0.25x** por canal.
*   **Volumen Bajo**: Los picos de audio actuales rondan el **45%**, desperdiciando el rango dinámico del DAC.
*   **Ineficiencia en C3**: El uso de `float` en procesadores sin FPU (RISC-V mononúcleo) consume ciclos de CPU críticos que afectan la estabilidad del motor.

## 2. Propuesta: Mezcla No-Lineal (Estilo NES)
Inspirado en el hardware real de la NES, se propone una mezcla que actúa como un compresor analógico natural.

### Ventajas
*   **Mayor Volumen**: Permite que canales individuales usen hasta un 60-70% del rango dinámico.
*   **Protección Total**: La curva asintótica garantiza que la suma nunca exceda el límite de 16 bits (32767).
*   **Sonido Auténtico**: Produce los armónicos característicos de la consola original.

## 3. Estrategia de Implementación por Arquitectura

### A. Para ESP32 Clásica / S3 (Con FPU)
Se utilizará una aproximación matemática de la fórmula de la NES adaptada a floats para mayor precisión.

**Fórmula Propuesta:**
```cpp
float mixed = (pulse_sum * 0.15f) + (tnd_sum * 0.10f); // Aproximación simplificada
// O mejor aún, una curva de saturación suave:
float output = mixed / (1.0f + abs(mixed * 0.2f));
```

### B. Para ESP32-C3 / Mononúcleo (Sin FPU)
Se utilizará **aritmética de enteros (int32)** y una **Look-Up Table (LUT)** para la compresión no-lineal.

**Pasos:**
1.  **Acumulación**: Sumar canales en `int32_t` para evitar desbordamiento intermedio.
2.  **Compresión vía LUT**: Usar el valor sumado como índice para una tabla precalculada de 512 o 1024 entradas.
3.  **Cero Flotantes**: Todo el proceso se realiza con desplazamientos de bits y acceso a memoria, ideal para el núcleo RISC-V.

## 4. Fases del Proyecto

### Fase 1: Investigación y Prototipado
- [ ] Definir la curva de compresión ideal que eleve los picos promedio del 45% al ~80%.
- [ ] Generar la LUT para sistemas sin FPU.

### Fase 2: Abstracción en el Engine
- [ ] Modificar `AudioScheduler` para soportar diferentes implementaciones de mezcla según el hardware (`#ifdef`).
- [ ] Implementar `IntegerNonLinearMixer` para arquitecturas RISC-V.

### Fase 3: Validación
- [ ] Verificar picos de audio en Serial Monitor (objetivo: 75-85% sin distorsión).
- [ ] Medir impacto de CPU en ESP32-C3 para asegurar 60 FPS estables.

## 5. Conclusión
Este plan no solo resuelve el problema del volumen bajo, sino que hace que el motor PixelRoot32 sea verdaderamente multiplataforma y eficiente, respetando las limitaciones de hardware de cada variante de la ESP32.
