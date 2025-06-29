# xBGAS TTU Memory Safety Tests

## Quick Start

This directory contains refactored and automated xBGAS TTU memory safety evaluation tests for CHERI-Morello.

### 1. Clean Up (First Time Only)
```bash
chmod +x cleanup_obsolete.sh
./cleanup_obsolete.sh
```

### 2. Build All Tests
```bash
make -f Makefile_bsd all
```

### 3. Build by Category
```bash
make -f Makefile_bsd spatial    # Spatial safety tests
make -f Makefile_bsd temporal   # Temporal safety tests
make -f Makefile_bsd realworld  # Real-world vulnerabilities
make -f Makefile_bsd heap       # Heap manipulation tests
```

## Documentation

- **BUILD_AND_TEST_GUIDE.md** - Complete build and test instructions
- **SECURITY_TESTS_REFACTORING.md** - Detailed test documentation
- **AUTOMATION_SUMMARY_FINAL.md** - Project completion summary

## Test Categories

- **Spatial Safety (5 tests)**: Memory access violations
- **Temporal Safety (7 tests)**: Use-after-free, double-free vulnerabilities  
- **Real-world (5 tests)**: HeartBleed, DOP, code reuse attacks
- **Heap Manipulation (3 tests)**: Advanced heap exploitation techniques

## Files

- `Makefile_bsd` - Canonical build system (BSD make compatible)
- `*_refactored.c` - Modernized, documented test files
- `cleanup_obsolete.sh` - Removes obsolete build files

Total: 17 refactored security tests with comprehensive automation.
