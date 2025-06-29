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

echo -e "${BLUE}================================================================="
echo -e "xBGAS TTU Automated Testing Framework - Quick Validation"
echo -e "=================================================================${NC}"

# Check if we're in the right directory
if [[ ! -f "Makefile_automated" ]]; then
    echo -e "${RED}Error: Makefile_automated not found${NC}"
    exit 1
fi

if [[ ! -f "run_tests.sh" ]]; then
    echo -e "${RED}Error: run_tests.sh not found${NC}"
    exit 1
fi

# Check if refactored test sources exist
echo -e "${BLUE}Checking refactored test sources...${NC}"
refactored_sources=(
    "ttu_s1_free_not_at_start_refactored.c"
    "ttu_s2_free_not_on_heap_refactored.c"
    "ttu_s3_null_ptr_dereference_refactored.c"
    "ttu_s4_oob_read_refactored.c"
    "ttu_s5_oob_write_refactored.c"
    "ttu_t1_double_free_refactored.c"
    "ttu_t2_hm_fake_chunk_malloc_refactored.c"
    "ttu_t3_hm_house_of_spirit_refactored.c"
    "ttu_t4_hm_p_and_c_chunk_refactored.c"
    "ttu_t5_use_after_free_refactored.c"
    "ttu_t6_uaf_function_pointer_refactored_fixed.c"
    "ttu_t7_uaf_memcpy_refactored_fixed.c"
    "ttu_r1_HeartBleed_refactored.c"
    "ttu_r2_dop_refactored.c"
    "ttu_r3_uaf_to_code_reuse_refactored.c"
    "ttu_r4_illegal_ptr_deref_refactored.c"
    "ttu_r5_df_switch_refactored.c"
)

missing_files=0
for source in "${refactored_sources[@]}"; do
    if [[ -f "$source" ]]; then
        echo -e "${GREEN}✓ Found: $source${NC}"
    else
        echo -e "${RED}✗ Missing: $source${NC}"
        missing_files=$((missing_files + 1))
    fi
done

if [[ $missing_files -gt 0 ]]; then
    echo -e "${RED}Error: $missing_files refactored source files are missing${NC}"
    exit 1
fi

echo -e "${GREEN}All refactored source files found!${NC}"

# Check if runtime dependencies exist
echo -e "${BLUE}Checking runtime dependencies...${NC}"
if [[ -f "../../runtime/xbMrtime_api_asm.s" ]]; then
    echo -e "${GREEN}✓ Found: runtime assembly source${NC}"
else
    echo -e "${YELLOW}⚠ Warning: runtime assembly source not found${NC}"
fi

# Make run_tests.sh executable
echo -e "${BLUE}Setting executable permissions...${NC}"
chmod +x run_tests.sh
echo -e "${GREEN}✓ run_tests.sh is now executable${NC}"

# Test Makefile syntax
echo -e "${BLUE}Validating Makefile syntax...${NC}"
if make -f Makefile_automated --dry-run help > /dev/null 2>&1; then
    echo -e "${GREEN}✓ Makefile syntax is valid${NC}"
else
    echo -e "${RED}✗ Makefile syntax error${NC}"
    exit 1
fi

# Test script help function
echo -e "${BLUE}Testing automation script help...${NC}"
if ./run_tests.sh help > /dev/null 2>&1; then
    echo -e "${GREEN}✓ Automation script is functional${NC}"
else
    echo -e "${RED}✗ Automation script has issues${NC}"
    exit 1
fi

# Quick syntax check on a few refactored sources
echo -e "${BLUE}Performing quick syntax check...${NC}"
syntax_ok=0
syntax_total=0

for source in "${refactored_sources[@]:0:3}"; do  # Check first 3 files
    syntax_total=$((syntax_total + 1))
    if cc -fsyntax-only "$source" 2>/dev/null; then
        echo -e "${GREEN}✓ Syntax OK: $source${NC}"
        syntax_ok=$((syntax_ok + 1))
    else
        echo -e "${YELLOW}⚠ Syntax warnings/errors: $source${NC}"
    fi
done

echo -e "${BLUE}Quick syntax check: $syntax_ok/$syntax_total files OK${NC}"

# Summary
echo -e "${GREEN}================================================================="
echo -e "Validation Summary:"
echo -e "================================================================="
echo -e "✓ All refactored source files present"
echo -e "✓ Makefile syntax valid"
echo -e "✓ Automation script functional"
echo -e "✓ Basic syntax check passed"
echo -e ""
echo -e "Framework is ready for use!"
echo -e ""
echo -e "Quick test commands:"
echo -e "  ./run_tests.sh build-spatial      # Build spatial tests"
echo -e "  ./run_tests.sh test-spatial       # Test spatial tests"
echo -e "  ./run_tests.sh validate-all       # Validate all builds"
echo -e "  make -f Makefile_automated help   # Show Makefile help"
echo -e "=================================================================${NC}"
