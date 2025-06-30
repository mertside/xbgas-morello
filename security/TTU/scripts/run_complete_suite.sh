#!/bin/sh
#
# Complete Test Suite Runner
# Builds and executes all tests with comprehensive reporting
#
# Usage: ./run_complete_suite.sh
#

set -e

echo "==============================================================================="
echo "🚀 xBGAS-Morello TTU Complete Security Test Suite"
echo "==============================================================================="
echo ""

# Clean start
echo "🧹 Cleaning previous builds..."
make -f Makefile_objaware clean >/dev/null 2>&1

echo ""
echo "🔨 Building all test categories..."
echo "==================================="
make -f Makefile_objaware all

echo ""
echo "🧪 Executing comprehensive test suite..."
echo "========================================"
make -f Makefile_objaware run-all

echo ""
echo "📊 Analyzing results..."
echo "======================="
make -f Makefile_objaware analyze

echo ""
echo "==============================================================================="
echo "🎯 Quick Analysis with Interpretation Script"
echo "==============================================================================="

# Try to run the interpretation script from multiple locations
if [ -f "scripts/interpret_results.sh" ]; then
    chmod +x scripts/interpret_results.sh
    scripts/interpret_results.sh
elif [ -f "interpret_results.sh" ]; then
    chmod +x interpret_results.sh
    ./interpret_results.sh
else
    echo "Interpretation script not found - using make summary instead"
    make -f Makefile_objaware summary
fi

echo ""
echo "==============================================================================="
echo "✅ Complete test suite execution finished!"
echo ""
echo "📋 Additional commands:"
echo "  make show-failures   # Show tests that completed normally"
echo "  make show-all-logs   # Show all test execution logs"
echo "  make summary        # Quick results summary"
echo "==============================================================================="
