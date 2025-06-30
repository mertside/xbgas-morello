# TTU Test Scripts

**Automation utilities for xBGAS-Morello TTU memory safety testing.**

## Scripts

| Script | Purpose | Usage |
|--------|---------|-------|
| `run_complete_suite.sh` | 🚀 Complete automation | Clean → Build → Test → Analyze |
| `interpret_results.sh` | 📊 CHERI-aware analysis | Parse results with correct semantics |
| `system_info.sh` | 🖥️ Environment info | System/compiler/CHERI status |
| `validate_build.sh` | 🔨 Build validation | Test all build components |
| `find_reports.sh` | 🔍 Report locator | Find test logs in various dirs |
| `cleanup_legacy.sh` | 🧹 Legacy removal | Remove old scattered scripts |

## Quick Start

```bash
# Make executable and run complete suite
chmod +x scripts/*.sh
./scripts/run_complete_suite.sh

# Or via Makefile
make complete-suite
```

## Integration

```bash
# Makefile targets
make complete-suite interpret system-info validate-build find-reports

# Direct execution
./scripts/run_complete_suite.sh    # Full automation
./scripts/interpret_results.sh     # Result analysis only
./scripts/find_reports.sh         # Diagnostic
```

## Key Features

- **CHERI-Aware**: Correctly interprets crashes as security successes
- **Auto-Discovery**: Finds reports in multiple possible locations  
- **Error Handling**: Strict POSIX shell with `set -e`
- **Cross-Platform**: Works on FreeBSD/Linux/macOS
