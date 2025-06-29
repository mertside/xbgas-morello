#!/bin/sh

#
# Simple compilation test script
#

echo "Testing individual compilation of refactored tests..."
echo "=================================================="

# Test basic compilation without assembly
echo "Testing basic compilation (no assembly):"
cc -g -O2 -Wall -Wextra -std=c99 -I../../runtime -lpthread -lm \
   -o test_basic.exe ttu_s1_free_not_at_start_refactored.c 2>&1

if [ $? -eq 0 ]; then
    echo "✓ Basic compilation successful"
    rm -f test_basic.exe
else
    echo "✗ Basic compilation failed"
fi

# Test compilation with assembly (if available)
if [ -f "../../runtime/xbMrtime_api_asm.s" ]; then
    echo "Testing compilation with assembly:"
    cc -g -O2 -Wall -Wextra -std=c99 -I../../runtime -lpthread -lm \
       ../../runtime/xbMrtime_api_asm.s \
       -o test_asm.exe ttu_s1_free_not_at_start_refactored.c 2>&1
    
    if [ $? -eq 0 ]; then
        echo "✓ Assembly compilation successful"
        rm -f test_asm.exe
    else
        echo "✗ Assembly compilation failed"
    fi
else
    echo "⚠ Assembly source not found"
fi

# Test if source file exists and is readable
echo "Checking source files:"
for src in ttu_s1_free_not_at_start_refactored.c ttu_s4_oob_read_refactored.c ttu_t1_double_free_refactored.c; do
    if [ -f "$src" ]; then
        echo "✓ Found: $src"
    else
        echo "✗ Missing: $src"
    fi
done

# Test runtime directory
echo "Checking runtime directory:"
if [ -d "../../runtime" ]; then
    echo "✓ Runtime directory exists"
    if [ -f "../../runtime/xbMrtime_api_asm.s" ]; then
        echo "✓ Assembly source exists"
    else
        echo "⚠ Assembly source missing"
    fi
else
    echo "✗ Runtime directory missing"
fi

echo "=================================================="
echo "Test completed"
