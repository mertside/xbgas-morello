#!/bin/sh
#
# Test all temporal safety tests after fixes
#

echo "=== Testing All Temporal Safety Test Fixes ==="

echo "Building all temporal tests..."
make -f Makefile_objaware temporal

if [ $? -eq 0 ]; then
    echo ""
    echo "✅ SUCCESS: All temporal tests built successfully!"
    echo ""
    echo "Continuing with full build..."
    make -f Makefile_objaware all
else
    echo "❌ Some temporal tests failed to build"
fi

echo "Done."
