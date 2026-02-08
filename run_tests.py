#!/usr/bin/env python3
"""
Script para ejecutar tests de PixelRoot32 Game Engine
Este script maneja mejor los errores de compilación en Windows/Git Bash
"""

import subprocess
import sys
import os
from pathlib import Path

# Configuración
CXX = "g++"
CXXFLAGS = ["-std=c++17", "-Wall", "-Wextra", "-g", "-O0", "-Iinclude", "-DPLATFORM_NATIVE", "-DUNIT_TEST"]
UNITY_DIR = ".pio/libdeps/native_test/Unity/src"
BUILD_DIR = Path("build/tests")

def run_command(cmd, description):
    """Ejecuta un comando y muestra errores"""
    print(f"\n{'='*60}")
    print(f"{description}")
    print(f"{'='*60}")
    print(f"Comando: {' '.join(cmd)}")
    
    try:
        result = subprocess.run(
            cmd,
            capture_output=True,
            text=True,
            encoding='utf-8',
            errors='replace'
        )
        
        if result.stdout:
            print("STDOUT:", result.stdout)
        if result.stderr:
            print("STDERR:", result.stderr)
        
        if result.returncode != 0:
            print(f"[X] Error (codigo {result.returncode})")
            return False
        
        print("[OK] Exito")
        return True
        
    except Exception as e:
        print(f"[X] Excepcion: {e}")
        return False

def compile_and_run(test_name, test_file, output_name, source_files=None):
    """Compila y ejecuta un test"""
    BUILD_DIR.mkdir(parents=True, exist_ok=True)
    
    output_path = BUILD_DIR / output_name
    test_path = Path(test_file)
    unity_c = Path(UNITY_DIR) / "unity.c"
    
    # Verificar que los archivos existen
    if not test_path.exists():
        print(f"❌ No existe: {test_path}")
        return False
    
    if not unity_c.exists():
        print(f"❌ No existe Unity. Instalando...")
        subprocess.run(["pio", "lib", "install", "--global", "Unity"])
    
    # Preparar lista de archivos a compilar
    files = [str(test_path), str(unity_c)]
    if source_files:
        files.extend(source_files)
    
    # Compilar
    cmd = [CXX] + CXXFLAGS + [f"-I{UNITY_DIR}"] + files + ["-o", str(output_path)]
    
    if not run_command(cmd, f"Compilando {test_name}"):
        return False
    
    # Ejecutar
    print(f"\n>> Ejecutando {test_name}...")
    return run_command([str(output_path)], f"Tests de {test_name}")

def main():
    print("="*60)
    print("PixelRoot32 Game Engine - Test Runner")
    print("="*60)
    
    # Simple argument parsing for filtering
    filter_name = None
    if len(sys.argv) > 1:
        if sys.argv[1] == "--only" and len(sys.argv) > 2:
            filter_name = sys.argv[2]
        else:
            filter_name = sys.argv[1]
    
    # Verificar g++
    try:
        result = subprocess.run([CXX, "--version"], capture_output=True, text=True)
        print(f"\nCompilador: {result.stdout.splitlines()[0]}")
    except:
        print(f"❌ No se encontró {CXX}")
        sys.exit(1)
    
    # Tests a ejecutar: (nombre, archivo_test, ejecutable, [archivos_fuente_opcionales])
    tests = [
        ("Math", "test/unit/test_math/test_mathutil.cpp", "test_mathutil", None),
        ("Core-Rect", "test/unit/test_rect/test_rect.cpp", "test_rect", None),
        ("Core-Entity", "test/unit/test_entity/test_entity.cpp", "test_entity", None),
        ("Core-Actor", "test/unit/test_actor/test_actor.cpp", "test_actor", None),
        ("Core-Scene", "test/unit/test_scene/test_scene.cpp", "test_scene", None),
        ("Core-SceneManager", "test/unit/test_scene_manager/test_scene_manager.cpp", "test_scene_manager", None),
        ("Physics-Types", "test/unit/test_collision_types/test_collision_types.cpp", "test_collision_types", None),
        ("Physics-Primitives", "test/unit/test_collision_primitives/test_collision_primitives.cpp", "test_collision_primitives", None),
        ("Physics-System", "test/unit/test_collision_system/test_collision_system.cpp", "test_collision_system", None),
        ("Graphics-Color", "test/unit/test_color/test_color.cpp", "test_color", None),
        ("Graphics-Camera2D", "test/unit/test_camera2d/test_camera2d.cpp", "test_camera2d", None),
        ("Graphics-FontManager", "test/unit/test_font_manager/test_font_manager.cpp", "test_font_manager", None),
        ("Input-Config", "test/unit/test_input_config/test_input_config.cpp", "test_input_config", None),
        ("Input-Manager", "test/unit/test_input_manager/test_input_manager.cpp", "test_input_manager", ["src/input/InputManager.cpp", "src/platforms/mock/MockArduino.cpp"]),
        ("Audio-Queue", "test/unit/test_audio_command_queue/test_audio_command_queue.cpp", "test_audio_command_queue", None),
        ("Audio-Scheduler", "test/unit/test_audio_scheduler/test_audio_scheduler.cpp", "test_audio_scheduler", ["src/audio/DefaultAudioScheduler.cpp"]),
        ("Audio-Music", "test/unit/test_music_player/test_music_player.cpp", "test_music_player", ["src/audio/MusicPlayer.cpp", "src/audio/AudioEngine.cpp", "src/audio/DefaultAudioScheduler.cpp"]),
    ]
    
    results = []
    for name, file, output, sources in tests:
        if filter_name and filter_name.lower() not in name.lower():
            continue
            
        success = compile_and_run(name, file, output, sources)
        results.append((name, success))
        print()
    
    # Resumen
    print("="*60)
    print("RESUMEN")
    print("="*60)
    
    passed = sum(1 for _, success in results if success)
    total = len(results)
    
    for name, success in results:
        status = "[OK] PASO" if success else "[X] FALLO"
        print(f"{name:15} {status}")
    
    print(f"\nTotal: {passed}/{total} tests pasaron")
    
    if passed == total:
        print("\n*** TODOS LOS TESTS PASARON ***")
        return 0
    else:
        print(f"\n[!] {total - passed} test(s) fallaron")
        return 1

if __name__ == "__main__":
    sys.exit(main())
