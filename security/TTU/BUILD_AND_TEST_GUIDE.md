# xBGAS TTU Memory Safety Tests - Build and Test Guide

## Overview

This document describes the build system for the refactored xBGAS TTU (Texas Tech University) memory safety evaluation tests on CHERI-Morello architecture. The framework provides a clean, portable solution for building and testing the security test suite.

## Current Status

The testing framework has been cleaned up and simplified:
- **Obsolete files removed**: All problematic Makefiles and scripts have been removed
- **Clean build system**: Only working, BSD make compatible files remain
- **Well-documented tests**: All 17 tests have been refactored and documented

## Available Build Files

### Primary Files (Recommended)
- `Makefile_working` - **Canonical build system** (BSD make compatible, POSIX sh)
- `SECURITY_TESTS_REFACTORING.md` - Detailed documentation of all refactored tests

### Legacy Files (Preserved for Reference)
- `Makefile` - Original Makefile
- `test.sh` - Original test script

## Test Categories

### 1. Spatial Safety Tests (5 tests)
- `ttu_s1_free_not_at_start_refactored.c` - Free memory not at allocation start
- `ttu_s2_free_not_on_heap_refactored.c` - Free memory not on heap  
- `ttu_s3_null_ptr_dereference_refactored.c` - Null pointer dereference
- `ttu_s4_oob_read_refactored.c` - Out-of-bounds read access
- `ttu_s5_oob_write_refactored.c` - Out-of-bounds write access

### 2. Temporal Safety Tests (7 tests)
- `ttu_t1_double_free_refactored.c` - Double free vulnerability
- `ttu_t2_hm_fake_chunk_malloc_refactored.c` - Fake chunk malloc heap manipulation
- `ttu_t3_hm_house_of_spirit_refactored.c` - House of Spirit heap manipulation
- `ttu_t4_hm_p_and_c_chunk_refactored.c` - Parent and child chunk heap manipulation
- `ttu_t5_use_after_free_refactored.c` - Use-after-free vulnerability
- `ttu_t6_uaf_function_pointer_refactored_fixed.c` - Use-after-free with function pointers
- `ttu_t7_uaf_memcpy_refactored.c` - Use-after-free with memcpy

### 3. Real-world Vulnerability Tests (5 tests)
- `ttu_r1_HeartBleed_refactored.c` - HeartBleed OpenSSL vulnerability simulation
- `ttu_r2_dop_refactored.c` - Data-oriented programming attack
- `ttu_r3_uaf_to_code_reuse_refactored.c` - Use-after-free to code reuse
- `ttu_r4_illegal_ptr_deref_refactored.c` - Illegal pointer dereference
- `ttu_r5_df_switch_refactored.c` - Double-free with switch statements

## Quick Start

### Clean Up First (Run Once)
```bash
# Remove obsolete files
chmod +x cleanup_obsolete.sh
./cleanup_obsolete.sh
```

### Build All Tests
```bash
# Using the working Makefile
make -f Makefile_working all
```

### Build by Category
```bash
# Spatial safety tests
make -f Makefile_working spatial

# Temporal safety tests  
make -f Makefile_working temporal

# Real-world vulnerability tests
make -f Makefile_working realworld

# Heap manipulation tests only
make -f Makefile_working heap
```

### Build Individual Tests
```bash
# Build specific test
make -f Makefile_working ttu_s1_free_not_at_start_refactored

# Test compilation only (no execution)
make -f Makefile_working compile-all
```

### Clean Up Build Artifacts
```bash
make -f Makefile_working clean
```

## Available Targets

The `Makefile_working` provides these targets:

### Build Targets
- `all` - Build and test all refactored tests
- `spatial` - Build and test spatial safety tests
- `temporal` - Build and test temporal safety tests
- `realworld` - Build and test real-world vulnerability tests
- `heap` - Build and test heap manipulation tests
- `compile-all` - Compile all tests without running them

### Individual Test Targets
Each refactored test can be built individually:
```bash
make -f Makefile_working ttu_s1_free_not_at_start_refactored
make -f Makefile_working ttu_t1_double_free_refactored
make -f Makefile_working ttu_r1_HeartBleed_refactored
# ... etc for all tests
```

### Utility Targets
- `clean` - Remove all build artifacts
- `help` - Display available targets
- `config` - Show build configuration
- `debug` - Show debug information

## Build Configuration

The Makefile uses simple, portable configuration:
```makefile
CC = cc
CFLAGS = -g -O2 -Wall -Wextra -std=c99 -I../../runtime
LDFLAGS = -lpthread -lm
```

## Expected Output

When running tests, you'll see output like:
```
Building and testing ttu_s1_free_not_at_start_refactored...
cc -g -O2 -Wall -Wextra -std=c99 -I../../runtime -lpthread -lm -o ttu_s1_free_not_at_start_refactored ttu_s1_free_not_at_start_refactored.c
Running ttu_s1_free_not_at_start_refactored...
[Test output...]
```

## Troubleshooting

### Build Errors
1. **Missing runtime headers**: Ensure `../../runtime/` contains required headers
2. **Compilation failures**: Check that all `*_refactored.c` files exist
3. **Permission issues**: Ensure files are readable and directory is writable

### Runtime Errors
1. **Missing assembly**: The runtime assembly is optional for most tests
2. **Segmentation faults**: Expected for many security tests (this demonstrates vulnerabilities)
3. **Permission errors**: Some tests may require specific system permissions

## File Structure After Cleanup

```
security/TTU/
├── Makefile                              # Original Makefile
├── Makefile_working                      # Canonical build system ★
├── test.sh                              # Original test script
├── cleanup_obsolete.sh                  # Cleanup script ★
├── BUILD_AND_TEST_GUIDE.md             # This guide ★
├── SECURITY_TESTS_REFACTORING.md       # Test documentation
├── AUTOMATION_SUMMARY.md               # Summary document
├── ttu_s1_free_not_at_start_refactored.c     # Spatial test 1
├── ttu_s2_free_not_on_heap_refactored.c      # Spatial test 2
├── ttu_s3_null_ptr_dereference_refactored.c  # Spatial test 3
├── ttu_s4_oob_read_refactored.c              # Spatial test 4
├── ttu_s5_oob_write_refactored.c             # Spatial test 5
├── ttu_t1_double_free_refactored.c           # Temporal test 1
├── ttu_t2_hm_fake_chunk_malloc_refactored.c  # Temporal test 2
├── ttu_t3_hm_house_of_spirit_refactored.c    # Temporal test 3
├── ttu_t4_hm_p_and_c_chunk_refactored.c      # Temporal test 4
├── ttu_t5_use_after_free_refactored.c        # Temporal test 5
├── ttu_t6_uaf_function_pointer_refactored_fixed.c # Temporal test 6
├── ttu_t7_uaf_memcpy_refactored.c            # Temporal test 7
├── ttu_r1_HeartBleed_refactored.c            # Real-world test 1
├── ttu_r2_dop_refactored.c                   # Real-world test 2
├── ttu_r3_uaf_to_code_reuse_refactored.c     # Real-world test 3
├── ttu_r4_illegal_ptr_deref_refactored.c     # Real-world test 4
├── ttu_r5_df_switch_refactored.c             # Real-world test 5
└── [original test files preserved]
```

## Next Steps

1. Run the cleanup script to remove obsolete files
2. Use `Makefile_working` as your primary build system
3. Refer to `SECURITY_TESTS_REFACTORING.md` for detailed test documentation
4. Report any issues with the simplified build system

## Summary

The TTU memory safety tests have been successfully refactored and automated with:
- ✅ 17 refactored and documented security tests
- ✅ Clean, portable build system (BSD make + POSIX sh compatible)
- ✅ Category-based organization (spatial, temporal, real-world, heap)
- ✅ Comprehensive documentation
- ✅ Obsolete files cleaned up to avoid confusion

The framework is now ready for production use on CHERI-Morello systems.
