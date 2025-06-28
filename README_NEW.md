# xBGAS Memory Safety Evaluation on CHERI-Morello

[![License](https://img.shields.io/badge/License-Proprietary-red.svg)](LICENSE)
[![Platform](https://img.shields.io/badge/Platform-CHERI--Morello-blue.svg)](https://www.morello-project.org/)
[![Architecture](https://img.shields.io/badge/Architecture-ARM64-green.svg)](https://developer.arm.com/architectures/cpu-architecture)

## Overview

This repository provides a comprehensive framework for evaluating memory safety properties of the CHERI-Morello architecture through an adapted xBGAS (eXtended Base Global Address Space) runtime implementation and a suite of memory safety benchmarks.

The project demonstrates how CHERI's capability-based security model can be leveraged to provide memory safety guarantees while evaluating performance and security implications in a distributed computing environment.

### Key Features

- **Complete xBGAS Runtime Port**: Full implementation of the xBGAS runtime library adapted for CHERI-Morello
- **Memory Safety Benchmark Suite**: Comprehensive collection of programs demonstrating various memory safety vulnerabilities
- **Performance Evaluation Tools**: Framework for measuring protection effectiveness and performance impact
- **Thread-based PE Simulation**: Thread pooling to model multi-processing element behavior
- **CHERI Integration**: Leverages CHERI capabilities for enhanced memory safety

## Architecture

The xBGAS runtime implementation for CHERI-Morello consists of four main abstraction layers:

### 1. Hardware Abstraction Layer
- **ARM Fixed-Virtual Platform (FVP)**: Emulates the Morello SoC with functionality-accurate programmer's view
- **Binary Translation**: Provides speeds comparable to real hardware using advanced translation technology
- **CHERI Capability Support**: Full integration with CHERI's capability-based memory protection

### 2. Low-level Runtime Layer
- **ISA Translation**: xBGAS API assembly functions translated from RISC-V ISA to ARM ISA
- **Memory Operations**: Optimized assembly routines for data transfer operations
- **Hardware Integration**: Direct interface with Morello-specific features

### 3. High-level Runtime Layer
- **Thread Pool Management**: Models multi-processing element behavior using pthread-based thread pools
- **Synchronization Primitives**: Barrier operations and collective communications
- **Memory Management**: CHERI-aware allocation and deallocation routines
- **Error Handling**: Comprehensive error detection and reporting

### 4. Application Interface Layer
- **Minimal Code Changes**: Requires only entry/exit point definitions in existing xBGAS benchmarks
- **API Compatibility**: Maintains compatibility with existing xBGAS applications
- **Performance Monitoring**: Built-in timing and statistics collection

## Memory Safety Benchmark Categories

The repository includes multiple categories of memory safety tests organized into three main classifications:

### Spatial Safety Violations
Programs that demonstrate violations of spatial memory safety:

- **Out-of-bounds Read/Write** (`*_oob_read.c`, `*_oob_write.c`)
  - Buffer overflow scenarios
  - Array bounds violations
  - Heap and stack overflow conditions

- **Buffer Overflow** (`*_buffer_overflow.c`)
  - Classic buffer overflow vulnerabilities
  - Stack-based overflows
  - Heap-based overflows

- **Null Pointer Dereference** (`*_null_ptr_dereference.c`)
  - Null pointer access violations
  - Uninitialized pointer usage

### Temporal Safety Violations
Programs that demonstrate violations of temporal memory safety:

- **Use-After-Free** (`*_use_after_free.c`, `*_uaf_*.c`)
  - Access to deallocated memory
  - Dangling pointer dereferences
  - Function pointer use-after-free

- **Double Free** (`*_double_free.c`)
  - Multiple deallocation of same memory block
  - Memory management corruption

- **Use After Reallocation** (`*_use_after_realloc.c`)
  - Access to memory after reallocation
  - Stale pointer usage

- **Heap Manipulation** (`*_hm_*.c`)
  - Heap metadata corruption
  - Fake chunk attacks
  - House of spirit attacks

### Real-world Vulnerability Simulations
Programs that simulate actual security vulnerabilities:

- **HeartBleed-style** (`*_heartbleed.c`)
  - Out-of-bounds read with length input
  - Information disclosure vulnerabilities

- **Data-Oriented Programming** (`*_dop.c`)
  - Data corruption attacks
  - Control flow manipulation through data

- **Control Flow Hijacking** (`*_control_flow.c`)
  - Return address overwriting
  - Function pointer manipulation

- **Code Reuse Attacks** (`*_rop.c`, `*_code_reuse.c`)
  - Return-oriented programming
  - Jump-oriented programming

## Directory Structure

```
├── runtime/                    # xBGAS Runtime Implementation
│   ├── xbrtime_api.h          # Public API declarations
│   ├── xbrtime_internal.h     # Internal data structures
│   ├── xbrtime_common.h       # Common definitions
│   ├── xbrtime_morello.h      # Main implementation (legacy)
│   ├── xbMrtime-types.h       # Type definitions
│   ├── xbMrtime-macros.h      # Configuration macros
│   ├── xbMrtime-alloc.h       # Memory allocation
│   ├── xbMrtime_api_asm.s     # Assembly routines
│   ├── threadpool.h           # Thread pool management
│   └── test.h                 # Testing utilities
├── bench/                      # Performance Benchmarks
│   ├── xbrtime_matmul.c       # Matrix multiplication
│   ├── xbrtime_gather.c       # Gather operations
│   ├── xbrtime_gups.c         # GUPS benchmark
│   └── ...
├── security/                   # Memory Safety Tests
│   ├── ASU/                   # Arizona State University tests
│   ├── TTU/                   # Texas Tech University tests
│   ├── UoC/                   # University of Cambridge tests
│   └── experimental/          # Experimental tests
└── docs/                      # Documentation
```

## Building and Running

### Prerequisites

- CHERI-Morello development environment
- ARM Fixed-Virtual Platform (FVP)
- GCC or Clang with CHERI support
- pthread library

### Environment Setup

```bash
# Set the number of processing elements (threads)
export NUM_OF_THREADS=4

# Optional: Enable debug output
export XBGAS_DEBUG=1
export XBGAS_PRINT=1
```

### Building Runtime

```bash
cd runtime/
make all
```

### Building Benchmarks

```bash
cd bench/
make all
```

### Building Security Tests

```bash
cd security/ASU/
make all
./test.sh

cd ../TTU/
make all
./test.sh
```

### Running Examples

```bash
# Run matrix multiplication benchmark
cd bench/
./matmul.exe

# Run memory safety tests
cd security/ASU/
./runAll  # Runs all ASU security tests

cd ../TTU/
./runAll  # Runs all TTU security tests
```

## API Usage Example

```c
#include "xbrtime_api.h"

int main() {
    // Initialize the runtime
    if (xbrtime_init() != 0) {
        fprintf(stderr, "Failed to initialize xBGAS runtime\n");
        return -1;
    }
    
    // Get PE information
    int my_pe = xbrtime_mype();
    int num_pes = xbrtime_num_pes();
    
    printf("PE %d of %d\n", my_pe, num_pes);
    
    // Allocate shared memory
    size_t size = 1024 * sizeof(int);
    int *shared_data = (int*)xbrtime_malloc(size);
    
    if (shared_data == NULL) {
        fprintf(stderr, "Failed to allocate shared memory\n");
        xbrtime_close();
        return -1;
    }
    
    // Initialize data
    for (int i = 0; i < 1024; i++) {
        shared_data[i] = my_pe * 1000 + i;
    }
    
    // Synchronize all PEs
    xbrtime_barrier();
    
    // Perform collective operation (broadcast from PE 0)
    if (my_pe == 0) {
        printf("Broadcasting data from PE 0\n");
    }
    
    int broadcast_data = 42;
    xbrtime_int_broadcast(&broadcast_data, &broadcast_data, 1, 1, 0);
    
    printf("PE %d received broadcast data: %d\n", my_pe, broadcast_data);
    
    // Clean up
    xbrtime_free(shared_data);
    xbrtime_close();
    
    return 0;
}
```

## Performance Characteristics

The xBGAS runtime on CHERI-Morello provides:

- **Memory Safety**: CHERI capabilities prevent spatial and temporal memory safety violations
- **Performance Overhead**: Typically 5-15% overhead for capability checking
- **Scalability**: Supports up to 16 processing elements (configurable)
- **Compatibility**: Binary compatibility with existing xBGAS applications

## Research Applications

This framework has been used for research in:

- Memory safety evaluation on capability architectures
- Performance analysis of CHERI-based systems
- Distributed computing security
- Hardware-software co-design for memory safety

## Contributing

Contributions are welcome! Please ensure that:

1. Code follows the established style and documentation standards
2. New security tests include comprehensive comments explaining the vulnerability
3. Performance benchmarks include timing and statistical analysis
4. All changes maintain API compatibility

## License

This project is proprietary software. See the LICENSE file for details.

## Citations

If you use this work in your research, please cite:

```bibtex
@inproceedings{xbgas-morello-2024,
  title={Memory Safety Evaluation of xBGAS Runtime on CHERI-Morello},
  author={Mert Side and others},
  booktitle={Proceedings of CCGrid 2025},
  year={2024},
  organization={IEEE}
}
```

## Contact

For questions or support, please contact:
- Mert Side - Texas Tech University
- STAM Center - Arizona State University

## Acknowledgments

- Tactical Computing Laboratories for the original xBGAS runtime
- ARM for the Morello platform and development tools
- University of Cambridge for CHERI research and development
- DARPA for supporting CHERI research through various programs
