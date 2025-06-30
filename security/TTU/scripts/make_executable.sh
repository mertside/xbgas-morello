#!/bin/sh
#
# Make all scripts executable
#

echo "Setting executable permissions on all scripts..."

chmod +x scripts/*.sh

echo "✅ All scripts in scripts/ directory are now executable"
echo ""
echo "📋 Available scripts:"
ls -la scripts/*.sh | awk '{print "  " $9 " (" $1 ")"}'
echo ""
echo "🚀 Quick start: ./scripts/run_complete_suite.sh"
