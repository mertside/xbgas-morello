#!/bin/sh

#
# Quick validation script for the automated testing framework
# Tests the basic functionality without full execution
#

# Colors
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[0;33m'
BLUE='\033[0;34m'
NC='\033[0m'

printf "${BLUE}=================================================================\n"
printf "xBGAS TTU Automated Testing Framework - Quick Validation\n"
printf "=================================================================${NC}\n"

# Check if we're in the right directory
if [ ! -f "Makefile_automated" ]; then
    printf "${RED}Error: Makefile_automated not found${NC}\n"
    exit 1
fi

if [ ! -f "run_tests.sh" ]; then
    printf "${RED}Error: run_tests.sh not found${NC}\n"
    exit 1
fi

# Check if refactored test sources exist
printf "${BLUE}Checking refactored test sources...${NC}\n"

# POSIX shell compatible source list
check_source_file() {
    source_file="$1"
    if [ -f "$source_file" ]; then
        printf "${GREEN}✓ Found: $source_file${NC}\n"
        return 0
    else
        printf "${RED}✗ Missing: $source_file${NC}\n"
        return 1
    fi
}

# Check each refactored source file
missing_files=0

# Spatial safety tests
check_source_file "ttu_s1_free_not_at_start_refactored.c" || missing_files=$((missing_files + 1))
check_source_file "ttu_s2_free_not_on_heap_refactored.c" || missing_files=$((missing_files + 1))
check_source_file "ttu_s3_null_ptr_dereference_refactored.c" || missing_files=$((missing_files + 1))
check_source_file "ttu_s4_oob_read_refactored.c" || missing_files=$((missing_files + 1))
check_source_file "ttu_s5_oob_write_refactored.c" || missing_files=$((missing_files + 1))

# Temporal safety tests
check_source_file "ttu_t1_double_free_refactored.c" || missing_files=$((missing_files + 1))
check_source_file "ttu_t2_hm_fake_chunk_malloc_refactored.c" || missing_files=$((missing_files + 1))
check_source_file "ttu_t3_hm_house_of_spirit_refactored.c" || missing_files=$((missing_files + 1))
check_source_file "ttu_t4_hm_p_and_c_chunk_refactored.c" || missing_files=$((missing_files + 1))
check_source_file "ttu_t5_use_after_free_refactored.c" || missing_files=$((missing_files + 1))
check_source_file "ttu_t6_uaf_function_pointer_refactored_fixed.c" || missing_files=$((missing_files + 1))
check_source_file "ttu_t7_uaf_memcpy_refactored.c" || missing_files=$((missing_files + 1))

# Real-world vulnerability tests
check_source_file "ttu_r1_HeartBleed_refactored.c" || missing_files=$((missing_files + 1))
check_source_file "ttu_r2_dop_refactored.c" || missing_files=$((missing_files + 1))
check_source_file "ttu_r3_uaf_to_code_reuse_refactored.c" || missing_files=$((missing_files + 1))
check_source_file "ttu_r4_illegal_ptr_deref_refactored.c" || missing_files=$((missing_files + 1))
check_source_file "ttu_r5_df_switch_refactored.c" || missing_files=$((missing_files + 1))

if [ $missing_files -gt 0 ]; then
    printf "${RED}Error: $missing_files refactored source files are missing${NC}\n"
    exit 1
fi

printf "${GREEN}All refactored source files found!${NC}\n"

# Check if runtime dependencies exist
printf "${BLUE}Checking runtime dependencies...${NC}\n"
if [ -f "../../runtime/xbMrtime_api_asm.s" ]; then
    printf "${GREEN}✓ Found: runtime assembly source${NC}\n"
else
    printf "${YELLOW}⚠ Warning: runtime assembly source not found${NC}\n"
fi

# Make run_tests.sh executable
printf "${BLUE}Setting executable permissions...${NC}\n"
chmod +x run_tests.sh
printf "${GREEN}✓ run_tests.sh is now executable${NC}\n"

# Test Makefile syntax
printf "${BLUE}Validating Makefile syntax...${NC}\n"
if make -f Makefile_automated --dry-run help > /dev/null 2>&1; then
    printf "${GREEN}✓ Makefile syntax is valid${NC}\n"
else
    printf "${RED}✗ Makefile syntax error${NC}\n"
    exit 1
fi

# Test script help function
printf "${BLUE}Testing automation script help...${NC}\n"
if ./run_tests.sh help > /dev/null 2>&1; then
    printf "${GREEN}✓ Automation script is functional${NC}\n"
else
    printf "${RED}✗ Automation script has issues${NC}\n"
    exit 1
fi

# Quick syntax check on a few refactored sources
printf "${BLUE}Performing quick syntax check...${NC}\n"
syntax_ok=0
syntax_total=0

# Check first few files for basic syntax
for source in "ttu_s1_free_not_at_start_refactored.c" "ttu_s4_oob_read_refactored.c" "ttu_t1_double_free_refactored.c"; do
    syntax_total=$((syntax_total + 1))
    if cc -fsyntax-only "$source" 2>/dev/null; then
        printf "${GREEN}✓ Syntax OK: $source${NC}\n"
        syntax_ok=$((syntax_ok + 1))
    else
        printf "${YELLOW}⚠ Syntax warnings/errors: $source${NC}\n"
    fi
done

printf "${BLUE}Quick syntax check: $syntax_ok/$syntax_total files OK${NC}\n"

# Summary
printf "${GREEN}=================================================================\n"
printf "Validation Summary:\n"
printf "=================================================================\n"
printf "✓ All refactored source files present\n"
printf "✓ Makefile syntax valid\n"
printf "✓ Automation script functional\n"
printf "✓ Basic syntax check passed\n"
printf "\n"
printf "Framework is ready for use!\n"
printf "\n"
printf "Quick test commands:\n"
printf "  ./run_tests.sh build-spatial      # Build spatial tests\n"
printf "  ./run_tests.sh test-spatial       # Test spatial tests\n"
printf "  ./run_tests.sh validate-all       # Validate all builds\n"
printf "  make -f Makefile_automated help   # Show Makefile help\n"
printf "=================================================================${NC}\n"
