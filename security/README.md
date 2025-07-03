# xBGAS-Morello Security Test Suites

**CHERI-Morello capability-based memory safety evaluation frameworks.**

## Test Suites

### TTU - Comprehensive Memory Safety Tests
**Primary Framework** - Production-ready automation with 17 vulnerability tests
- **Location**: `TTU/`
- **Tests**: 17 (5 spatial, 7 temporal, 5 real-world)
- **Features**: Full automation, CHERI-aware analysis, BSD make compatible
- **Usage**: `cd TTU && make run-all`

### ASU - Arizona State University Tests
Legacy security tests from ASU research
- **Location**: `ASU/`
- **Focus**: Academic research validation

### UoC - University of Cambridge Tests  
Cambridge-developed CHERI security tests
- **Location**: `UoC/`
- **Focus**: CHERI capability system validation

### Experimental - Development Tests
Experimental and development security tests
- **Location**: `experimental/`
- **Focus**: New test development and research

## Recommended Usage

**For comprehensive security evaluation**: Use `TTU/` - it provides the most complete, automated framework with proper CHERI result interpretation.

**For specific research**: Use suite-specific directories (ASU, UoC, experimental) for targeted testing.

## Quick Start

```bash
# Primary recommendation - TTU comprehensive suite
cd TTU
make run-all

# View results with CHERI interpretation
make analyze
```

---
*xBGAS-Morello Security Research Platform*
