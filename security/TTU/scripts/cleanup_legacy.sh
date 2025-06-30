#!/bin/sh
#
# Cleanup Legacy Scripts
# Removes old, scattered shell scripts that have been reorganized
# into the scripts/ directory
#

set -e

echo "=== Cleaning up legacy shell scripts ==="
echo ""

# List of old scripts to remove (keeping the new organized ones)
OLD_SCRIPTS="
analyze_results.sh
final_results.sh
test_all_temporal_fixes.sh
test_asm_build.sh
test_run_all_fix.sh
find_reports.sh
interpret_results.sh
"

echo "Scripts to be removed:"
for script in $OLD_SCRIPTS; do
    if [ -f "$script" ]; then
        echo "  ✓ $script (exists)"
    else
        echo "  - $script (not found)"
    fi
done

echo ""
read -p "Remove these old scripts? (y/N): " confirm

if [ "$confirm" = "y" ] || [ "$confirm" = "Y" ]; then
    echo ""
    echo "Removing old scripts..."
    
    for script in $OLD_SCRIPTS; do
        if [ -f "$script" ]; then
            rm -f "$script"
            echo "  ✓ Removed $script"
        fi
    done
    
    echo ""
    echo "✅ Cleanup completed!"
    echo ""
    echo "📁 New organized scripts are in: ./scripts/"
    echo "📖 See scripts/README.md for usage information"
    echo ""
    echo "🚀 Quick start: ./scripts/run_complete_suite.sh"
else
    echo ""
    echo "Cleanup cancelled - old scripts preserved"
fi

echo ""
echo "=== Current script organization ==="
echo ""
echo "📂 scripts/ directory:"
if [ -d "scripts" ]; then
    ls -la scripts/ | grep -E '\.sh$' | sed 's/^/  /'
else
    echo "  (scripts directory not found)"
fi

echo ""
echo "📂 TTU directory (legacy):"
ls -la *.sh 2>/dev/null | sed 's/^/  /' || echo "  (no .sh files)"
