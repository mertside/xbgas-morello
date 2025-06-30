#!/bin/sh
#
# Test the run-all target fix
#

echo "=== Testing run-all Target Fix ==="

echo "Testing run-spatial target..."
make -f Makefile_objaware run-spatial

if [ $? -eq 0 ]; then
    echo "✓ run-spatial works!"
    echo ""
    echo "Testing full run-all..."
    make -f Makefile_objaware run-all
else
    echo "✗ run-spatial failed"
fi

echo "Done."
