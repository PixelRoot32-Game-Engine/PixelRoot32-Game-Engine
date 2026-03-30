#!/usr/bin/env python3
"""
Linux-optimized script for code coverage.
Maintains emojis and standard lcov paths for CI environments.
"""

import subprocess
import sys
import os
import argparse
import re

# Configuration
MIN_LINE_COVERAGE = 80.0
MIN_FUNCTION_COVERAGE = 90.0
COVERAGE_INFO = "coverage.info"
COVERAGE_FILTERED = "coverage_filtered.info"
COVERAGE_REPORT_DIR = "coverage_report"

def run_command(cmd, description):
    """Executes a command and handles errors."""
    print(f"\n{'='*60}")
    print(f"{description}")
    print(f"{'='*60}")
    print(f"Command: {' '.join(cmd)}")
    
    result = subprocess.run(cmd, capture_output=True, text=True)
    
    if result.stdout:
        print(result.stdout)
    if result.stderr and "error" in result.stderr.lower():
        print(f"STDERR: {result.stderr}")
    
    return result.returncode == 0

def run_tests():
    """Runs tests with PlatformIO."""
    print("\n🔍 Running tests...")
    return run_command(
        ["pio", "test", "-e", "native_test"],
        "RUNNING TESTS"
    )

def generate_coverage():
    """Generates coverage data with lcov."""
    print("\n📊 Generating coverage data...")
    
    if not run_command(
        ["lcov", "--capture", "--directory", ".", "--output-file", COVERAGE_INFO],
        "CAPTURING COVERAGE DATA"
    ) :
        return False
    
    if not run_command(
        ["lcov", "--remove", COVERAGE_INFO, 
         "/usr/*", "*/.pio/*", "*/test/*", "*/lib/*",
         "*/src/drivers/native/*", "*/include/drivers/native/*",
         "*/src/core/Engine.cpp", "*/src/graphics/Renderer.cpp", "*/src/physics/TileConsumptionHelper.cpp",
         "*/include/graphics/BaseDrawSurface.h", "*/include/graphics/DrawSurface.h", "*/include/graphics/ui/UICheckbox.h",
         "--output-file", COVERAGE_FILTERED],
        "FILTERING DATA"
    ):
        return False
    
    return True

def parse_coverage_summary():
    """Parses coverage summary."""
    result = subprocess.run(
        ["lcov", "--summary", COVERAGE_FILTERED],
        capture_output=True,
        text=True
    )
    
    output = result.stderr if result.stderr else result.stdout
    
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
    """Generates HTML coverage report."""
    print("\n🌐 Generating HTML report...")
    os.makedirs(COVERAGE_REPORT_DIR, exist_ok=True)
    return run_command(
        ["genhtml", COVERAGE_FILTERED, "--output-directory", COVERAGE_REPORT_DIR],
        "GENERATING HTML REPORT"
    )

def print_coverage_report(coverage):
    """Prints coverage report."""
    print(f"\n{'='*60}")
    print("📈 COVERAGE REPORT")
    print(f"{'='*60}")
    print(f"Line Coverage:      {coverage['lines']:6.2f}%  (minimum: {MIN_LINE_COVERAGE}%)")
    print(f"Function Coverage:  {coverage['functions']:6.2f}%  (minimum: {MIN_FUNCTION_COVERAGE}%)")
    if coverage['branches'] > 0:
        print(f"Branch Coverage:    {coverage['branches']:6.2f}%")
    print(f"{'='*60}")

def check_coverage_requirements(coverage):
    """Verifies that coverage requirements are met."""
    success = True
    if coverage['lines'] < MIN_LINE_COVERAGE:
        print(f"\n❌ Line coverage below minimum ({MIN_LINE_COVERAGE}%)")
        success = False
    else:
        print(f"\n✅ Line coverage OK")
    
    if coverage['functions'] < MIN_FUNCTION_COVERAGE:
        print(f"❌ Function coverage below minimum ({MIN_FUNCTION_COVERAGE}%)")
        success = False
    else:
        print(f"✅ Function coverage OK")
    
    return success

def main():
    parser = argparse.ArgumentParser(description="Code coverage verification (Linux)")
    parser.add_argument("--report", action="store_true", help="Generates HTML report")
    parser.add_argument("--no-tests", action="store_true", help="Does not run tests")
    args = parser.parse_args()
    
    if not args.no_tests:
        if not run_tests():
            print("\n❌ Tests failed")
            return 1
        print("\n✅ Tests passed successfully")
    
    if not generate_coverage():
        print("\n❌ Error generating coverage data")
        return 1
    
    coverage = parse_coverage_summary()
    print_coverage_report(coverage)
    
    if args.report:
        if generate_html_report():
            print(f"\n✅ HTML report generated in: {COVERAGE_REPORT_DIR}/index.html")
    
    success = check_coverage_requirements(coverage)
    
    print(f"\n{'='*60}")
    if success:
        print("🎉 COVERAGE MEETS REQUIREMENTS!")
    else:
        print("⚠️  COVERAGE DOES NOT MEET MINIMUM REQUIREMENTS")
    print(f"{'='*60}\n")
    
    return 0 if success else 1

if __name__ == "__main__":
    sys.exit(main())
