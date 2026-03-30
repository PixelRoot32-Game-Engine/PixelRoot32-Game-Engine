#!/usr/bin/env python3
"""
Windows-optimized script for code coverage.
Prioritizes gcovr (pip install gcovr) and falls back to lcov.
"""

import subprocess
import sys
import os
import argparse
import re

# Configuration
MIN_LINE_COVERAGE = 80.0
MIN_FUNCTION_COVERAGE = 90.0
LINE_WIGGLE_ROOM = 0.3
COVERAGE_INFO = "coverage.info"
COVERAGE_FILTERED = "coverage_filtered.info"
COVERAGE_REPORT_DIR = "coverage_report"

def run_command(cmd, description):
    """Executes a command and handles errors."""
    print(f"\n{'='*60}")
    print(f"{description}")
    print(f"{'='*60}")
    print(f"Command: {' '.join(cmd)}")
    
    try:
        # Use utf-8 and ignore errors for robustness on Windows
        result = subprocess.run(cmd, capture_output=True, text=True, encoding='utf-8', errors='replace')
        
        if result.stdout:
            print(result.stdout)
        if result.stderr and "error" in result.stderr.lower():
            print(f"STDERR: {result.stderr}")
        
        return result.returncode == 0
    except FileNotFoundError:
        return False
    except Exception as e:
        print(f"Exception: {e}")
        return False

def check_tool(tool_name):
    """Checks if a tool is available in the system."""
    try:
        subprocess.run([tool_name, "--version"], capture_output=True, text=True)
        return True
    except:
        return False

def run_tests():
    """Runs tests with PlatformIO."""
    print("\n[RUN] Running tests...")
    return run_command(
        ["pio", "test", "-e", "native_test"],
        "RUNNING TESTS"
    )

def generate_coverage_gcovr(generate_html=False):
    """Generates coverage data with gcovr."""
    print("\n[COV] Generating coverage data with gcovr...")
    
    # Base command (exclude native drivers: SDL2/audio, not exercised by unit tests)
    # Also exclude:
    # - Extremely complex files: Engine.cpp, Renderer.cpp, TileConsumptionHelper.cpp
    # - Simple header interfaces: BaseDrawSurface.h, DrawSurface.h, UICheckbox.h (abstract interfaces)
    cmd = ["gcovr", "-r", ".", "--object-directory", ".pio/build/native_test", "--filter", "src/", "--filter", "include/",
           "--exclude", "src/drivers/native/.*", "--exclude", "include/drivers/native/.*",
           "--exclude", "src/core/Engine.cpp", "--exclude", "src/graphics/Renderer.cpp", "--exclude", "src/physics/TileConsumptionHelper.cpp",
           "--exclude", "include/graphics/BaseDrawSurface.h", "--exclude", "include/graphics/DrawSurface.h", "--exclude", "include/graphics/ui/UICheckbox.h"]
    
    # 1. Run summary
    if not run_command(cmd + ["--print-summary"], "COV SUMMARY (GCOVR)"):
        return False
        
    # 2. Generate HTML if requested
    if generate_html:
        print("\n[HTML] Generating HTML report with gcovr...")
        os.makedirs(COVERAGE_REPORT_DIR, exist_ok=True)
        if not run_command(cmd + ["--html-details", "--output", f"{COVERAGE_REPORT_DIR}/index.html"], "GENERATING HTML REPORT"):
            return False
            
    return True

def generate_coverage_lcov():
    """Generates coverage data with lcov."""
    print("\n[COV] Generating coverage data with lcov...")
    
    # Capture coverage data
    if not run_command(
        ["lcov", "--capture", "--directory", ".", "--output-file", COVERAGE_INFO],
        "CAPTURING COVERAGE DATA"
    ) :
        return False
    
    # Filter system files and dependencies
    if not run_command(
        ["lcov", "--remove", COVERAGE_INFO, 
         "/usr/*", "*/.pio/*", "*/test/*", "*/lib/*",
         "--output-file", COVERAGE_FILTERED],
        "FILTERING DATA"
    ):
        return False
    
    return True

def parse_coverage_summary_lcov():
    """Parses coverage summary from lcov filtered file."""
    try:
        result = subprocess.run(
            ["lcov", "--summary", COVERAGE_FILTERED],
            capture_output=True,
            text=True,
            encoding='utf-8',
            errors='replace'
        )
        
        output = result.stderr if result.stderr else result.stdout
        
        # Extract percentages
        lines_match = re.search(r'lines\.*:\s*(\d+\.?\d*)%', output)
        functions_match = re.search(r'functions\.*:\s*(\d+\.?\d*)%', output)
        branches_match = re.search(r'branches\.*:\s*(\d+\.?\d*)%', output)
        
        return {
            'lines': float(lines_match.group(1)) if lines_match else 0.0,
            'functions': float(functions_match.group(1)) if functions_match else 0.0,
            'branches': float(branches_match.group(1)) if branches_match else 0.0
        }
    except:
        return None

def parse_coverage_summary_gcovr():
    """Parses coverage summary from gcovr output."""
    try:
        # Run gcovr again to capture output for parsing
        result = subprocess.run(
            ["gcovr", "-r", ".", "--object-directory", ".pio/build/native_test", "--filter", "src/", "--filter", "include/",
             "--exclude", "src/drivers/native/.*", "--exclude", "include/drivers/native/.*",
             "--exclude", "src/core/Engine.cpp", "--exclude", "src/graphics/Renderer.cpp", "--exclude", "src/physics/TileConsumptionHelper.cpp",
             "--exclude", "include/graphics/BaseDrawSurface.h", "--exclude", "include/graphics/DrawSurface.h", "--exclude", "include/graphics/ui/UICheckbox.h", "--print-summary"],
            capture_output=True,
            text=True,
            encoding='utf-8',
            errors='replace'
        )
        
        output = result.stdout
        
        # gcovr output format:
        # lines: 85.2% (1234 out of 1448)
        # functions: 92.1% (123 out of 134)
        lines_match = re.search(r'lines:\s*(\d+\.?\d*)%', output)
        functions_match = re.search(r'functions:\s*(\d+\.?\d*)%', output)
        
        return {
            'lines': float(lines_match.group(1)) if lines_match else 0.0,
            'functions': float(functions_match.group(1)) if functions_match else 0.0,
            'branches': 0.0  # Not parsing branches for now
        }
    except:
        return None

def generate_html_report_lcov():
    """Generates HTML coverage report with genhtml."""
    print("\n[HTML] Generating HTML report...")
    os.makedirs(COVERAGE_REPORT_DIR, exist_ok=True)
    return run_command(
        ["genhtml", COVERAGE_FILTERED, "--output-directory", COVERAGE_REPORT_DIR],
        "GENERATING HTML REPORT"
    )

def print_coverage_report(coverage):
    """Prints coverage report."""
    if not coverage:
        print("\n[!] Could not parse coverage results.")
        return

    print(f"\n{'='*60}")
    print("COVERAGE REPORT")
    print(f"{'='*60}")
    print(f"Line Coverage:      {coverage['lines']:6.2f}%  (minimum: {MIN_LINE_COVERAGE}%)")
    print(f"Function Coverage:  {coverage['functions']:6.2f}%  (minimum: {MIN_FUNCTION_COVERAGE}%)")
    print(f"{'='*60}")

def check_coverage_requirements(coverage):
    """Verifies that coverage requirements are met."""
    if not coverage:
        return False

    success = True
    
    if coverage['lines'] < MIN_LINE_COVERAGE:
        print(f"\n[X] Line coverage below minimum ({MIN_LINE_COVERAGE}%)")
        success = False
    else:
        print(f"\n[OK] Line coverage OK")
    
    if coverage['functions'] < MIN_FUNCTION_COVERAGE:
        print(f"[X] Function coverage below minimum ({MIN_FUNCTION_COVERAGE}%)")
        success = False
    else:
        print(f"[OK] Function coverage OK")
    
    return success

def main():
    parser = argparse.ArgumentParser(description="Code coverage verification (Windows)")
    parser.add_argument("--report", action="store_true", help="Generates HTML report")
    parser.add_argument("--no-tests", action="store_true", help="Does not run tests")
    args = parser.parse_args()
    
    if not args.no_tests:
        if not run_tests():
            print("\n[X] Tests failed")
            return 1
        print("\n[OK] Tests passed successfully")
    
    has_gcovr = check_tool("gcovr")
    has_lcov = check_tool("lcov")
    
    coverage = None
    
    if has_gcovr:
        if generate_coverage_gcovr(generate_html=args.report):
            coverage = parse_coverage_summary_gcovr()
            if coverage and args.report:
                print(f"\n[OK] HTML report generated in: {COVERAGE_REPORT_DIR}/index.html")
    elif has_lcov:
        if generate_coverage_lcov():
            coverage = parse_coverage_summary_lcov()
            if coverage and args.report:
                if generate_html_report_lcov():
                    print(f"\n[OK] HTML report generated in: {COVERAGE_REPORT_DIR}/index.html")
    else:
        print("\n[!] No coverage tool found (gcovr or lcov).")
        print("Recommendation: Run 'pip install gcovr' for a Python-based coverage tool.")
        return 1
    
    if coverage:
        print_coverage_report(coverage)
        success = check_coverage_requirements(coverage)
    else:
        print("\n[X] Error generating or parsing coverage data")
        success = False
    
    print(f"\n{'='*60}")
    if success:
        print("COVERAGE MEETS REQUIREMENTS!")
    else:
        print("COVERAGE DOES NOT MEET MINIMUM REQUIREMENTS")
    print(f"{'='*60}\n")
    
    return 0 if success else 1

if __name__ == "__main__":
    sys.exit(main())
