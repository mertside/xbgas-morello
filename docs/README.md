# xBGAS-Morello Documentation

This directory contains project documentation and diagrams.

## Contents

- **`xbrtime-simple-overview-uml.png`** - UML diagram showing the xBGAS runtime architecture
- **`xbrtime-simple-overview-uml.pdf`** - PDF version of the architecture diagram

## Architecture Overview

The UML diagram illustrates the four-layer architecture of the xBGAS-Morello implementation:

1. **Hardware Abstraction Layer** - ARM FVP emulation of Morello SoC
2. **Low-level Runtime** - Translated API functions from RISC-V to ARM ISA  
3. **High-level Runtime** - Thread pooling for multi-PE simulation
4. **Application Interface** - Minimally modified benchmarks with entry/exit points

## Additional Documentation

- **Main project documentation**: [`../README.md`](../README.md)
- **Security test documentation**: [`../security/TTU/README.md`](../security/TTU/README.md)
- **Performance benchmark documentation**: [`../bench/README.md`](../bench/README.md)
- **Security suite overview**: [`../security/README.md`](../security/README.md)
