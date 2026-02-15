#!/usr/bin/env python3
"""
Script to run PixelRoot32 Game Engine tests
This script handles compilation errors better on Windows/Git Bash
"""

import subprocess
import sys
import os
import shutil
from pathlib import Path

# Configuration
CXX = "g++"
CXXFLAGS = ["-std=c++11", "-Wall", "-Wextra", "-g", "-O0", "-Iinclude", "-DPLATFORM_NATIVE", "-DUNIT_TEST", "-DSDL_MAIN_HANDLED", "-DTEST_MOCK_GRAPHICS"]
# Unity might be in different paths depending on whether it was installed via PIO or manually
UNITY_DIR = Path(".pio/libdeps/native_test/Unity/src")
if not UNITY_DIR.exists():
    # Try to find it in a common alternative path
    alt_unity = Path("test/lib/Unity/src")
    if alt_unity.exists():
        UNITY_DIR = alt_unity
    else:
        # Revert to default path for installation
        UNITY_DIR = Path(".pio/libdeps/native_test/Unity/src")

BUILD_DIR = Path("build/tests")

def ensure_unity():
    """Ensures Unity is available, installing it if necessary"""
    unity_c = UNITY_DIR / "unity.c"
    unity_h = UNITY_DIR / "unity.h"
    
    if unity_c.exists() and unity_h.exists():
        return True
        
    print(f"\n[!] Unity not found in {UNITY_DIR}")
    print("Attempting to install Unity...")
    
    # Attempt 1: Use PlatformIO if available
    try:
        print("Attempting via PlatformIO (pio pkg install)...")
        # Ensure directory exists so pio doesn't fail if there's no environment
        subprocess.run(["pio", "pkg", "install", "-e", "native_test"], check=True)
        if unity_c.exists():
            print("[OK] Unity installed via PlatformIO")
            return True
    except (subprocess.CalledProcessError, FileNotFoundError):
        print("[-] PlatformIO not available or installation failed")

    # Attempt 2: Download directly from GitHub (using git)
    try:
        print("Attempting to clone Unity from GitHub...")
        temp_dir = Path("temp_unity")
        if temp_dir.exists():
            shutil.rmtree(temp_dir)
            
        subprocess.run(["git", "clone", "--depth", "1", "https://github.com/ThrowTheSwitch/Unity.git", str(temp_dir)], check=True)
        
        # Create expected directory structure
        UNITY_DIR.mkdir(parents=True, exist_ok=True)
        
        # Copy necessary files
        for f in ["unity.c", "unity.h", "unity_internals.h"]:
            src = temp_dir / "src" / f
            if src.exists():
                shutil.copy(src, UNITY_DIR / f)
        
        # Clean up
        shutil.rmtree(temp_dir)
        
        if unity_c.exists():
            print("[OK] Unity downloaded and configured successfully")
            return True
    except (subprocess.CalledProcessError, FileNotFoundError) as e:
        print(f"[-] Error attempting to clone with git: {e}")

    print("❌ Could not install Unity automatically.")
    print("Please install it manually in .pio/libdeps/native_test/Unity/src")
    return False

def run_command(cmd, description):
    """Executes a command and shows errors"""
    print(f"\n{'='*60}")
    print(f"{description}")
    print(f"{'='*60}")
    print(f"Command: {' '.join(cmd)}")
    
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
            print(f"[X] Error (code {result.returncode})")
            return False
        
        print("[OK] Success")
        return True
        
    except Exception as e:
        print(f"[X] Exception: {e}")
        return False

def compile_and_run(test_name, test_file, output_name, source_files=None):
    """Compiles and runs a test"""
    BUILD_DIR.mkdir(parents=True, exist_ok=True)
    
    output_path = BUILD_DIR / output_name
    test_path = Path(test_file)
    unity_c = UNITY_DIR / "unity.c"
    
    # Verify files exist
    if not test_path.exists():
        print(f"❌ Does not exist: {test_path}")
        return False
    
    # Prepare list of files to compile
    files = [str(test_path), str(unity_c)]
    if source_files:
        files.extend(source_files)
    
    # Compile
    cmd = [CXX] + CXXFLAGS + [f"-I{UNITY_DIR}"] + files + ["-o", str(output_path)]
    
    if not run_command(cmd, f"Compiling {test_name}"):
        return False
    
    # Execute
    print(f"\n>> Running {test_name}...")
    return run_command([str(output_path)], f"{test_name} Tests")

def main():
    print("="*60)
    print("PixelRoot32 Game Engine - Test Runner")
    print("="*60)
    
    # Ensure Unity before starting
    if not ensure_unity():
        print("❌ Error: Cannot continue without Unity.")
        sys.exit(1)
    
    # Simple argument parsing for filtering
    filter_name = None
    if len(sys.argv) > 1:
        if sys.argv[1] == "--only" and len(sys.argv) > 2:
            filter_name = sys.argv[2]
        else:
            filter_name = sys.argv[1]
    
    # Verify g++
    try:
        result = subprocess.run([CXX, "--version"], capture_output=True, text=True)
        print(f"\nCompiler: {result.stdout.splitlines()[0]}")
    except:
        print(f"❌ {CXX} not found")
        sys.exit(1)
    
    # Tests to run: (name, test_file, executable, [optional_source_files])
    tests = [
        ("Math", "test/unit/test_math/test_mathutil.cpp", "test_mathutil", None),
        ("Core-Rect", "test/unit/test_rect/test_rect.cpp", "test_rect", None),
        ("Core-Entity", "test/unit/test_entity/test_entity.cpp", "test_entity", ["src/graphics/Renderer.cpp", "src/graphics/Color.cpp", "src/graphics/FontManager.cpp", "src/graphics/Font5x7.cpp", "src/graphics/DisplayConfig.cpp"]),
        ("Core-Actor", "test/unit/test_actor/test_actor.cpp", "test_actor", ["src/graphics/Renderer.cpp", "src/graphics/Color.cpp", "src/graphics/FontManager.cpp", "src/graphics/Font5x7.cpp", "src/graphics/DisplayConfig.cpp"]),
        ("Core-Scene", "test/unit/test_scene/test_scene.cpp", "test_scene", ["src/core/Scene.cpp", "src/physics/CollisionSystem.cpp", "src/physics/CollisionPrimitives.cpp", "src/graphics/Renderer.cpp", "src/graphics/Color.cpp", "src/graphics/FontManager.cpp", "src/graphics/Font5x7.cpp", "src/graphics/DisplayConfig.cpp"]),
        ("Core-SceneManager", "test/unit/test_scene_manager/test_scene_manager.cpp", "test_scene_manager", ["src/core/SceneManager.cpp", "src/core/Scene.cpp", "src/physics/CollisionSystem.cpp", "src/physics/CollisionPrimitives.cpp", "src/graphics/Renderer.cpp", "src/graphics/Color.cpp", "src/graphics/FontManager.cpp", "src/graphics/Font5x7.cpp", "src/graphics/DisplayConfig.cpp"]),
        ("Physics-Types", "test/unit/test_collision_types/test_collision_types.cpp", "test_collision_types", None),
        ("Physics-Primitives", "test/unit/test_collision_primitives/test_collision_primitives.cpp", "test_collision_primitives", ["src/physics/CollisionPrimitives.cpp"]),
        ("Physics-System", "test/unit/test_collision_system/test_collision_system.cpp", "test_collision_system", ["src/physics/CollisionSystem.cpp", "src/physics/CollisionPrimitives.cpp"]),
        ("Graphics-Color", "test/unit/test_color/test_color.cpp", "test_color", ["src/graphics/Color.cpp"]),
        ("Graphics-Camera2D", "test/unit/test_camera2d/test_camera2d.cpp", "test_camera2d", ["src/graphics/Camera2D.cpp", "src/graphics/Renderer.cpp", "src/graphics/Color.cpp", "src/graphics/FontManager.cpp", "src/graphics/Font5x7.cpp", "src/graphics/DisplayConfig.cpp"]),
        ("Graphics-FontManager", "test/unit/test_font_manager/test_font_manager.cpp", "test_font_manager", ["src/graphics/FontManager.cpp", "src/graphics/Font5x7.cpp"]),
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
    
    # Summary
    print("="*60)
    print("SUMMARY")
    print("="*60)
    
    passed = sum(1 for _, success in results if success)
    total = len(results)
    
    for name, success in results:
        status = "[OK] PASSED" if success else "[X] FAILED"
        print(f"{name:15} {status}")
    
    print(f"\nTotal: {passed}/{total} tests passed")
    
    if passed == total:
        print("\n*** ALL TESTS PASSED ***")
        return 0
    else:
        print(f"\n[!] {total - passed} test(s) failed")
        return 1

if __name__ == "__main__":
    sys.exit(main())
