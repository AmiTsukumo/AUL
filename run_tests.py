#!/usr/bin/env python3
import subprocess
import os
import sys

os.chdir('/workspaces/AUL')

# Compile
print("=== Compiling AUL interpreter ===")
result = subprocess.run(['g++', '-std=c++17', '-o', 'aul_interpreter', 'AUL.cpp'], 
                       capture_output=True, text=True)
if result.returncode != 0:
    print("COMPILATION FAILED")
    print("STDERR:", result.stderr)
    print("STDOUT:", result.stdout)
    sys.exit(1)
else:
    print("Compilation successful!")

# Test 1: test_simple.aul
print("\n=== Testing with test_simple.aul ===")
result = subprocess.run(['./aul_interpreter', 'test_simple.aul'], 
                       capture_output=True, text=True)
print("STDOUT:", result.stdout)
if result.stderr:
    print("STDERR:", result.stderr)
print("Exit code:", result.returncode)

# Test 2: test_error.aul
print("\n=== Testing with test_error.aul ===")
result = subprocess.run(['./aul_interpreter', 'test_error.aul'], 
                       capture_output=True, text=True)
print("STDOUT:", result.stdout)
if result.stderr:
    print("STDERR:", result.stderr)
print("Exit code:", result.returncode)

# Test 3: test_debug.aul
print("\n=== Testing with test_debug.aul ===")
result = subprocess.run(['./aul_interpreter', 'test_debug.aul'], 
                       capture_output=True, text=True)
print("STDOUT:", result.stdout)
if result.stderr:
    print("STDERR:", result.stderr)
print("Exit code:", result.returncode)

print("\n=== All tests completed ===")
