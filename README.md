# xBGAS Memory Safety Evaluation on CHERI-Morello

Porting the xBGAS runtime library onto the CHERI-enabled Morello boards consists of work in four main abstraction layers. 
First, we utilize ARM’s Fixed-Virtual Platform (FVP) ecosystem to emulate a real Morello SoC. 
The FVP provides a functionality-accurate programmer’s view of the hardware platform using the binary translation technology running at speeds comparable to the real hardware. 
Second, we modify the low-level runtime by translating the xBGAS API assembly functions written in RISC-V ISA to ARM ISA. 
Third, we leverage thread pooling on the high-level runtime to model the behavior of multiple processes. 
Finally, we propose minimal changes to the existing xBGAS benchmarks by only requiring the developer to add an entry point and an exit point in their programs to define the code segment that multiple threads can execute in parallel.

This repository provides a comprehensive framework for evaluating memory safety properties of the CHERI-Morello architecture through an adapted xBGAS runtime implementation and a suite of memory safety benchmarks.

## Overview

The project consists of:
1. **xBGAS Runtime Implementation** - A port of the xBGAS runtime library to CHERI-Morello architecture
2. **Memory Safety Benchmark Suite** - A collection of programs demonstrating various memory safety vulnerabilities
3. **Evaluation Framework** - Tools for measuring protection effectiveness and performance impact

Our implementation leverages CHERI's capability-based security model to provide memory safety guarantees while evaluating the performance and security implications.

## Architecture

The xBGAS runtime implementation for CHERI-Morello consists of four main components:

1. **Hardware Abstraction** - Uses ARM's Fixed-Virtual Platform (FVP) to emulate the Morello SoC
2. **Low-level Runtime** - Translated API functions from RISC-V ISA to ARM ISA
3. **High-level Runtime** - Leverages thread pooling to model multi-process behavior
4. **Benchmarking Interface** - Minimally modified xBGAS benchmarks with defined entry/exit points

<img width="742" alt="xbrtime-simple-overview-uml" src="https://github.com/mertside/xbrtime-simple/blob/main/docs/xbrtime-simple-overview-uml.png">

## Memory Safety Benchmark Categories

The repository includes multiple categories of memory safety tests:

### Spatial Safety
- Out-of-bounds read/write
- Buffer overflow
- Null pointer dereference

### Temporal Safety
- Use-after-free
- Double free
- Use after reallocation
- Heap manipulation attacks

### Real-world Vulnerabilities
- HeartBleed-style vulnerabilities
- Data-oriented programming attacks
- Control flow hijacking
- Code reuse attacks

## Directory Structure

```
xbgas-morello/
├── bench/                    # Performance benchmarks and test programs
│   ├── README.md            # Benchmark documentation and usage guide
│   ├── openshmem/           # OpenSHMEM compatibility tests
│   └── gups/               # Global Updates Per Second benchmarks
├── docs/                    # Project documentation and UML diagrams
│   └── README.md           # Documentation index and architecture overview
├── runtime/                 # xBGAS runtime implementation for CHERI-Morello
│   ├── xbrtime_morello.h   # Main runtime header
│   ├── xbMrtime_api_asm.s  # Assembly API functions
│   └── *.h                 # Runtime type definitions and macros
└── security/               # Memory safety evaluation suites
    ├── README.md           # Security suite overview
    ├── TTU/               # Texas Tech University test suite (production-ready)
    │   └── README.md      # TTU test documentation and usage
    ├── ASU/               # Arizona State University test suite
    ├── UoC/               # University of Cambridge test suite  
    └── experimental/      # Experimental vulnerability tests
```

## Quick Start

### Prerequisites

- CHERI-Morello development environment
- ARM Fixed Virtual Platform (FVP) or Morello hardware
- BSD-compatible make
- GCC toolchain with CHERI support

### Building and Running TTU Security Tests

The TTU security test suite is the primary, production-ready evaluation framework:

```bash
# Navigate to the TTU test suite
cd security/TTU

# Build all tests
make all

# Run all tests and generate results
make run

# Build and run specific categories
make spatial          # Spatial safety tests
make temporal         # Temporal safety tests  
make real-world       # Real-world vulnerability tests

# Generate analysis and reports
make analysis         # Analyze CHERI trap results
make summary          # Generate test summary
make logs            # Show detailed build logs
```

### Test Categories

The TTU suite includes 17 core memory safety tests plus 2 baseline comparison tests (19 total):

**Spatial Safety Tests (5 tests + 2 baselines)**
- `ttu_s1_free_not_at_start` - Invalid free() calls
- `ttu_s2_free_not_on_heap` - Free of non-heap memory
- `ttu_s3_null_ptr_dereference` - Null pointer dereference
- `ttu_s4_oob_read` - Out-of-bounds read access
- `ttu_s5_oob_write` - Out-of-bounds write access
- `ttu_s4_baseline_oob_read` - Baseline comparison for OOB read
- `ttu_s5_baseline_oob_write` - Baseline comparison for OOB write

**Temporal Safety Tests (7 tests)**
- `ttu_t1_double_free` - Double free vulnerabilities
- `ttu_t2_hm_fake_chunk_malloc` - Heap metadata manipulation
- `ttu_t3_hm_house_of_spirit` - House of Spirit attack
- `ttu_t4_hm_p_and_c_chunk` - Parent/child chunk manipulation
- `ttu_t5_use_after_free` - Use-after-free vulnerabilities
- `ttu_t6_uaf_function_pointer` - UAF with function pointers
- `ttu_t7_uaf_memcpy` - UAF in memory operations

**Real-world Vulnerability Tests (5 tests)**
- `ttu_r1_HeartBleed` - HeartBleed-style buffer over-read
- `ttu_r2_dop` - Data-oriented programming attacks
- `ttu_r3_uaf_to_code_reuse` - UAF leading to code reuse
- `ttu_r4_illegal_ptr_deref` - Illegal pointer dereference
- `ttu_r5_df_switch` - Double-free in switch statements

## Build System Features

The TTU Makefile provides:

- **BSD Make Compatibility** - Works with FreeBSD, NetBSD, OpenBSD make
- **Object Directory Awareness** - Respects `MAKEOBJDIR` and `.OBJDIR`
- **Category-based Building** - Build tests by vulnerability type
- **CHERI-aware Analysis** - Automatic detection of capability violations
- **Comprehensive Reporting** - Detailed logs and summaries
- **Error Handling** - Robust error detection and reporting

## CHERI-Morello Integration

The tests leverage CHERI's capability-based security features:

- **Spatial Safety** - Capability bounds checking prevents buffer overflows
- **Temporal Safety** - Capability revocation prevents use-after-free
- **Control Flow Integrity** - Code capabilities prevent ROP/JOP attacks
- **Pointer Integrity** - Capability tags prevent pointer manipulation

When CHERI protections are active, memory safety violations trigger capability exceptions that are detected and reported by the test framework.

## Performance Benchmarks (`bench/`)

The [`bench/`](bench/) directory contains a comprehensive performance evaluation framework for measuring CHERI-Morello overhead and scalability:

### Benchmark Categories

**Core xBGAS Operations**
- `xbrtime_matmul.c` - Matrix multiplication with distributed memory patterns
- `xbrtime_gather.c` - Gather operations testing remote memory collection  
- `xbrtime_gups.c` - Global Updates Per Second (memory bandwidth intensive)
- `xbrtime_broadcast8.c` - Collective communication patterns
- `xbrtime_reduction8.c` - Reduction operations across processing elements

**SHMEM Compatibility Tests**
- `SHMEMRandomAccess.c` - SHMEM-style random memory access patterns
- `SHMEMRandomAccess_v2.c` - Enhanced version with improved algorithms
- `openshmem/` - Full OpenSHMEM compatibility benchmark suite

**Specialized Performance Tests**  
- `gups/` - HPCC-compliant GUPS benchmarks for standardized comparison

### Key Metrics
- **Memory bandwidth** under CHERI capability protection
- **Thread pool efficiency** with memory safety constraints
- **Remote memory access performance** with capability bounds checking
- **Scalability characteristics** across multiple processing elements

```bash
# Navigate to benchmarks
cd bench/

# Build and run all benchmarks
make all && make test
```

## Contributing

1. Follow the existing code style and documentation standards
2. Add new tests to appropriate categories (spatial/temporal/real-world)
3. Update the Makefile to include new tests
4. Ensure tests work with both CHERI-enabled and baseline configurations
5. Add comprehensive documentation for new features

## Research Context

This framework supports research in:

- **Memory Safety Evaluation** - Quantifying protection effectiveness
- **Performance Analysis** - Measuring CHERI overhead
- **Security Assessment** - Evaluating attack surface reduction
- **Runtime System Design** - Optimizing capability-based systems

## License

This project builds upon various open-source components. See individual directories for specific licensing information.

## Acknowledgments

- ARM for the Morello platform and FVP emulation
- University of Cambridge for CHERI architecture research
- Texas Tech University for the vulnerability test suite development
- xBGAS project contributors, and especially Tactical Computing Labs, for the runtime foundation
