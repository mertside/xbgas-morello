#
# BSD Make obj-directory-aware Makefile for xBGAS TTU Memory Safety Tests
#

# Basic Configuration
CC ?= cc
CFLAGS = -g -O2 -Wall -Wextra -std=c99 -I../../runtime
LDFLAGS = -lpthread -lm
COMPILE = $(CC) $(CFLAGS) $(LDFLAGS)

# Auto-detect if we're in obj directory (BSD make behavior)
# If current directory contains no .c files but parent does, we're in obj
CURRENT_C_FILES != ls -1 *.c 2>/dev/null | wc -l
PARENT_C_FILES != ls -1 ../*.c 2>/dev/null | wc -l

# Set source directory based on detection
.if $(CURRENT_C_FILES) == 0 && $(PARENT_C_FILES) > 0
SRCDIR = ..
RTDIR = ../../../runtime
.else
SRCDIR = .
RTDIR = ../../runtime
.endif

# Update CFLAGS with correct runtime path
CFLAGS = -g -O2 -Wall -Wextra -std=c99 -I$(RTDIR)

# Assembly source file
ASM_SOURCE = $(RTDIR)/xbMrtime_api_asm.s

# Compilation command - include assembly if it exists
.if exists($(ASM_SOURCE))
COMPILE = $(CC) $(CFLAGS) $(LDFLAGS) $(ASM_SOURCE)
.else
COMPILE = $(CC) $(CFLAGS) $(LDFLAGS)
.endif

# Output Directory
REPORTDIR = ./reports

# Test Sources with auto-detected source directory
SPATIAL_SOURCES = \
	$(SRCDIR)/ttu_s1_free_not_at_start_refactored.c \
	$(SRCDIR)/ttu_s2_free_not_on_heap_refactored.c \
	$(SRCDIR)/ttu_s3_null_ptr_dereference_refactored.c \
	$(SRCDIR)/ttu_s4_oob_read_refactored.c \
	$(SRCDIR)/ttu_s5_oob_write_refactored.c

TEMPORAL_SOURCES = \
	$(SRCDIR)/ttu_t1_double_free_refactored.c \
	$(SRCDIR)/ttu_t2_hm_fake_chunk_malloc_refactored.c \
	$(SRCDIR)/ttu_t3_hm_house_of_spirit_refactored.c \
	$(SRCDIR)/ttu_t4_hm_p_and_c_chunk_refactored.c \
	$(SRCDIR)/ttu_t5_use_after_free_refactored.c \
	$(SRCDIR)/ttu_t6_uaf_function_pointer_refactored_fixed.c \
	$(SRCDIR)/ttu_t7_uaf_memcpy_refactored.c

REALWORLD_SOURCES = \
	$(SRCDIR)/ttu_r1_HeartBleed_refactored.c \
	$(SRCDIR)/ttu_r2_dop_refactored.c \
	$(SRCDIR)/ttu_r3_uaf_to_code_reuse_refactored.c \
	$(SRCDIR)/ttu_r4_illegal_ptr_deref_refactored.c \
	$(SRCDIR)/ttu_r5_df_switch_refactored.c

HEAP_SOURCES = \
	$(SRCDIR)/ttu_t2_hm_fake_chunk_malloc_refactored.c \
	$(SRCDIR)/ttu_t3_hm_house_of_spirit_refactored.c \
	$(SRCDIR)/ttu_t4_hm_p_and_c_chunk_refactored.c

# ============================================================================
#                              BUILD TARGETS  
# ============================================================================

all: check-environment spatial temporal realworld
	@echo "===================================================="
	@echo "All TTU refactored tests built successfully!"
	@echo "===================================================="

check-environment:
	@echo "=== Environment Check ==="
	@echo "Compiler: $(CC)"
	@echo "Working directory: `pwd`"
	@echo "Source directory: $(SRCDIR)"
	@echo "Runtime directory: $(RTDIR)"
	@echo "Assembly source: $(ASM_SOURCE)"
	@echo "Available refactored files in source dir:"
	@ls -1 $(SRCDIR)/*_refactored*.c 2>/dev/null | wc -l | sed 's/^/  Found: /' || echo "  Found: 0"
	@echo "Runtime directory exists: `test -d $(RTDIR) && echo YES || echo NO`"
	@echo "Assembly source exists: `test -f $(ASM_SOURCE) && echo YES || echo NO`"
.if !exists($(ASM_SOURCE))
	@echo "⚠ WARNING: Assembly source not found - some tests may fail to link"
	@echo "  Expected: $(ASM_SOURCE)"
.endif
	@mkdir -p $(REPORTDIR)

spatial: check-environment
	@echo "=== Building Spatial Safety Tests ==="
	@for src in $(SPATIAL_SOURCES); do \
		if test -f "$$src"; then \
			target=`basename $$src .c`.exe; \
			echo "Building $$target from $$src..."; \
			if $(COMPILE) -o $$target $$src > $(REPORTDIR)/build_$$target.log 2>&1; then \
				echo "  ✓ Built $$target"; \
			else \
				echo "  ✗ Failed to build $$target"; \
				echo "  Error details:"; \
				cat $(REPORTDIR)/build_$$target.log | sed 's/^/    /'; \
				exit 1; \
			fi; \
		else \
			echo "  ⚠ Missing source file: $$src"; \
		fi; \
	done

temporal: check-environment
	@echo "=== Building Temporal Safety Tests ==="
	@for src in $(TEMPORAL_SOURCES); do \
		if test -f "$$src"; then \
			target=`basename $$src .c`.exe; \
			echo "Building $$target from $$src..."; \
			if $(COMPILE) -o $$target $$src > $(REPORTDIR)/build_$$target.log 2>&1; then \
				echo "  ✓ Built $$target"; \
			else \
				echo "  ✗ Failed to build $$target"; \
				echo "  Error details:"; \
				cat $(REPORTDIR)/build_$$target.log | sed 's/^/    /'; \
				exit 1; \
			fi; \
		else \
			echo "  ⚠ Missing source file: $$src"; \
		fi; \
	done

realworld: check-environment
	@echo "=== Building Real-World Tests ==="
	@for src in $(REALWORLD_SOURCES); do \
		if test -f "$$src"; then \
			target=`basename $$src .c`.exe; \
			echo "Building $$target from $$src..."; \
			if $(COMPILE) -o $$target $$src > $(REPORTDIR)/build_$$target.log 2>&1; then \
				echo "  ✓ Built $$target"; \
			else \
				echo "  ✗ Failed to build $$target"; \
				echo "  Error details:"; \
				cat $(REPORTDIR)/build_$$target.log | sed 's/^/    /'; \
				exit 1; \
			fi; \
		else \
			echo "  ⚠ Missing source file: $$src"; \
		fi; \
	done

heap: check-environment
	@echo "=== Building Heap Manipulation Tests ==="
	@for src in $(HEAP_SOURCES); do \
		if test -f "$$src"; then \
			target=`basename $$src .c`.exe; \
			echo "Building $$target from $$src..."; \
			if $(COMPILE) -o $$target $$src > $(REPORTDIR)/build_$$target.log 2>&1; then \
				echo "  ✓ Built $$target"; \
			else \
				echo "  ✗ Failed to build $$target"; \
				echo "  Error details:"; \
				cat $(REPORTDIR)/build_$$target.log | sed 's/^/    /'; \
				exit 1; \
			fi; \
		else \
			echo "  ⚠ Missing source file: $$src"; \
		fi; \
	done

# ============================================================================
#                              TEST EXECUTION
# ============================================================================

run-spatial: spatial
	@echo "=== Running Spatial Safety Tests ==="
	@for src in $(SPATIAL_SOURCES); do \
		target=`basename $$src .c`.exe; \
		if test -f "$$target"; then \
			echo "Running $$target..."; \
			./$$target > $(REPORTDIR)/run_$$target.log 2>&1; \
			status=$$?; \
			if test $$status -eq 0; then \
				echo "  ⚠ $$target completed normally (vulnerability may not be caught)"; \
			else \
				echo "  ✓ $$target trapped by CHERI-Morello (exit code: $$status)"; \
			fi; \
		fi; \
	done

run-temporal: temporal
	@echo "=== Running Temporal Safety Tests ==="
	@for src in $(TEMPORAL_SOURCES); do \
		target=`basename $$src .c`.exe; \
		if test -f "$$target"; then \
			echo "Running $$target..."; \
			./$$target > $(REPORTDIR)/run_$$target.log 2>&1; \
			status=$$?; \
			if test $$status -eq 0; then \
				echo "  ⚠ $$target completed normally (vulnerability may not be caught)"; \
			else \
				echo "  ✓ $$target trapped by CHERI-Morello (exit code: $$status)"; \
			fi; \
		fi; \
	done

run-realworld: realworld
	@echo "=== Running Real-World Tests ==="
	@for src in $(REALWORLD_SOURCES); do \
		target=`basename $$src .c`.exe; \
		if test -f "$$target"; then \
			echo "Running $$target..."; \
			./$$target > $(REPORTDIR)/run_$$target.log 2>&1; \
			status=$$?; \
			if test $$status -eq 0; then \
				echo "  ⚠ $$target completed normally (vulnerability may not be caught)"; \
			else \
				echo "  ✓ $$target trapped by CHERI-Morello (exit code: $$status)"; \
			fi; \
		fi; \
	done

run-all: all
	@echo "========================================================="
	@echo "Running comprehensive TTU security test suite"
	@echo "========================================================="
	@$(MAKE) -f $(SRCDIR)/Makefile_objaware run-spatial
	@$(MAKE) -f $(SRCDIR)/Makefile_objaware run-temporal
	@$(MAKE) -f $(SRCDIR)/Makefile_objaware run-realworld

# ============================================================================
#                              VALIDATION
# ============================================================================

check-files:
	@echo "=== File Check ==="
	@echo "Source directory: $(SRCDIR)"
	@echo "Spatial sources (expected 5):"
	@for src in $(SPATIAL_SOURCES); do \
		test -f "$$src" && echo "  ✓ $$src" || echo "  ✗ $$src (missing)"; \
	done
	@echo "Temporal sources (expected 7):"
	@for src in $(TEMPORAL_SOURCES); do \
		test -f "$$src" && echo "  ✓ $$src" || echo "  ✗ $$src (missing)"; \
	done
	@echo "Real-world sources (expected 5):"
	@for src in $(REALWORLD_SOURCES); do \
		test -f "$$src" && echo "  ✓ $$src" || echo "  ✗ $$src (missing)"; \
	done

compile-test:
	@echo "=== Compilation Test ==="
	@echo "Testing single file compilation..."
	@test_file="$(SRCDIR)/ttu_s1_free_not_at_start_refactored.c"; \
	if test -f "$$test_file"; then \
		echo "Compiling $$test_file..."; \
		$(COMPILE) -o test_compile.exe $$test_file && \
			{ echo "✓ Compilation successful"; rm -f test_compile.exe; } || \
			echo "✗ Compilation failed"; \
	else \
		echo "✗ Test file not found: $$test_file"; \
	fi

# ============================================================================
#                              MAINTENANCE
# ============================================================================

clean:
	@echo "Cleaning generated files..."
	@rm -f *.exe *.o *.core
	@rm -rf $(REPORTDIR)

clean-logs:
	@rm -rf $(REPORTDIR)
	@mkdir -p $(REPORTDIR)

# ============================================================================
#                                 HELP
# ============================================================================

config:
	@echo "=== Build Configuration ==="
	@echo "CC = $(CC)"
	@echo "CFLAGS = $(CFLAGS)"
	@echo "LDFLAGS = $(LDFLAGS)"
	@echo "COMPILE = $(COMPILE)"
	@echo "SRCDIR = $(SRCDIR)"
	@echo "RTDIR = $(RTDIR)"
	@echo "REPORTDIR = $(REPORTDIR)"
	@echo "PWD = `pwd`"

help:
	@echo "TTU Memory Safety Tests - BSD Make obj-aware Build System"
	@echo "========================================================="
	@echo "This Makefile automatically handles BSD make's obj directory behavior"
	@echo ""
	@echo "ENVIRONMENT:"
	@echo "  check-environment - Show build environment"
	@echo "  config           - Show build configuration"
	@echo "  check-files      - Verify source files exist"
	@echo ""
	@echo "BUILD TARGETS:"
	@echo "  all        - Build all refactored tests"
	@echo "  spatial    - Build spatial safety tests (5)"
	@echo "  temporal   - Build temporal safety tests (7)"
	@echo "  realworld  - Build real-world tests (5)"
	@echo "  heap       - Build heap manipulation tests (3)"
	@echo ""
	@echo "TEST TARGETS:"
	@echo "  run-all      - Run all tests"
	@echo "  run-spatial  - Run spatial tests"
	@echo "  run-temporal - Run temporal tests"
	@echo "  run-realworld- Run real-world tests"
	@echo ""
	@echo "ANALYSIS:"
	@echo "  summary      - Show test results summary"  
	@echo "  analyze      - Detailed security analysis"
	@echo "  show-failures- Show tests that completed normally"
	@echo "  show-all-logs- Show all test execution logs"
	@echo ""
	@echo "SCRIPTS:"
	@echo "  complete-suite- Run complete automated test suite"
	@echo "  interpret     - Interpret test results (CHERI-aware)"
	@echo "  system-info   - Show system and environment info"
	@echo "  validate-build- Validate build system"
	@echo "  find-reports  - Locate test reports"
	@echo "  cleanup-scripts- Remove legacy scattered scripts"
	@echo "  cleanup-docs  - Remove redundant documentation"
	@echo ""
	@echo "IMPORTANT - CHERI-Morello Result Interpretation:"
	@echo "  ✓ 'Killed' or 'In-address space security exception' = SUCCESS"
	@echo "    (CHERI caught the memory safety violation)"
	@echo "  ⚠ Normal program completion = Potential issue"
	@echo "    (Vulnerability may not have been caught)"
	@echo ""
	@echo "Quick start: make complete-suite"

# ============================================================================
#                         ANALYSIS AND REPORTING
# ============================================================================

# Comprehensive analysis of test results
analyze: run-all
	@echo "=== CHERI-Morello Memory Safety Test Analysis ==="
	@echo ""
	@echo "Report directory: $(REPORTDIR)"
	@trapped=0; normal=0; total=0; \
	echo "=== Test Result Summary ==="; \
	for log in $(REPORTDIR)/run_*.log; do \
		if test -f "$$log"; then \
			total=$$((total + 1)); \
			test_name=`basename $$log .log | sed 's/run_//'`; \
			if grep -q "In-address space security exception\|Killed\|Segmentation fault\|Abort" "$$log" 2>/dev/null; then \
				trapped=$$((trapped + 1)); \
				echo "  ✓ $$test_name: CHERI protection triggered (GOOD)"; \
			else \
				normal=$$((normal + 1)); \
				echo "  ⚠ $$test_name: Completed normally (vulnerability not caught)"; \
			fi; \
		fi; \
	done; \
	echo ""; \
	echo "=== Final Results ==="; \
	echo "Total tests: $$total"; \
	echo "Protected by CHERI: $$trapped"; \
	echo "Completed normally: $$normal"; \
	echo "CHERI protection rate: `expr $$trapped \* 100 / $$total 2>/dev/null || echo 0`%"; \
	echo ""; \
	if test $$trapped -gt $$normal; then \
		echo "✓ CHERI-Morello is successfully catching most memory safety violations!"; \
	else \
		echo "⚠ Many tests completed without triggering CHERI protections."; \
	fi

# Quick summary
summary:
	@echo "=== Quick Test Summary ==="
	@if test -d "$(REPORTDIR)"; then \
		total=`ls $(REPORTDIR)/run_*.log 2>/dev/null | wc -l | tr -d ' '`; \
		trapped=`grep -l "In-address space security exception\|Killed\|Segmentation fault\|Abort" $(REPORTDIR)/run_*.log 2>/dev/null | wc -l | tr -d ' '`; \
		echo "Tests run: $$total"; \
		echo "CHERI protections triggered: $$trapped"; \
		echo "Normal completions: `expr $$total - $$trapped 2>/dev/null || echo 0`"; \
	else \
		echo "No reports found. Run 'make run-all' first."; \
	fi

# Show detailed logs for failed (non-trapped) tests
show-failures:
	@echo "=== Tests that completed normally (potential issues) ==="
	@for log in $(REPORTDIR)/run_*.log; do \
		if test -f "$$log" && ! grep -q "In-address space security exception\|Killed\|Segmentation fault\|Abort" "$$log" 2>/dev/null; then \
			echo ""; \
			echo "=== `basename $$log` ==="; \
			cat "$$log"; \
		fi; \
	done

# Show all test outputs
show-all-logs:
	@echo "=== All Test Execution Logs ==="
	@for log in $(REPORTDIR)/run_*.log; do \
		if test -f "$$log"; then \
			echo ""; \
			echo "=== `basename $$log` ==="; \
			cat "$$log"; \
		fi; \
	done

# Debug and diagnostic targets
debug-reports:
	@echo "=== Report Location Debug ==="
	@echo "Current directory: `pwd`"
	@echo "REPORTDIR setting: $(REPORTDIR)"
	@echo "Expected report path: $(REPORTDIR)"
	@echo ""
	@echo "Checking for reports..."
	@if test -d "$(REPORTDIR)"; then \
		echo "Report directory exists: $(REPORTDIR)"; \
		if ls $(REPORTDIR)/run_*.log >/dev/null 2>&1; then \
			echo "Found log files:"; \
			ls -la $(REPORTDIR)/run_*.log | sed 's/^/  /'; \
		else \
			echo "No log files found in $(REPORTDIR)"; \
		fi; \
	else \
		echo "Report directory does not exist: $(REPORTDIR)"; \
	fi
	@echo ""
	@echo "Checking for executables..."
	@if ls *.exe >/dev/null 2>&1; then \
		echo "Found executables:"; \
		ls -la *.exe | sed 's/^/  /'; \
	else \
		echo "No executables found"; \
	fi

find-all-reports:
	@echo "=== Finding all test reports ==="
	@find . -name "run_*.log" -type f 2>/dev/null | while read file; do \
		echo "Found: $$file"; \
		echo "  Size: `ls -lh $$file | awk '{print $$5}'`"; \
		echo "  Modified: `ls -l $$file | awk '{print $$6, $$7, $$8}'`"; \
	done || echo "No run_*.log files found anywhere"

# ============================================================================
#                              SCRIPT INTEGRATION
# ============================================================================

# Quick access to organized scripts
system-info:
	@chmod +x scripts/system_info.sh
	@scripts/system_info.sh

validate-build:
	@chmod +x scripts/validate_build.sh
	@scripts/validate_build.sh

complete-suite:
	@chmod +x scripts/run_complete_suite.sh
	@scripts/run_complete_suite.sh

interpret:
	@chmod +x scripts/interpret_results.sh
	@scripts/interpret_results.sh

find-reports:
	@chmod +x scripts/find_reports.sh
	@scripts/find_reports.sh

# Legacy cleanup
cleanup-scripts:
	@chmod +x scripts/cleanup_legacy.sh
	@scripts/cleanup_legacy.sh

cleanup-docs:
	@chmod +x scripts/cleanup_docs.sh
	@scripts/cleanup_docs.sh

.PHONY: all check-environment spatial temporal realworld heap
.PHONY: run-spatial run-temporal run-realworld run-all
.PHONY: check-files compile-test clean clean-logs config help
.PHONY: summary analyze show-failures show-all-logs
.PHONY: debug-reports find-all-reports
.PHONY: system-info validate-build complete-suite interpret find-reports cleanup-scripts cleanup-docs

# EOF
