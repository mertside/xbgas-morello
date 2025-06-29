# xBGAS TTU Memory Safety Tests - Automated Testing Documentation

## Overview

This document describes the automated testing framework for the refactored xBGAS TTU (Texas Tech University) memory safety evaluation tests on CHERI-Morello architecture. The framework provides comprehensive automation for building, testing, and analyzing the security test suite.

## Files

### Core Files
- `Makefile_automated` - Advanced Makefile with automated build and test targets
- `run_tests.sh` - Comprehensive test automation script
- `SECURITY_TESTS_REFACTORING.md` - Detailed documentation of refactored tests

### Test Categories

#### 1. Spatial Safety Tests (5 tests)
- `ttu_s1_free_not_at_start_refactored.c` - Free memory not at allocation start
- `ttu_s2_free_not_on_heap_refactored.c` - Free memory not on heap
- `ttu_s3_null_ptr_dereference_refactored.c` - Null pointer dereference
- `ttu_s4_oob_read_refactored.c` - Out-of-bounds read access
- `ttu_s5_oob_write_refactored.c` - Out-of-bounds write access

#### 2. Temporal Safety Tests (7 tests)
- `ttu_t1_double_free_refactored.c` - Double free vulnerability
- `ttu_t2_hm_fake_chunk_malloc_refactored.c` - Fake chunk malloc heap manipulation
- `ttu_t3_hm_house_of_spirit_refactored.c` - House of Spirit heap manipulation
- `ttu_t4_hm_p_and_c_chunk_refactored.c` - Parent and child chunk heap manipulation
- `ttu_t5_use_after_free_refactored.c` - Use-after-free vulnerability
- `ttu_t6_uaf_function_pointer_refactored_fixed.c` - Use-after-free with function pointers
- `ttu_t7_uaf_memcpy_refactored_fixed.c` - Use-after-free with memcpy

#### 3. Real-world Vulnerability Tests (5 tests)
- `ttu_r1_HeartBleed_refactored.c` - HeartBleed OpenSSL vulnerability simulation
- `ttu_r2_dop_refactored.c` - Data-oriented programming attack
- `ttu_r3_uaf_to_code_reuse_refactored.c` - Use-after-free to code reuse
- `ttu_r4_illegal_ptr_deref_refactored.c` - Illegal pointer dereference
- `ttu_r5_df_switch_refactored.c` - Double-free with switch statements

#### 4. Heap Manipulation Subset (3 tests)
Subset of temporal tests focusing specifically on heap manipulation:
- `ttu_t2_hm_fake_chunk_malloc_refactored.c`
- `ttu_t3_hm_house_of_spirit_refactored.c`
- `ttu_t4_hm_p_and_c_chunk_refactored.c`

## Quick Start

### 1. Build All Refactored Tests
```bash
# Using Makefile
make -f Makefile_automated all

# Using automation script
./run_tests.sh build-all
```

### 2. Run All Tests with Comprehensive Reporting
```bash
# Using Makefile
make -f Makefile_automated test-all-refactored

# Using automation script
./run_tests.sh test-all
```

### 3. Test Specific Categories
```bash
# Spatial safety tests
./run_tests.sh test-spatial

# Temporal safety tests  
./run_tests.sh test-temporal

# Real-world vulnerability tests
./run_tests.sh test-realworld

# Heap manipulation tests
./run_tests.sh test-heap
```

## Detailed Usage

### Makefile Targets

#### Build Targets
- `all` - Build all refactored tests (default)
- `spatial-refactored` - Build spatial safety tests
- `temporal-refactored` - Build temporal safety tests
- `realworld-refactored` - Build real-world vulnerability tests
- `heap-refactored` - Build heap manipulation tests

#### Test Execution Targets
- `test-all-refactored` - Run all refactored tests with comprehensive reporting
- `test-spatial-refactored` - Run spatial tests with detailed reporting
- `test-temporal-refactored` - Run temporal tests with detailed reporting
- `test-realworld-refactored` - Run real-world tests with detailed reporting
- `test-heap-refactored` - Run heap tests with detailed reporting

#### Validation Targets
- `validate-build` - Validate all refactored tests compile successfully
- `syntax-check` - Check syntax of all refactored source files
- `generate-comprehensive-report` - Generate detailed test report

#### Maintenance Targets
- `clean` - Remove all generated files
- `clean-reports` - Clean only test reports
- `rebuild` - Clean and rebuild everything

### Automation Script Commands

#### Build Commands
```bash
./run_tests.sh build-all          # Build all refactored tests
./run_tests.sh build-spatial      # Build spatial safety tests
./run_tests.sh build-temporal     # Build temporal safety tests
./run_tests.sh build-realworld    # Build real-world tests
./run_tests.sh build-heap         # Build heap manipulation tests
```

#### Test Commands
```bash
./run_tests.sh test-all           # Run all tests with reporting
./run_tests.sh test-spatial       # Run spatial tests
./run_tests.sh test-temporal      # Run temporal tests
./run_tests.sh test-realworld     # Run real-world tests
./run_tests.sh test-heap          # Run heap tests
```

#### Validation Commands
```bash
./run_tests.sh validate-all       # Validate compilation
./run_tests.sh syntax-check       # Check syntax
./run_tests.sh generate-report    # Generate comprehensive report
./run_tests.sh clean              # Clean generated files
```

#### Script Options
```bash
./run_tests.sh test-all --timeout=60    # Set 60-second timeout
./run_tests.sh test-all --verbose       # Enable verbose output
./run_tests.sh test-all --quiet         # Suppress non-essential output
./run_tests.sh test-all --no-color      # Disable colored output
```

## Output and Reporting

### Report Directory Structure
```
./reports/
├── spatial_test_results.txt           # Spatial test results
├── temporal_test_results.txt          # Temporal test results
├── realworld_test_results.txt         # Real-world test results
├── heap_test_results.txt              # Heap test results
├── comprehensive_test_report.txt      # Combined summary
├── comprehensive_summary.txt          # Detailed comprehensive report
├── build_validation.txt              # Build validation results
├── automation.log                    # Automation script log
├── build_*.log                       # Individual build logs
└── *_output.txt                      # Individual test outputs
```

### Sample Output

#### Successful Test Run
```
=================================================================
xBGAS TTU Memory Safety Tests - Automated Test Runner
Texas Tech University - CHERI-Morello Security Evaluation
=================================================================
Running spatial tests...
  Running ttu_s1_free_not_at_start_refactored.exe...
    ✓ PASSED: ttu_s1_free_not_at_start_refactored.exe
  Running ttu_s2_free_not_on_heap_refactored.exe...
    ✓ PASSED: ttu_s2_free_not_on_heap_refactored.exe
  ...
Spatial Tests Summary:
  Passed: 5/5
  Failed: 0
  Success Rate: 100%
```

#### Build Validation
```
====================================================
Validating Build Process for All Refactored Tests
====================================================
Validating build for ttu_s1_free_not_at_start_refactored.c...
✓ Build successful: ttu_s1_free_not_at_start_refactored.c
...
Build Validation Summary: 17/17 successful, 0 failed
```

## Test Execution Features

### Timeout Protection
- Default 30-second timeout per test
- Configurable via `--timeout=N` option
- Prevents hanging tests from blocking execution

### Comprehensive Logging
- Detailed execution logs in `reports/automation.log`
- Individual test outputs captured
- Build logs for debugging compilation issues

### Error Handling
- Graceful handling of compilation failures
- Test execution failures logged with exit codes
- Timeout detection and reporting

### Color-coded Output
- Green: Success messages
- Red: Error messages
- Yellow: Warning messages
- Blue: Information messages
- Cyan: Category headers
- Purple: Validation messages

## Integration with Build System

### Compiler Configuration
```makefile
CC = cc
CFLAGS = -g -O2 -Wall -Wextra -std=c99
LDFLAGS = -lpthread -lm
INCLUDES = -I../../runtime
ASM_SOURCES = ../../runtime/xbMrtime_api_asm.s
```

### CHERI-Morello Compatibility
Tests are designed to work with both standard GCC and CHERI-Morello LLVM:
```bash
# Standard compilation
CC=cc make -f Makefile_automated all

# CHERI-Morello compilation
CC=/usr/local64/llvm-morello/bin/clang make -f Makefile_automated all
```

## Advanced Usage

### Custom Test Execution
```bash
# Run specific test manually
./ttu_s1_free_not_at_start_refactored.exe

# Run with timeout
timeout 30s ./ttu_s1_free_not_at_start_refactored.exe

# Capture output
./ttu_s1_free_not_at_start_refactored.exe > output.txt 2>&1
```

### Batch Processing
```bash
# Build and test all categories
for category in spatial temporal realworld heap; do
    ./run_tests.sh build-$category
    ./run_tests.sh test-$category
done
```

### Continuous Integration
```bash
#!/bin/bash
# CI script example
set -e
./run_tests.sh validate-all
./run_tests.sh syntax-check  
./run_tests.sh test-all
./run_tests.sh generate-report
```

## Troubleshooting

### Common Issues

#### 1. Compilation Failures
```bash
# Check build logs
cat reports/build_*.log

# Validate syntax
./run_tests.sh syntax-check

# Clean and rebuild
make -f Makefile_automated clean
make -f Makefile_automated all
```

#### 2. Test Execution Failures
```bash
# Check individual test output
cat reports/ttu_*_output.txt

# Run with verbose logging
./run_tests.sh test-all --verbose

# Increase timeout for slow tests
./run_tests.sh test-all --timeout=60
```

#### 3. Permission Issues
```bash
# Make script executable
chmod +x run_tests.sh

# Check directory permissions
ls -la reports/
```

#### 4. Missing Dependencies
```bash
# Check required files
ls -la ../../runtime/xbMrtime_api_asm.s
ls -la Makefile_automated

# Verify compiler
which cc
cc --version
```

## Performance Metrics

### Test Suite Composition
- **Total Refactored Tests**: 17
- **Spatial Safety Tests**: 5 (29.4%)
- **Temporal Safety Tests**: 7 (41.2%)
- **Real-world Vulnerability Tests**: 5 (29.4%)
- **Heap Manipulation Subset**: 3 (17.6%)

### Execution Time Estimates
- **Individual Test**: < 5 seconds (typical)
- **Category Test Suite**: < 30 seconds
- **Full Test Suite**: < 2 minutes
- **Build All Tests**: < 1 minute

## Future Enhancements

### Planned Features
1. **Memory Leak Detection** - Integration with Valgrind
2. **Code Coverage Analysis** - GCOV integration
3. **Performance Benchmarking** - Execution time analysis
4. **CHERI Capability Analysis** - Capability violation detection
5. **Automated Regression Testing** - Git hook integration

### Extensibility
The framework is designed for easy extension:
- Add new test categories by updating source lists
- Extend reporting with custom analysis
- Integration with external testing frameworks
- Custom validation rules and checks

## Conclusion

This automated testing framework provides comprehensive coverage of the refactored xBGAS TTU memory safety tests, enabling efficient development, validation, and analysis of CHERI-Morello security features. The combination of Makefile automation and shell script flexibility ensures robust testing capabilities for ongoing security research and development.
