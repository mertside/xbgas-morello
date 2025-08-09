# xBGAS-Morello TTU Memory Safety Tests

**CHERI-Morello memory safety evaluation with 17 core security vulnerability tests plus 2 baseline comparison tests (19 total).**

## Quick Start

```bash
make run-all    # Build and execute all tests
make analyze    # Analyze results with CHERI interpretation
make summary    # Quick protection rate summary
```

## Test Categories

- **Spatial Safety (5)**: Buffer overflows, bounds violations, illegal memory access  
- **Temporal Safety (7)**: Use-after-free, double-free, heap manipulation
- **Real-World (5)**: HeartBleed, ROP/DOP, code reuse attacks

## Key Targets

```bash
# Build by category
make spatial temporal realworld

# Execute tests
make run-all run-spatial run-temporal run-realworld

# Analysis & debugging
make analyze summary show-failures debug-reports

# Utilities
make clean check-environment help
```

## CHERI Result Interpretation

**CRITICAL**: Memory safety testing inverts normal success/failure:

- ✅ **`Killed`** / **`In-address space security exception`** = **SUCCESS**
  - CHERI caught the memory safety violation
- ⚠️ **Normal completion** = **Potential vulnerability**
  - Attack may not have been detected

## Files

```
TTU/
├── Makefile           # Main build system
├── README.md          # This file
├── ttu_*.c           # 17 core tests + 2 baseline tests (19 total)
└── results_*.txt     # Historical results
```

## Dependencies

- CHERI-capable compiler (`cc`/`gcc`/`clang`)
- Runtime headers: `../../runtime/xbrtime_morello.h`
- Assembly source: `../../runtime/xbMrtime_api_asm.s`
- BSD make or GNU make

## Test List

### Spatial Safety
- `ttu_s1_free_not_at_start` - Offset pointer free
- `ttu_s2_free_not_on_heap` - Stack/global memory free
- `ttu_s3_null_ptr_dereference` - NULL pointer access
- `ttu_s4_oob_read` - Out-of-bounds read
- `ttu_s5_oob_write` - Out-of-bounds write
- `ttu_s4_baseline_oob_read` - Baseline OOB read comparison
- `ttu_s5_baseline_oob_write` - Baseline OOB write comparison

### Temporal Safety
- `ttu_t1_double_free` - Double free vulnerability
- `ttu_t2_hm_fake_chunk_malloc` - Fake heap chunk
- `ttu_t3_hm_house_of_spirit` - House of Spirit attack
- `ttu_t4_hm_p_and_c_chunk` - Parent/child chunk exploit
- `ttu_t5_use_after_free` - Use-after-free
- `ttu_t6_uaf_function_pointer` - Function pointer UAF
- `ttu_t7_uaf_memcpy` - UAF with memcpy

### Real-World
- `ttu_r1_HeartBleed` - OpenSSL HeartBleed
- `ttu_r2_dop` - Data-oriented programming
- `ttu_r3_uaf_to_code_reuse` - UAF to code reuse
- `ttu_r4_illegal_ptr_deref` - Illegal pointer dereference
- `ttu_r5_df_switch` - Double-free in switch

## Expected Protection Rates

- **Spatial**: 80-100% (bounds checking effective)
- **Temporal**: 60-80% (heap attacks harder to detect)
- **Real-World**: 70-90% (varies by attack complexity)

## Troubleshooting

```bash
make check-environment  # Verify build setup
make debug-reports     # Check report generation
make find-all-reports  # Locate test logs
```

---
*CHERI-Morello Memory Safety Research Framework*

Total: 17 refactored security tests with comprehensive automation.
