#!/usr/bin/env python3
"""
Script para verificar que la cobertura de código cumple con el mínimo requerido.
Ejecuta los tests y genera reportes de cobertura.

Uso:
    python scripts/coverage_check.py
    python scripts/coverage_check.py --report  # Genera reporte HTML
"""

import subprocess
import sys
import os
import argparse
import re

# Configuración
MIN_LINE_COVERAGE = 80.0
MIN_FUNCTION_COVERAGE = 90.0
BUILD_DIR = ".pio/build/native_test"
COVERAGE_INFO = "coverage.info"
COVERAGE_FILTERED = "coverage_filtered.info"
COVERAGE_REPORT_DIR = "coverage_report"

def run_command(cmd, description):
    """Ejecuta un comando y maneja errores."""
    print(f"\n{'='*60}")
    print(f"{description}")
    print(f"{'='*60}")
    print(f"Comando: {' '.join(cmd)}")
    
    result = subprocess.run(cmd, capture_output=True, text=True)
    
    if result.stdout:
        print(result.stdout)
    if result.stderr and "error" in result.stderr.lower():
        print(f"STDERR: {result.stderr}")
    
    return result.returncode == 0

def run_tests():
    """Ejecuta los tests con PlatformIO."""
    print("\n🔍 Ejecutando tests...")
    return run_command(
        ["pio", "test", "-e", "native_test"],
        "EJECUTANDO TESTS"
    )

def generate_coverage():
    """Genera el archivo de cobertura con lcov."""
    print("\n📊 Generando datos de cobertura...")
    
    # Capturar datos de cobertura
    if not run_command(
        ["lcov", "--capture", "--directory", ".", "--output-file", COVERAGE_INFO],
        "CAPTURANDO DATOS DE COBERTURA"
    ):
        return False
    
    # Filtrar archivos del sistema y dependencias
    if not run_command(
        ["lcov", "--remove", COVERAGE_INFO, 
         "/usr/*", "*/.pio/*", "*/test/*", "*/lib/*",
         "--output-file", COVERAGE_FILTERED],
        "FILTRANDO DATOS"
    ):
        return False
    
    return True

def parse_coverage_summary():
    """Parsea el resumen de cobertura."""
    result = subprocess.run(
        ["lcov", "--summary", COVERAGE_FILTERED],
        capture_output=True,
        text=True
    )
    
    output = result.stderr if result.stderr else result.stdout
    
    # Extraer porcentajes
    lines_match = re.search(r'lines\.*:\s*(\d+\.?\d*)%', output)
    functions_match = re.search(r'functions\.*:\s*(\d+\.?\d*)%', output)
    branches_match = re.search(r'branches\.*:\s*(\d+\.?\d*)%', output)
    
    return {
        'lines': float(lines_match.group(1)) if lines_match else 0.0,
        'functions': float(functions_match.group(1)) if functions_match else 0.0,
        'branches': float(branches_match.group(1)) if branches_match else 0.0,
        'raw_output': output
    }

def generate_html_report():
    """Genera reporte HTML de cobertura."""
    print("\n🌐 Generando reporte HTML...")
    
    # Crear directorio si no existe
    os.makedirs(COVERAGE_REPORT_DIR, exist_ok=True)
    
    return run_command(
        ["genhtml", COVERAGE_FILTERED, "--output-directory", COVERAGE_REPORT_DIR],
        "GENERANDO REPORTE HTML"
    )

def print_coverage_report(coverage):
    """Imprime el reporte de cobertura."""
    print(f"\n{'='*60}")
    print("📈 REPORTE DE COBERTURA")
    print(f"{'='*60}")
    print(f"Line Coverage:      {coverage['lines']:6.2f}%  (mínimo: {MIN_LINE_COVERAGE}%)")
    print(f"Function Coverage:  {coverage['functions']:6.2f}%  (mínimo: {MIN_FUNCTION_COVERAGE}%)")
    if coverage['branches'] > 0:
        print(f"Branch Coverage:    {coverage['branches']:6.2f}%")
    print(f"{'='*60}")

def check_coverage_requirements(coverage):
    """Verifica que se cumplan los requisitos de cobertura."""
    success = True
    
    if coverage['lines'] < MIN_LINE_COVERAGE:
        print(f"\n❌ Cobertura de líneas por debajo del mínimo ({MIN_LINE_COVERAGE}%)")
        success = False
    else:
        print(f"\n✅ Cobertura de líneas OK")
    
    if coverage['functions'] < MIN_FUNCTION_COVERAGE:
        print(f"❌ Cobertura de funciones por debajo del mínimo ({MIN_FUNCTION_COVERAGE}%)")
        success = False
    else:
        print(f"✅ Cobertura de funciones OK")
    
    return success

def clean_coverage_files():
    """Limpia archivos temporales de cobertura."""
    files_to_remove = [
        COVERAGE_INFO,
        COVERAGE_FILTERED,
        "*.gcov",
        "*.gcda",
        "*.gcno"
    ]
    
    for pattern in files_to_remove:
        try:
            if "*" in pattern:
                import glob
                for f in glob.glob(pattern):
                    os.remove(f)
            elif os.path.exists(pattern):
                os.remove(pattern)
        except:
            pass

def main():
    parser = argparse.ArgumentParser(description="Script de verificación de cobertura de código")
    parser.add_argument("--report", action="store_true", help="Genera reporte HTML")
    parser.add_argument("--clean", action="store_true", help="Limpia archivos temporales")
    parser.add_argument("--no-tests", action="store_true", help="No ejecuta tests, solo genera reporte")
    args = parser.parse_args()
    
    if args.clean:
        print("🧹 Limpiando archivos temporales...")
        clean_coverage_files()
        return 0
    
    # Ejecutar tests
    if not args.no_tests:
        if not run_tests():
            print("\n❌ Tests fallaron")
            return 1
        print("\n✅ Tests pasaron correctamente")
    
    # Generar cobertura
    if not generate_coverage():
        print("\n❌ Error generando datos de cobertura")
        return 1
    
    # Parsear y mostrar resumen
    coverage = parse_coverage_summary()
    print_coverage_report(coverage)
    
    # Generar reporte HTML si se solicita
    if args.report:
        if generate_html_report():
            print(f"\n✅ Reporte HTML generado en: {COVERAGE_REPORT_DIR}/index.html")
            print(f"   Abre el archivo en tu navegador para ver el reporte detallado")
    
    # Verificar requisitos
    success = check_coverage_requirements(coverage)
    
    # Resultado final
    print(f"\n{'='*60}")
    if success:
        print("🎉 ¡COBERTURA CUMPLE CON LOS REQUISITOS!")
    else:
        print("⚠️  COBERTURA NO CUMPLE CON LOS REQUISITOS MÍNIMOS")
    print(f"{'='*60}\n")
    
    return 0 if success else 1

if __name__ == "__main__":
    sys.exit(main())
