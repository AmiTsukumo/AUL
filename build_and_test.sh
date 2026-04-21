#!/bin/bash
set -e

echo "=== Compiling AUL interpreter ==="
cd /workspaces/AUL
g++ -std=c++17 -o aul_interpreter AUL.cpp

echo "=== Testing with test_simple.aul ==="
./aul_interpreter test_simple.aul

echo ""
echo "=== Testing with test.aul (part 1) ==="
./aul_interpreter test_part1.aul

echo ""
echo "=== Testing error handling with test_error.aul ==="
./aul_interpreter test_error.aul 2>&1

echo ""
echo "=== Running debug tests ==="
./aul_interpreter test_debug.aul

echo ""
echo "=== All tests completed ==="
