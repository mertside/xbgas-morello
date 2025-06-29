#!/bin/sh
#
# Debug script to check directory structure and make behavior
#

echo "=== Directory Investigation ==="
echo "Current shell directory: $(pwd)"
echo "Physical directory (no symlinks): $(pwd -P)"
echo ""

echo "=== Directory Contents ==="
echo "Files in current directory:"
ls -la | head -10
echo ""

echo "=== Looking for source files ==="
echo "Refactored files in current directory:"
ls -1 *_refactored*.c 2>/dev/null || echo "None found"
echo ""

echo "=== Check for obj subdirectory ==="
if [ -d "obj" ]; then
    echo "obj directory exists"
    echo "Contents of obj:"
    ls -la obj/ | head -5
else
    echo "No obj directory found"
fi
echo ""

echo "=== Makefile location ==="
if [ -f "Makefile_universal" ]; then
    echo "✓ Makefile_universal exists in current directory"
else
    echo "✗ Makefile_universal not found in current directory"
fi
echo ""

echo "=== Test make working directory ==="
echo "Testing what directory make sees:"
make --version | head -1 2>/dev/null || echo "make version unavailable"
make -f /dev/null config 2>/dev/null || echo "Make test completed"
