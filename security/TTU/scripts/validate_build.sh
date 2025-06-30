#!/bin/sh
#
# Build Validation and Test Script
# Validates that the build system works correctly and tests are properly compiled
#
# Usage: ./validate_build.sh
#

set -e

echo "============================================================"
echo "🔨 xBGAS-Morello TTU Build System Validation"
echo "============================================================"
echo ""

# Check environment first
echo "📋 Environment Check:"
echo "---------------------"
make -f Makefile_objaware check-environment

echo ""
echo "🔍 File Validation:"
echo "-------------------"
make -f Makefile_objaware check-files

echo ""
echo "⚙️  Configuration Test:"
echo "-----------------------"
make -f Makefile_objaware config

echo ""
echo "🧪 Compilation Test:"
echo "--------------------"
make -f Makefile_objaware compile-test

echo ""
echo "🏗️  Category Build Tests:"
echo "-------------------------"

echo "Testing spatial safety builds..."
if make -f Makefile_objaware spatial >/dev/null 2>&1; then
    echo "✓ Spatial tests build successfully"
else
    echo "✗ Spatial tests build failed"
    exit 1
fi

echo "Testing temporal safety builds..."
if make -f Makefile_objaware temporal >/dev/null 2>&1; then
    echo "✓ Temporal tests build successfully"
else
    echo "✗ Temporal tests build failed"
    exit 1
fi

echo "Testing real-world attack builds..."
if make -f Makefile_objaware realworld >/dev/null 2>&1; then
    echo "✓ Real-world tests build successfully"
else
    echo "✗ Real-world tests build failed"
    exit 1
fi

echo ""
echo "🎯 Full Build Test:"
echo "-------------------"
if make -f Makefile_objaware all >/dev/null 2>&1; then
    echo "✅ Complete build successful!"
    
    # Count built executables
    exe_count=$(ls -1 *.exe 2>/dev/null | wc -l | tr -d ' ')
    echo "Built $exe_count test executables"
    
    if [ "$exe_count" -ge 15 ]; then
        echo "✅ Expected number of tests built"
    else
        echo "⚠ Fewer tests built than expected (expected ~17)"
    fi
else
    echo "❌ Complete build failed"
    exit 1
fi

echo ""
echo "============================================================"
echo "✅ Build system validation completed successfully!"
echo "Run 'make run-all' to execute the test suite."
echo "============================================================"
