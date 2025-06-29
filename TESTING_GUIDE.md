# xBGAS-Morello Testing Guide

This guide provides step-by-step instructions for testing the refactored xBGAS runtime codebase for CHERI-Morello. The testing process is organized into different categories to ensure comprehensive validation of the refactored code.

## Overview

The testing process is divided into four main categories:
1. **Build Verification** - Ensure all components compile correctly
2. **Runtime Benchmarks** - Test core xBGAS runtime functionality
3. **Security Tests** - Validate CHERI capability enforcement
4. **Regression Testing** - Compare results with original implementation

## Prerequisites

Before running tests, ensure you have:
- CHERI-Morello toolchain installed
- Access to Morello hardware or emulator
- Basic build tools (make, gcc/clang)
- Sufficient privileges for memory testing

## 1. Build Verification

### 1.1 Test Runtime Headers

First, verify that the refactored headers compile correctly:

```bash
# Navigate to the runtime directory
cd /Users/MertSide/Developer/GitProjects/xBGAS/xbgas-morello/runtime

# Test header compilation (create a simple test file)
cat > header_test.c << 'EOF'
#include "xbrtime_common.h"
#include "xbrtime_api.h"
#include "xbrtime_internal.h"
#include "xbMrtime-types.h"
#include "xbMrtime-macros.h"
#include "test.h"

int main() {
    printf("All headers compiled successfully!\n");
    return 0;
}
EOF

# Compile the header test
cc -g -O2 -Wall -I. -o header_test.exe header_test.c
./header_test.exe

# Clean up
rm header_test.c header_test.exe
```

**Expected Result:** Should compile without errors and print success message.

### 1.2 Test Macro Definitions

Verify that the refactored macros work correctly:

```bash
cd /Users/MertSide/Developer/GitProjects/xBGAS/xbgas-morello/runtime

# Create macro test
cat > macro_test.c << 'EOF'
#include "xbMrtime-macros.h"
#include "test.h"

int main() {
    // Test memory alignment macros
    void *ptr = malloc(100);
    if (ptr) {
        printf("Memory allocation: %p\n", ptr);
        printf("Is aligned (8): %s\n", IS_ALIGNED((uintptr_t)ptr, 8) ? "Yes" : "No");
        free(ptr);
    }
    
    // Test utility macros
    printf("Array size test: %zu\n", ARRAY_SIZE((int[]){1,2,3,4,5}));
    printf("Min/Max test: min(5,3)=%d, max(5,3)=%d\n", MIN(5,3), MAX(5,3));
    
    TEST_LOG("Macro test completed successfully");
    return 0;
}
EOF

cc -g -O2 -Wall -I. -o macro_test.exe macro_test.c
./macro_test.exe

# Clean up
rm macro_test.c macro_test.exe
```

**Expected Result:** Should output memory alignment info, array size, min/max results, and success log.

## 2. Runtime Benchmarks Testing

### 2.1 Build All Benchmarks

```bash
cd /Users/MertSide/Developer/GitProjects/xBGAS/xbgas-morello/bench

# Clean any previous builds
make clean

# Build all benchmarks
make all
```

**Expected Result:** All executables should build successfully:
- `matmul.exe`
- `gather.exe`
- `gups.exe`
- `shmemRandomAccess.exe`
- `shmemRandomAccess_v2.exe`
- `broadcast8.exe`
- `reduction8.exe`

### 2.2 Run Individual Benchmarks

Test each benchmark individually to ensure they work correctly:

```bash
cd /Users/MertSide/Developer/GitProjects/xBGAS/xbgas-morello/bench

# Test matrix multiplication
echo "Testing Matrix Multiplication..."
./matmul.exe

# Test gather operation
echo "Testing Gather Operation..."
./gather.exe

# Test GUPS (Giga Updates Per Second)
echo "Testing GUPS..."
./gups.exe

# Test SHMEM Random Access (both versions)
echo "Testing SHMEM Random Access v1..."
./shmemRandomAccess.exe

echo "Testing SHMEM Random Access v2..."
./shmemRandomAccess_v2.exe

# Test broadcast and reduction
echo "Testing Broadcast..."
./broadcast8.exe

echo "Testing Reduction..."
./reduction8.exe
```

### 2.3 Run All Benchmarks Together

```bash
cd /Users/MertSide/Developer/GitProjects/xBGAS/xbgas-morello/bench

# Run the test target (all benchmarks)
make test
```

**Expected Result:** All benchmarks should run without crashes and produce reasonable performance metrics.

## 3. Security Tests

### 3.1 Build Security Tests

#### TTU Security Tests (Improved)

```bash
cd /Users/MertSide/Developer/GitProjects/xBGAS/xbgas-morello/security/TTU

# If using the improved Makefile
cp Makefile_improved Makefile

# Build all security tests
make all

# Or build by category
make spatial    # Spatial safety tests
make temporal   # Temporal safety tests
make real       # Real-world exploit tests
```

#### ASU Security Tests

```bash
cd /Users/MertSide/Developer/GitProjects/xBGAS/xbgas-morello/security/ASU

# Build all ASU tests
make all

# Or build by category
make spatial    # Spatial safety tests
make temporal   # Temporal safety tests
make real       # Real-world exploit tests
make heap       # Heap manipulation tests
```

### 3.2 Run Security Tests

#### Test the Refactored Examples

```bash
cd /Users/MertSide/Developer/GitProjects/xBGAS/xbgas-morello/security/TTU

# Build and test the refactored out-of-bounds read test
cc -g -O2 -Wall -I../../runtime -lpthread -lm -o ttu_s4_oob_read_refactored.exe ttu_s4_oob_read_refactored.c ../../runtime/xbMrtime_api_asm.s
echo "Testing refactored OOB read (should be caught by CHERI)..."
./ttu_s4_oob_read_refactored.exe

# Build and test the refactored out-of-bounds write test
cc -g -O2 -Wall -I../../runtime -lpthread -lm -o ttu_s5_oob_write_refactored.exe ttu_s5_oob_write_refactored.c ../../runtime/xbMrtime_api_asm.s
echo "Testing refactored OOB write (should be caught by CHERI)..."
./ttu_s5_oob_write_refactored.exe

# Build and test the refactored use-after-free test
cc -g -O2 -Wall -I../../runtime -lpthread -lm -o ttu_t5_use_after_free_refactored.exe ttu_t5_use_after_free_refactored.c ../../runtime/xbMrtime_api_asm.s
echo "Testing refactored use-after-free (should be caught by CHERI)..."
./ttu_t5_use_after_free_refactored.exe

# Build and test the refactored double-free test
cc -g -O2 -Wall -I../../runtime -lpthread -lm -o ttu_t1_double_free_refactored.exe ttu_t1_double_free_refactored.c ../../runtime/xbMrtime_api_asm.s
echo "Testing refactored double-free (should be caught by CHERI)..."
./ttu_t1_double_free_refactored.exe

# Build and test the refactored function pointer UAF test
cc -g -O2 -Wall -I../../runtime -lpthread -lm -o ttu_t6_uaf_function_pointer_refactored.exe ttu_t6_uaf_function_pointer_refactored_fixed.c ../../runtime/xbMrtime_api_asm.s
echo "Testing refactored function pointer UAF (should be caught by CHERI)..."
./ttu_t6_uaf_function_pointer_refactored.exe

# Build and test the refactored memcpy UAF test
cc -g -O2 -Wall -I../../runtime -lpthread -lm -o ttu_t7_uaf_memcpy_refactored.exe ttu_t7_uaf_memcpy_refactored_fixed.c ../../runtime/xbMrtime_api_asm.s
echo "Testing refactored memcpy UAF (should be caught by CHERI)..."
./ttu_t7_uaf_memcpy_refactored.exe
```

**Expected Result:** Should demonstrate CHERI capability violation detection.

#### Run All Security Tests

```bash
# TTU tests
cd /Users/MertSide/Developer/GitProjects/xBGAS/xbgas-morello/security/TTU
./test.sh > test_results_$(date +%Y%m%d).txt

# ASU tests
cd /Users/MertSide/Developer/GitProjects/xBGAS/xbgas-morello/security/ASU
./test.sh > test_results_$(date +%Y%m%d).txt
```

### 3.3 Analyze Security Test Results

```bash
# TTU analysis
cd /Users/MertSide/Developer/GitProjects/xBGAS/xbgas-morello/security/TTU

# If using improved Makefile
make analyze

# Or manual analysis
echo "=== Spatial Safety Test Results ==="
grep -E "(PASS|FAIL|CAUGHT|UNCAUGHT)" test_results_*.txt | grep -E "s[1-5]"

echo "=== Temporal Safety Test Results ==="
grep -E "(PASS|FAIL|CAUGHT|UNCAUGHT)" test_results_*.txt | grep -E "t[1-7]"

echo "=== Real-world Exploit Test Results ==="
grep -E "(PASS|FAIL|CAUGHT|UNCAUGHT)" test_results_*.txt | grep -E "r[1-5]"
```

## 4. Regression Testing

### 4.1 Compare Performance

Create a performance comparison script:

```bash
cd /Users/MertSide/Developer/GitProjects/xBGAS/xbgas-morello

cat > performance_test.sh << 'EOF'
#!/bin/bash

echo "=== xBGAS Performance Regression Test ==="
echo "Date: $(date)"
echo "System: $(uname -a)"
echo

cd bench

echo "Building benchmarks..."
make clean && make all

echo "Running performance tests..."

# Matrix Multiplication
echo "--- Matrix Multiplication ---"
time ./matmul.exe

# GUPS
echo "--- GUPS (Giga Updates Per Second) ---"
time ./gups.exe

# Random Access
echo "--- SHMEM Random Access ---"
time ./shmemRandomAccess.exe

# Broadcast and Reduction
echo "--- Broadcast ---"
time ./broadcast8.exe

echo "--- Reduction ---"
time ./reduction8.exe

echo "=== Performance Test Complete ==="
EOF

chmod +x performance_test.sh
./performance_test.sh > performance_results_$(date +%Y%m%d).txt
```

### 4.2 Compare Security Detection

```bash
cd /Users/MertSide/Developer/GitProjects/xBGAS/xbgas-morello

cat > security_regression.sh << 'EOF'
#!/bin/bash

echo "=== Security Regression Test ==="
echo "Date: $(date)"
echo

# Test both TTU and ASU security suites
echo "Testing TTU security suite..."
cd security/TTU
make clean && make all
./test.sh > ../../ttu_regression_$(date +%Y%m%d).txt

echo "Testing ASU security suite..."
cd ../ASU
make clean && make all
./test.sh > ../../asu_regression_$(date +%Y%m%d).txt

cd ../..

echo "=== Security Regression Summary ==="
echo "TTU Results:"
grep -c "CAUGHT\|PASS" ttu_regression_*.txt
echo "ASU Results:"
grep -c "CAUGHT\|PASS" asu_regression_*.txt

echo "=== Security Regression Test Complete ==="
EOF

chmod +x security_regression.sh
./security_regression.sh
```

## 5. Validation Checklist

After running all tests, verify the following:

### ✅ Build Verification
- [ ] All runtime headers compile without warnings
- [ ] Macro definitions work correctly
- [ ] Type definitions are compatible

### ✅ Runtime Functionality
- [ ] All benchmarks build successfully
- [ ] Matrix multiplication produces correct results
- [ ] GUPS benchmark runs without crashes
- [ ] SHMEM operations work correctly
- [ ] Broadcast and reduction operations function properly

### ✅ Security Enforcement
- [ ] Spatial safety violations are caught
- [ ] Temporal safety violations are detected
- [ ] Use-after-free is prevented
- [ ] Buffer overflows are stopped
- [ ] Double-free is caught

### ✅ Performance
- [ ] No significant performance degradation
- [ ] Memory usage remains reasonable
- [ ] Compilation times are acceptable

### ✅ Code Quality
- [ ] No new compiler warnings
- [ ] Documentation is clear and helpful
- [ ] Code structure is improved
- [ ] Maintainability is enhanced

## 6. Troubleshooting

### Common Issues and Solutions

#### Build Failures
```bash
# Check compiler version
cc --version

# Verify include paths
echo '#include "xbrtime_common.h"' | cc -E -I./runtime -
```

#### Runtime Errors
```bash
# Check for missing dependencies
ldd your_executable

# Run with debugging
gdb ./your_executable
```

#### Security Test Failures
```bash
# Verify CHERI capabilities are enabled
# Check system logs for capability violations
dmesg | grep -i cheri
```

## 7. Continuous Testing

### Automated Testing Script

```bash
cat > run_all_tests.sh << 'EOF'
#!/bin/bash

set -e

echo "=== xBGAS Comprehensive Test Suite ==="
echo "Started: $(date)"

# Build verification
echo "1. Build verification..."
cd runtime
make -f ../test_headers.mk || echo "Header test failed"

# Runtime tests
echo "2. Runtime benchmarks..."
cd ../bench
make clean && make all
make test

# Security tests
echo "3. Security tests..."
cd ../security/TTU
make clean && make all
./test.sh > test_results_auto.txt

cd ../ASU
make clean && make all
./test.sh > test_results_auto.txt

# Summary
echo "=== Test Summary ==="
echo "Completed: $(date)"
echo "Check individual test outputs for details."
EOF

chmod +x run_all_tests.sh
```

This comprehensive testing guide ensures that your refactored xBGAS runtime maintains all functionality while benefiting from improved documentation and code organization. The tests validate both the correctness of the refactoring and the continued security enforcement capabilities of the CHERI-Morello platform.
