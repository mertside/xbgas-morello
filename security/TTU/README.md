# xBGAS-Morello TTU Memory Safety Tests

**CHERI-Morello capability-based memory safety evaluation suite with 17 security vulnerability tests.**

## Quick Start

```bash
# Build and run all tests
make run-all

# Analyze results
make analyze
```

## Test Categories

**Spatial Safety (5 tests)**: Buffer overflows, bounds violations, illegal memory access  
**Temporal Safety (7 tests)**: Use-after-free, double-free, heap manipulation attacks  
**Real-World (5 tests)**: HeartBleed, ROP/DOP, code reuse, illegal pointer dereference

## Build System

**`Makefile`** - BSD make compatible, obj-directory aware
- Auto-detects execution context (TTU vs obj directory)
- Includes runtime assembly automatically
- Comprehensive error handling and validation

### Core Targets
```bash
# Build
make all spatial temporal realworld

# Execute  
make run-all run-spatial run-temporal run-realworld

# Analysis
make analyze summary show-failures show-all-logs

# Maintenance  
make clean check-environment config help
```

## Result Interpretation

**CRITICAL**: CHERI-Morello inverts traditional success/failure semantics:

- ✅ **SUCCESS**: `Killed`, `In-address space security exception`, `Segmentation fault`
  - CHERI capability system caught the memory safety violation
- ⚠️ **CONCERN**: Normal program completion (exit code 0)
  - Vulnerability may not have been detected

## File Structure

```
TTU/
├── Makefile              # Main build system
├── Makefile_legacy       # Legacy build system backup
├── README.md             # This documentation
├── test_legacy.sh        # Legacy test runner
├── results_*.txt         # Historical test results
└── ttu_*.c              # Test source files (17 tests)
    ├── ttu_s*.c         # Spatial safety tests (5)
    ├── ttu_t*.c         # Temporal safety tests (7) 
    └── ttu_r*.c         # Real-world attack tests (5)
```

## Dependencies

- **Compiler**: cc/gcc/clang with CHERI support
- **Runtime**: `../../runtime/xbrtime_morello.h`, `xbMrtime_api_asm.s`
- **Build**: BSD make or GNU make
- **System**: CHERI-Morello capable platform

## Test Details

### Spatial Safety
- `ttu_s1_free_not_at_start` - Free with offset pointer
- `ttu_s2_free_not_on_heap` - Free stack/global memory  
- `ttu_s3_null_ptr_dereference` - NULL pointer access
- `ttu_s4_oob_read` - Out-of-bounds read
- `ttu_s5_oob_write` - Out-of-bounds write

### Temporal Safety  
- `ttu_t1_double_free` - Double free vulnerability
- `ttu_t2_hm_fake_chunk_malloc` - Heap metadata manipulation
- `ttu_t3_hm_house_of_spirit` - Fake chunk attack
- `ttu_t4_hm_p_and_c_chunk` - Parent/child chunk manipulation
- `ttu_t5_use_after_free` - Use-after-free access
- `ttu_t6_uaf_function_pointer` - Function pointer UAF
- `ttu_t7_uaf_memcpy` - UAF with memcpy

### Real-World Attacks
- `ttu_r1_HeartBleed` - OpenSSL HeartBleed simulation  
- `ttu_r2_dop` - Data-oriented programming
- `ttu_r3_uaf_to_code_reuse` - UAF leading to code reuse
- `ttu_r4_illegal_ptr_deref` - Illegal pointer dereference
- `ttu_r5_df_switch` - Double-free in switch statement

## Expected Results

**CHERI Protection Rates**:
- Spatial: 80-100% (most bounds violations caught)
- Temporal: 60-80% (heap attacks vary)  
- Real-World: 70-90% (depends on attack complexity)

High protection rates indicate effective CHERI capability enforcement.

## Troubleshooting

```bash
make check-environment # Check build environment
make check-files       # Verify source files exist
make debug-reports     # Debug report generation
make find-all-reports  # Locate all test logs
```

**Common Issues**:
- Missing assembly source → Check `runtime/xbMrtime_api_asm.s`
- Build failures → Verify CHERI compiler setup
- No reports found → Run `make debug-reports`

## Architecture

**Build System**: Auto-detecting BSD make with obj directory support  
**Runtime Integration**: Automatic assembly inclusion, header path resolution  
**Result Analysis**: CHERI-aware interpretation with protection rate calculation  
**Test Execution**: Automated build-run-analyze pipeline

---
*xBGAS-Morello TTU Memory Safety Evaluation Suite - CHERI Capability Security Research*

Total: 17 refactored security tests with comprehensive automation.
