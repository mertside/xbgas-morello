# xBGAS-Morello Performance Benchmarks

Performance evaluation suite for measuring CHERI-Morello overhead and scalability in the xBGAS runtime environment.

## Quick Start

```bash
# Build all benchmarks
make all

# Run performance tests
make test

# Clean build artifacts  
make clean
```

## Benchmark Categories

### Core xBGAS Operations
- **`xbrtime_matmul.c`** - Matrix multiplication with distributed memory access patterns
- **`xbrtime_gather.c`** - Gather operations testing remote memory collection
- **`xbrtime_gups.c`** - Global Updates Per Second (memory bandwidth intensive)
- **`xbrtime_broadcast8.c`** - Collective communication patterns
- **`xbrtime_reduction8.c`** - Reduction operations across processing elements

### SHMEM Compatibility Tests
- **`SHMEMRandomAccess.c`** - SHMEM-style random memory access patterns
- **`SHMEMRandomAccess_v2.c`** - Enhanced version with improved algorithms
- **`openshmem/`** - Full OpenSHMEM compatibility benchmark suite

### Specialized Performance Tests
- **`gups/`** - HPCC-compliant GUPS benchmarks for standardized comparison

## Key Performance Metrics

The benchmarks measure:
- **Memory bandwidth** under CHERI capability protection
- **Thread pool efficiency** with memory safety constraints  
- **Remote memory access performance** with capability bounds checking
- **Scalability characteristics** across multiple processing elements
- **CHERI overhead** compared to baseline architectures

## Research Applications

These benchmarks support research in:
- **Performance impact analysis** of capability-based memory safety
- **Scalability assessment** of CHERI-protected distributed computing
- **Overhead quantification** for academic publications
- **Real-world deployment viability** evaluation

## Integration with Security Tests

The benchmarks complement the security evaluation in [`../security/TTU/`](../security/TTU/) by demonstrating that CHERI-Morello provides both:
1. **Security** - Memory safety protection (shown by security tests)
2. **Performance** - Acceptable overhead for practical deployment (shown by these benchmarks)

## Build Requirements

- CHERI-Morello development environment
- BSD-compatible make
- xBGAS runtime headers (automatically included from `../runtime/`)
- POSIX-compliant thread support
