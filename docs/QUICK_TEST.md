# Quick Start Testing Guide

This document provides immediate steps to test the refactored xBGAS-Morello codebase.

## Immediate Validation (5 minutes)

1. **Run the validation script:**
   ```bash
   ./validate_refactoring.sh
   ```
   This checks that all refactored files are in place and compile correctly.

2. **Test headers only:**
   ```bash
   make -f test_headers.mk test-headers
   ```

## Quick Functional Test (10 minutes)

1. **Build and test benchmarks:**
   ```bash
   cd bench
   make clean && make all
   make test
   cd ..
   ```

2. **Test security examples:**
   ```bash
   cd security/TTU
   
   # Test out-of-bounds read (refactored)
   cc -g -O2 -Wall -I../../runtime -lpthread -lm -o test_oob_read.exe ttu_s4_oob_read_refactored.c ../../runtime/xbMrtime_api_asm.s
   ./test_oob_read.exe
   
   # Test out-of-bounds write (refactored)  
   cc -g -O2 -Wall -I../../runtime -lpthread -lm -o test_oob_write.exe ttu_s5_oob_write_refactored.c ../../runtime/xbMrtime_api_asm.s
   ./test_oob_write.exe
   
   # Test use-after-free (refactored)
   cc -g -O2 -Wall -I../../runtime -lpthread -lm -o test_uaf.exe ttu_t5_use_after_free_refactored.c ../../runtime/xbMrtime_api_asm.s
   ./test_uaf.exe
   
   # Test function pointer use-after-free (refactored)
   cc -g -O2 -Wall -I../../runtime -lpthread -lm -o test_func_ptr.exe ttu_t6_uaf_function_pointer_refactored_fixed.c ../../runtime/xbMrtime_api_asm.s
   ./test_func_ptr.exe
   
   # Test memcpy use-after-free (refactored)
   cc -g -O2 -Wall -I../../runtime -lpthread -lm -o test_memcpy_uaf.exe ttu_t7_uaf_memcpy_refactored_fixed.c ../../runtime/xbMrtime_api_asm.s
   ./test_memcpy_uaf.exe
   
   # Test heap manipulation (refactored)
   cc -g -O2 -Wall -I../../runtime -lpthread -lm -o test_heap_manip.exe ttu_t2_hm_fake_chunk_malloc_refactored.c ../../runtime/xbMrtime_api_asm.s
   ./test_heap_manip.exe
   
   # Test HeartBleed vulnerability (refactored)
   cc -g -O2 -Wall -I../../runtime -lpthread -lm -o test_heartbleed.exe ttu_r1_HeartBleed_refactored.c ../../runtime/xbMrtime_api_asm.s
   ./test_heartbleed.exe
   
   cd ../..
   ```

## Full Test Suite (30+ minutes)

Follow the comprehensive `TESTING_GUIDE.md` for complete validation.

## Expected Results

- ✅ All files should be present and compile without errors
- ✅ Benchmarks should run and produce performance metrics
- ✅ Security tests should demonstrate CHERI protection
- ✅ No functionality should be lost from the original implementation

## Troubleshooting

- **Build errors:** Check that you have the CHERI-Morello toolchain installed
- **Missing files:** Ensure all refactored files were created properly
- **Runtime errors:** Verify you're running on compatible hardware/emulator

For detailed testing instructions, see `TESTING_GUIDE.md`.
