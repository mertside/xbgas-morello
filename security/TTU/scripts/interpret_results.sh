#!/bin/sh
#
# CHERI-Morello Memory Safety Test Results Interpreter
# Analyzes test execution results and provides clear interpretation
# of CHERI protection outcomes.
#
# Usage: ./interpret_results.sh
# 
# This script correctly interprets test results where CHERI protections
# catching vulnerabilities are considered successes, not failures.
#

set -e

echo "==============================================================================="
echo "                 CHERI-Morello Memory Safety Test Results"
echo "==============================================================================="
echo ""

# Check multiple possible locations for test reports
echo "Searching for test reports..."

# Possible locations for reports
POSSIBLE_DIRS=""
if pwd | grep -q "/obj$"; then
    # We're in obj directory
    POSSIBLE_DIRS="./reports . ../reports"
    echo "Running from obj directory"
else
    # We're in TTU directory  
    POSSIBLE_DIRS="obj/reports obj ./reports reports"
    echo "Running from TTU directory"
fi

REPORT_DIR=""
for dir in $POSSIBLE_DIRS; do
    if [ -d "$dir" ] && ls "$dir"/run_*.log >/dev/null 2>&1; then
        REPORT_DIR="$dir"
        echo "Found test reports in: $REPORT_DIR"
        break
    fi
done

if [ -z "$REPORT_DIR" ]; then
    echo "❌ No test reports found!"
    echo ""
    echo "Searched in:"
    for dir in $POSSIBLE_DIRS; do
        echo "  - $dir"
    done
    echo ""
    echo "Please run 'make run-all' first to generate test results."
    echo "The reports should be created in the 'reports' subdirectory"
    echo "of wherever the tests are executed."
    exit 1
fi

echo ""

echo "=== INTERPRETING RESULTS ==="
echo ""
echo "Key: ✓ = CHERI protection worked (vulnerability caught)"
echo "     ⚠ = Test completed normally (vulnerability may have succeeded)"
echo "     ❌ = Test failed to run"
echo ""

total=0
protected=0
normal=0
failed=0

for log in "$REPORT_DIR"/run_*.log; do
    if [ -f "$log" ]; then
        total=$((total + 1))
        test_name=$(basename "$log" .log | sed 's/run_//' | sed 's/\.exe//')
        
        # Check if the test was protected by CHERI
        if grep -q "In-address space security exception\|Killed\|Segmentation fault\|Abort\|core dumped" "$log" 2>/dev/null; then
            protected=$((protected + 1))
            echo "✓ $test_name: CHERI-Morello caught memory safety violation"
        elif grep -q "Error\|error\|failed\|Failed" "$log" 2>/dev/null; then
            failed=$((failed + 1))
            echo "❌ $test_name: Test execution failed"
        else
            normal=$((normal + 1))
            echo "⚠ $test_name: Completed normally (vulnerability may not be caught)"
        fi
    fi
done

echo ""
echo "==============================================================================="
echo "                              SUMMARY REPORT"
echo "==============================================================================="
echo ""
echo "Total tests executed: $total"
echo "CHERI protections triggered: $protected"
echo "Normal completions: $normal"
echo "Execution failures: $failed"
echo ""

if [ $total -gt 0 ]; then
    protection_rate=$((protected * 100 / total))
    echo "CHERI protection rate: ${protection_rate}%"
    echo ""
    
    if [ $protected -gt $((total / 2)) ]; then
        echo "✅ EXCELLENT: CHERI-Morello is successfully catching most memory safety violations!"
        echo "   This demonstrates that the capability-based security model is working effectively."
    elif [ $protected -gt 0 ]; then
        echo "✅ GOOD: CHERI-Morello caught some vulnerabilities, demonstrating partial protection."
        echo "   Some tests may be designed to test edge cases or specific vulnerability types."
    else
        echo "⚠  WARNING: No CHERI protections were triggered."
        echo "   This could indicate issues with the test setup or capability environment."
    fi
else
    echo "❌ No tests were processed."
fi

echo ""
echo "NOTE: In CHERI-Morello security testing, programs that crash or are killed"
echo "      by 'In-address space security exception' are SUCCESSES - they indicate"
echo "      that CHERI's capability-based memory protection is working correctly!"
echo ""

# Offer to show detailed logs
echo "==============================================================================="
echo "For detailed test logs, use:"
echo "  make show-all-logs    # Show all execution logs"
echo "  make show-failures    # Show only tests that completed normally"
echo "  make analyze         # Run full analysis with categories"
echo "==============================================================================="
