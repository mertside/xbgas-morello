#!/bin/sh
#
# Quick test to validate assembly source inclusion
#

echo "=== Testing Assembly Source Inclusion ==="
echo "Current directory: $(pwd)"

# Test from TTU directory
echo "\n--- Test from TTU directory ---"
make -f Makefile_objaware config | grep -E "(COMPILE|ASM_SOURCE)"
make -f Makefile_objaware check-environment | grep -E "(Assembly|WARNING)"

# Test compile of one file to see if assembly is included
echo "\n--- Test compilation with assembly ---"
make -f Makefile_objaware compile-test

echo "\nDone."
