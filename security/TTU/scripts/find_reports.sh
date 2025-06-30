#!/bin/sh
#
# Report Discovery Utility
# Helps locate test execution reports in various possible locations
#
# Usage: ./find_reports.sh
#

set -e

echo "=== Test Report Location Finder ==="
echo ""
echo "Current working directory: $(pwd)"
echo ""

echo "=== Searching for run_*.log files ==="

# Search in current directory and subdirectories
echo "Checking current directory..."
if ls run_*.log >/dev/null 2>&1; then
    echo "  ✓ Found run_*.log files in current directory:"
    ls -la run_*.log | sed 's/^/    /'
else
    echo "  - No run_*.log files in current directory"
fi

echo ""
echo "Checking reports subdirectory..."
if [ -d "reports" ] && ls reports/run_*.log >/dev/null 2>&1; then
    echo "  ✓ Found run_*.log files in ./reports/:"
    ls -la reports/run_*.log | sed 's/^/    /'
else
    echo "  - No run_*.log files in ./reports/"
fi

echo ""
echo "Checking obj subdirectory..."
if [ -d "obj" ]; then
    echo "  obj directory exists"
    if ls obj/run_*.log >/dev/null 2>&1; then
        echo "  ✓ Found run_*.log files in ./obj/:"
        ls -la obj/run_*.log | sed 's/^/    /'
    else
        echo "  - No run_*.log files in ./obj/"
    fi
    
    if [ -d "obj/reports" ] && ls obj/reports/run_*.log >/dev/null 2>&1; then
        echo "  ✓ Found run_*.log files in ./obj/reports/:"
        ls -la obj/reports/run_*.log | sed 's/^/    /'
    else
        echo "  - No run_*.log files in ./obj/reports/"
    fi
else
    echo "  - obj directory doesn't exist"
fi

echo ""
echo "Checking parent directory..."
if ls ../run_*.log >/dev/null 2>&1; then
    echo "  ✓ Found run_*.log files in parent directory:"
    ls -la ../run_*.log | sed 's/^/    /'
else
    echo "  - No run_*.log files in parent directory"
fi

echo ""
echo "=== Finding all run_*.log files recursively ==="
find . -name "run_*.log" -type f 2>/dev/null | while read file; do
    echo "  Found: $file"
    echo "    Size: $(ls -lh "$file" | awk '{print $5}')"
    echo "    Modified: $(ls -l "$file" | awk '{print $6, $7, $8}')"
done

echo ""
echo "=== Checking for .exe files (compiled tests) ==="
if ls *.exe >/dev/null 2>&1; then
    echo "  ✓ Found .exe files in current directory:"
    ls -la *.exe | sed 's/^/    /'
else
    echo "  - No .exe files in current directory"
fi

if [ -d "obj" ] && ls obj/*.exe >/dev/null 2>&1; then
    echo "  ✓ Found .exe files in ./obj/:"
    ls -la obj/*.exe | sed 's/^/    /'
else
    echo "  - No .exe files in ./obj/"
fi

echo ""
echo "=== Recommendations ==="
echo "1. Run this script from both TTU and TTU/obj directories"
echo "2. Run 'make run-all' if no reports are found"
echo "3. Check that tests are actually executing and creating logs"
