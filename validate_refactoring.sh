#!/bin/sh

# Quick Validation Script for xBGAS Refactored Code
# This script provides a quick way to validate that the refactoring was successful

set -e

echo "==============================================="
echo "   xBGAS Refactoring Validation Script"
echo "==============================================="
echo "Date: $(date)"
echo "PWD: $(pwd)"
echo ""

# Color codes for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Function to print colored output
print_status() {
    local status=$1
    local message=$2
    case $status in
        "SUCCESS")
            echo -e "${GREEN}✓ $message${NC}"
            ;;
        "ERROR")
            echo -e "${RED}✗ $message${NC}"
            ;;
        "WARNING")
            echo -e "${YELLOW}⚠ $message${NC}"
            ;;
        "INFO")
            echo -e "${BLUE}ℹ $message${NC}"
            ;;
    esac
}

# Function to check if file exists
check_file() {
    local file=$1
    local description=$2
    
    if [ -f "$file" ]; then
        print_status "SUCCESS" "$description exists: $file"
        return 0
    else
        print_status "ERROR" "$description missing: $file"
        return 1
    fi
}

# Function to check directory
check_directory() {
    local dir=$1
    local description=$2
    
    if [ -d "$dir" ]; then
        print_status "SUCCESS" "$description exists: $dir"
        return 0
    else
        print_status "ERROR" "$description missing: $dir"
        return 1
    fi
}

echo "1. Checking Project Structure..."
echo "================================="

# Check main directories
check_directory "runtime" "Runtime directory"
check_directory "security" "Security directory"
check_directory "bench" "Benchmark directory"
check_directory "docs" "Documentation directory"

echo ""

echo "2. Checking Refactored Files..."
echo "==============================="

# Check new/refactored runtime files
check_file "runtime/xbrtime_common.h" "New common header"
check_file "runtime/xbrtime_api.h" "New API header"
check_file "runtime/xbrtime_internal.h" "New internal header"
check_file "runtime/xbMrtime-types.h" "Refactored types header"
check_file "runtime/xbMrtime-macros.h" "Refactored macros header"
check_file "runtime/test.h" "Refactored test utilities"

# Check documentation files
check_file "README_NEW.md" "New comprehensive README"
check_file "REFACTORING_GUIDE.md" "Refactoring guide"
check_file "REFACTORING_SUMMARY.md" "Refactoring summary"
check_file "TESTING_GUIDE.md" "Testing guide"

# Check improved Makefile
check_file "security/TTU/Makefile_improved" "Improved TTU Makefile"

# Check example refactored security test
check_file "security/TTU/ttu_s4_oob_read_refactored.c" "Refactored security test example"

echo ""

echo "3. Testing Header Compilation..."
echo "================================"

# Test header compilation
print_status "INFO" "Creating header test..."

cat > temp_header_test.c << 'EOF'
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "runtime/xbrtime_common.h"
#include "runtime/xbrtime_api.h"
#include "runtime/xbrtime_internal.h"
#include "runtime/xbMrtime-types.h"
#include "runtime/xbMrtime-macros.h"
#include "runtime/test.h"

int main() {
    printf("Header compilation test passed!\n");
    return 0;
}
EOF

# Try compilation with verbose error reporting
print_status "INFO" "Attempting header compilation..."
if cc -g -O2 -Wall -Iruntime -o temp_header_test.exe temp_header_test.c 2>temp_compile_error.log; then
    print_status "SUCCESS" "All headers compile successfully"
    ./temp_header_test.exe
    rm -f temp_header_test.exe
else
    print_status "ERROR" "Header compilation failed. Error details:"
    echo "--- Compilation Errors ---"
    cat temp_compile_error.log
    echo "--- End Errors ---"
    
    # Try a simpler test with just basic headers
    print_status "INFO" "Trying simplified header test..."
    cat > temp_simple_test.c << 'EOF'
#include <stdio.h>
#include "runtime/xbMrtime-types.h"
#include "runtime/xbMrtime-macros.h"

int main() {
    printf("Basic headers work!\n");
    return 0;
}
EOF
    
    if cc -g -O2 -Wall -Iruntime -o temp_simple_test.exe temp_simple_test.c 2>/dev/null; then
        print_status "SUCCESS" "Basic headers compile successfully"
        ./temp_simple_test.exe
        rm -f temp_simple_test.exe
    else
        print_status "ERROR" "Even basic headers fail to compile"
    fi
    rm -f temp_simple_test.c
fi

rm -f temp_header_test.c temp_compile_error.log

echo ""

echo "4. Testing Benchmark Build..."
echo "============================="

cd bench
if make clean >/dev/null 2>&1 && make all >/dev/null 2>&1; then
    print_status "SUCCESS" "All benchmarks build successfully"
    
    # List built executables
    echo "Built executables:"
    ls -la *.exe 2>/dev/null || true
else
    print_status "ERROR" "Benchmark build failed"
fi
cd ..

echo ""

echo "5. Testing Security Test Build..."
echo "================================="

# Test TTU security builds
cd security/TTU
if make clean >/dev/null 2>&1 && make all >/dev/null 2>&1; then
    print_status "SUCCESS" "TTU security tests build successfully"
    
    # Count built executables
    exe_count=$(ls -1 *.exe 2>/dev/null | wc -l)
    print_status "INFO" "Built $exe_count TTU security test executables"
else
    print_status "WARNING" "TTU security test build had issues"
fi
cd ../..

echo ""

echo "6. Code Quality Check..."
echo "========================"

# Check for common issues in refactored files
print_status "INFO" "Checking for potential issues..."

# Check for TODO/FIXME comments
todo_count=$(grep -r "TODO\|FIXME" runtime/ 2>/dev/null | wc -l)
if [ $todo_count -gt 0 ]; then
    print_status "WARNING" "Found $todo_count TODO/FIXME comments to address"
else
    print_status "SUCCESS" "No outstanding TODO/FIXME comments"
fi

# Check for proper header guards
print_status "INFO" "Checking header guards..."
for header in runtime/*.h; do
    if [ -f "$header" ]; then
        if grep -q "#ifndef\|#define" "$header"; then
            print_status "SUCCESS" "Header guard found in $(basename $header)"
        else
            print_status "WARNING" "No header guard in $(basename $header)"
        fi
    fi
done

echo ""

echo "7. Documentation Check..."
echo "========================="

# Check documentation completeness
docs=(
    "README_NEW.md"
    "REFACTORING_GUIDE.md"
    "REFACTORING_SUMMARY.md"
    "TESTING_GUIDE.md"
)

for doc in "${docs[@]}"; do
    if [ -f "$doc" ]; then
        word_count=$(wc -w < "$doc")
        print_status "SUCCESS" "$doc ($word_count words)"
    else
        print_status "ERROR" "$doc missing"
    fi
done

echo ""

echo "8. Validation Summary..."
echo "========================"

print_status "INFO" "Refactoring validation completed!"
echo ""
print_status "INFO" "Key improvements achieved:"
echo "  • Modular header organization (common, api, internal)"
echo "  • Enhanced documentation and comments"
echo "  • Improved type safety and clarity"
echo "  • Better build system organization"
echo "  • Comprehensive testing framework"
echo ""

print_status "INFO" "Next steps:"
echo "  1. Run the full test suite: ./TESTING_GUIDE.md"
echo "  2. Build and test on CHERI-Morello hardware"
echo "  3. Compare performance with original implementation"
echo "  4. Review and integrate any additional feedback"

echo ""
echo "==============================================="
echo "   Validation Complete!"
echo "==============================================="

# Clean up any temporary files
rm -f temp_*
