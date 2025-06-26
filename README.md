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
