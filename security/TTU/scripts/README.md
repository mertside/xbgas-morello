# TTU Security Test Scripts

This directory contains utility scripts for the xBGAS-Morello TTU memory safety test suite.

## Main Scripts

### 🧪 Test Execution
- **`run_complete_suite.sh`** - Complete test suite runner with reporting
  - Builds all tests, runs them, and provides comprehensive analysis
  - Use this for complete automated testing

### 📊 Result Analysis  
- **`interpret_results.sh`** - CHERI-Morello result interpreter
  - Correctly interprets test results (crashes = success in security testing)
  - Provides clear analysis of CHERI protection effectiveness

### 🔍 Diagnostics
- **`find_reports.sh`** - Locates test reports in various directories
  - Helps troubleshoot when reports can't be found
  - Shows where log files are actually created

- **`system_info.sh`** - System and environment information
  - Displays system configuration, compiler info, CHERI status
  - Useful for debugging environment issues

### 🔨 Build Validation
- **`validate_build.sh`** - Build system validation
  - Tests that all components build correctly
  - Validates environment and dependencies

## Quick Start

```bash
# Make scripts executable
chmod +x scripts/*.sh

# Complete test run with analysis
./scripts/run_complete_suite.sh

# Or run individual components:
./scripts/system_info.sh          # Check environment
./scripts/validate_build.sh       # Validate build system  
./scripts/find_reports.sh         # Find test reports
./scripts/interpret_results.sh    # Analyze results
```

## Script Descriptions

### run_complete_suite.sh
The main automation script that:
1. Cleans previous builds
2. Builds all test categories
3. Executes comprehensive test suite
4. Provides analysis and interpretation

### interpret_results.sh
Critical for understanding CHERI-Morello results:
- ✅ "Killed" or "In-address space security exception" = SUCCESS
- ⚠️ Normal completion = Potential vulnerability not caught
- Searches multiple locations for test reports
- Provides protection rate statistics

### find_reports.sh
Diagnostic utility that searches for test reports in:
- Current directory
- `./reports/`
- `./obj/` and `./obj/reports/`
- Parent directory
- Recursive search

### system_info.sh
Environment validator showing:
- System information (CPU, memory, OS)
- Compiler versions and capabilities
- CHERI/Morello environment status
- Project file validation
- Build tool availability

### validate_build.sh
Build system validator that:
- Checks environment and files
- Tests compilation of individual categories
- Validates complete build process
- Counts built executables

## Integration with Makefile

These scripts work alongside the main `Makefile_objaware` targets:

```bash
# Makefile targets
make run-all          # Run all tests
make analyze          # Built-in analysis
make summary          # Quick summary
make show-failures    # Show non-trapped tests

# Script alternatives/supplements
./scripts/run_complete_suite.sh    # Complete automation
./scripts/interpret_results.sh     # Enhanced interpretation
```

## Error Handling

All scripts use `set -e` for strict error handling and will exit on any command failure. This ensures reliable automation and clear error reporting.

## Compatibility

Scripts are written in POSIX shell (`/bin/sh`) for maximum compatibility across:
- FreeBSD (Morello)
- Linux  
- macOS
- Other UNIX-like systems
