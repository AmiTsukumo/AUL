#!/bin/bash
set -e

cd /workspaces/AUL

echo "=========================================="
echo "STEP 1: Compiling AUL Interpreter"
echo "=========================================="
g++ -std=c++17 -Wall -Wextra -o aul_final AUL.cpp 2>&1

if [ -f aul_final ]; then
    echo "✅ Compilation SUCCESSFUL"
else
    echo "❌ Compilation FAILED"
    exit 1
fi

echo ""
echo "=========================================="
echo "STEP 2: Running test_simple.aul"
echo "=========================================="
./aul_final test_simple.aul 2>&1 | head -30

echo ""
echo "=========================================="
echo "STEP 3: Running test_all_fixes.aul"
echo "=========================================="
./aul_final test_all_fixes.aul 2>&1 | head -30

echo ""
echo "=========================================="
echo "STEP 4: Running test_error.aul"
echo "=========================================="
./aul_final test_error.aul 2>&1 | head -30

echo ""
echo "=========================================="
echo "STEP 5: Running test_debug.aul"
echo "=========================================="
./aul_final test_debug.aul 2>&1 | head -30
