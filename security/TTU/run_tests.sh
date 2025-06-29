#!/bin/sh

#
# Automated Test Runner for xBGAS TTU Memory Safety Tests
# 
# Copyright (C) 2024 Texas Tech University
# All Rights Reserved
#
# This script provides comprehensive automation for building, testing,
# and analyzing the refactored TTU memory safety test suite.
#

# Script configuration
SCRIPT_NAME="$(basename "$0")"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
TEST_DIR="$SCRIPT_DIR"
MAKEFILE="$TEST_DIR/Makefile_automated"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
BLUE='\033[0;34m'
PURPLE='\033[0;35m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

# Configuration
TIMEOUT_DURATION=30
REPORT_DIR="./reports"
LOG_FILE="$REPORT_DIR/automation.log"

# Test categories
SPATIAL_TESTS="ttu_s1_free_not_at_start_refactored ttu_s2_free_not_on_heap_refactored ttu_s3_null_ptr_dereference_refactored ttu_s4_oob_read_refactored ttu_s5_oob_write_refactored"
TEMPORAL_TESTS="ttu_t1_double_free_refactored ttu_t2_hm_fake_chunk_malloc_refactored ttu_t3_hm_house_of_spirit_refactored ttu_t4_hm_p_and_c_chunk_refactored ttu_t5_use_after_free_refactored ttu_t6_uaf_function_pointer_refactored_fixed ttu_t7_uaf_memcpy_refactored"
REALWORLD_TESTS="ttu_r1_HeartBleed_refactored ttu_r2_dop_refactored ttu_r3_uaf_to_code_reuse_refactored ttu_r4_illegal_ptr_deref_refactored ttu_r5_df_switch_refactored"
HEAP_TESTS="ttu_t2_hm_fake_chunk_malloc_refactored ttu_t3_hm_house_of_spirit_refactored ttu_t4_hm_p_and_c_chunk_refactored"

# Logging function
log() {
    echo -e "$(date '+%Y-%m-%d %H:%M:%S') - $1" | tee -a "$LOG_FILE"
}

# Print colored output
print_colored() {
    local color=$1
    local message=$2
    echo -e "${color}${message}${NC}"
}

# Print banner
print_banner() {
    print_colored "$GREEN" "================================================================="
    print_colored "$GREEN" "xBGAS TTU Memory Safety Tests - Automated Test Runner"
    print_colored "$GREEN" "Texas Tech University - CHERI-Morello Security Evaluation"
    print_colored "$GREEN" "================================================================="
}

# Print usage
print_usage() {
    echo "Usage: $SCRIPT_NAME [COMMAND] [OPTIONS]"
    echo ""
    echo "COMMANDS:"
    echo "  build-all          Build all refactored tests"
    echo "  build-spatial      Build spatial safety tests"
    echo "  build-temporal     Build temporal safety tests"
    echo "  build-realworld    Build real-world vulnerability tests"
    echo "  build-heap         Build heap manipulation tests"
    echo ""
    echo "  test-all           Run all refactored tests"
    echo "  test-spatial       Run spatial safety tests"
    echo "  test-temporal      Run temporal safety tests" 
    echo "  test-realworld     Run real-world vulnerability tests"
    echo "  test-heap          Run heap manipulation tests"
    echo ""
    echo "  validate-all       Validate all tests compile successfully"
    echo "  syntax-check       Check syntax of all source files"
    echo "  generate-report    Generate comprehensive test report"
    echo "  clean              Clean all generated files"
    echo "  help               Show this help message"
    echo ""
    echo "OPTIONS:"
    echo "  --timeout=N        Set test timeout in seconds (default: $TIMEOUT_DURATION)"
    echo "  --verbose          Enable verbose output"
    echo "  --quiet            Suppress non-essential output"
    echo "  --no-color         Disable colored output"
    echo ""
    echo "EXAMPLES:"
    echo "  $SCRIPT_NAME build-all"
    echo "  $SCRIPT_NAME test-spatial --verbose"
    echo "  $SCRIPT_NAME test-all --timeout=60"
    echo "  $SCRIPT_NAME validate-all --quiet"
}

# Initialize environment
initialize() {
    # Create necessary directories
    mkdir -p "$REPORT_DIR"
    
    # Initialize log file
    log "Initializing automated test runner"
    log "Working directory: $TEST_DIR"
    log "Makefile: $MAKEFILE"
    
    # Check if Makefile exists
    if [[ ! -f "$MAKEFILE" ]]; then
        print_colored "$RED" "Error: Makefile not found at $MAKEFILE"
        exit 1
    fi
    
    # Check if we're in the correct directory
    if [[ ! -f "ttu_s1_free_not_at_start_refactored.c" ]]; then
        print_colored "$RED" "Error: Refactored test sources not found in current directory"
        print_colored "$RED" "Please run this script from the TTU security tests directory"
        exit 1
    fi
}

# Build tests by category
build_tests() {
    local category=$1
    local target=""
    
    case $category in
        "all")
            target="all"
            ;;
        "spatial")
            target="spatial-refactored"
            ;;
        "temporal")
            target="temporal-refactored"
            ;;
        "realworld")
            target="realworld-refactored"
            ;;
        "heap")
            target="heap-refactored"
            ;;
        *)
            print_colored "$RED" "Error: Unknown build category '$category'"
            return 1
            ;;
    esac
    
    print_colored "$BLUE" "Building $category tests..."
    log "Building $category tests using target '$target'"
    
    if make -f "$MAKEFILE" "$target"; then
        print_colored "$GREEN" "Successfully built $category tests"
        log "Build successful for $category tests"
        return 0
    else
        print_colored "$RED" "Failed to build $category tests"
        log "Build failed for $category tests"
        return 1
    fi
}

# Run tests by category with detailed reporting
run_test_category() {
    local category=$1
    local test_list=""
    
    case $category in
        "spatial")
            test_list="$SPATIAL_TESTS"
            ;;
        "temporal")
            test_list="$TEMPORAL_TESTS"
            ;;
        "realworld")
            test_list="$REALWORLD_TESTS"
            ;;
        "heap")
            test_list="$HEAP_TESTS"
            ;;
        *)
            print_colored "$RED" "Error: Unknown test category '$category'"
            return 1
            ;;
    esac
    
    print_colored "$CYAN" "Running $category tests..."
    log "Starting $category test execution"
    
    local total=0
    local passed=0
    local failed=0
    local report_file="$REPORT_DIR/${category}_detailed_results.txt"
    
    # Initialize report file
    {
        echo "xBGAS TTU Memory Safety Tests - $category Category"
        echo "Test execution started: $(date)"
        echo "================================================================="
        echo ""
    } > "$report_file"
    
    # Build tests first
    if ! build_tests "$category"; then
        print_colored "$RED" "Cannot run tests - build failed"
        return 1
    fi
    
    # Run each test
    for test_name in $test_list; do
        local test_exe="${test_name}.exe"
        print_colored "$YELLOW" "  Running $test_exe..."
        
        total=$((total + 1))
        
        # Run test with timeout
        local test_output_file="$REPORT_DIR/${test_name}_output.txt"
        if timeout "$TIMEOUT_DURATION" ./"$test_exe" > "$test_output_file" 2>&1; then
            print_colored "$GREEN" "    ✓ PASSED: $test_exe"
            echo "PASSED: $test_name" >> "$report_file"
            passed=$((passed + 1))
        else
            local exit_code=$?
            print_colored "$RED" "    ✗ FAILED: $test_exe (exit code: $exit_code)"
            echo "FAILED: $test_name (exit code: $exit_code)" >> "$report_file"
            failed=$((failed + 1))
            
            # Log timeout specifically
            if [[ $exit_code -eq 124 ]]; then
                echo "  Reason: Test timed out after $TIMEOUT_DURATION seconds" >> "$report_file"
            fi
        fi
    done
    
    # Write summary to report
    {
        echo ""
        echo "================================================================="
        echo "Test execution completed: $(date)"
        echo "Summary: $passed/$total tests passed, $failed failed"
        echo "Success rate: $(( passed * 100 / total ))%"
    } >> "$report_file"
    
    # Print summary
    print_colored "$CYAN" "$category Tests Summary:"
    print_colored "$GREEN" "  Passed: $passed/$total"
    print_colored "$RED" "  Failed: $failed"
    if [[ $failed -eq 0 ]]; then
        print_colored "$GREEN" "  Success Rate: 100%"
    else
        print_colored "$YELLOW" "  Success Rate: $(( passed * 100 / total ))%"
    fi
    
    log "$category tests completed: $passed/$total passed, $failed failed"
    
    return $failed
}

# Run all test categories
run_all_tests() {
    print_colored "$GREEN" "Running comprehensive test suite..."
    log "Starting comprehensive test execution"
    
    local overall_passed=0
    local overall_failed=0
    local categories=("spatial" "temporal" "realworld" "heap")
    
    for category in "${categories[@]}"; do
        echo ""
        run_test_category "$category"
        local category_failed=$?
        
        if [[ $category_failed -gt 0 ]]; then
            overall_failed=$((overall_failed + category_failed))
        fi
    done
    
    # Generate comprehensive report
    generate_comprehensive_report
    
    # Print overall summary
    echo ""
    print_colored "$GREEN" "================================================================="
    print_colored "$GREEN" "COMPREHENSIVE TEST SUITE COMPLETED"
    print_colored "$GREEN" "================================================================="
    
    if [[ $overall_failed -eq 0 ]]; then
        print_colored "$GREEN" "All tests passed successfully!"
        log "Comprehensive test suite completed successfully"
        return 0
    else
        print_colored "$YELLOW" "Some tests failed. Check detailed reports in $REPORT_DIR"
        log "Comprehensive test suite completed with $overall_failed failures"
        return $overall_failed
    fi
}

# Validate all tests compile
validate_compilation() {
    print_colored "$BLUE" "Validating compilation of all refactored tests..."
    log "Starting compilation validation"
    
    if make -f "$MAKEFILE" validate-build; then
        print_colored "$GREEN" "All refactored tests compile successfully"
        log "Compilation validation successful"
        return 0
    else
        print_colored "$RED" "Some tests failed to compile"
        log "Compilation validation failed"
        return 1
    fi
}

# Check syntax of all source files
check_syntax() {
    print_colored "$PURPLE" "Checking syntax of all refactored source files..."
    log "Starting syntax check"
    
    if make -f "$MAKEFILE" syntax-check; then
        print_colored "$GREEN" "All source files have valid syntax"
        log "Syntax check successful"
        return 0
    else
        print_colored "$RED" "Some source files have syntax errors"
        log "Syntax check failed"
        return 1
    fi
}

# Generate comprehensive report
generate_comprehensive_report() {
    print_colored "$CYAN" "Generating comprehensive test report..."
    log "Generating comprehensive report"
    
    local report_file="$REPORT_DIR/comprehensive_summary.txt"
    
    {
        echo "xBGAS TTU Memory Safety Tests - Comprehensive Summary"
        echo "Generated: $(date)"
        echo "================================================================="
        echo ""
        
        # Test environment info
        echo "TEST ENVIRONMENT:"
        echo "  Directory: $TEST_DIR"
        echo "  Makefile: $MAKEFILE"
        echo "  Timeout: $TIMEOUT_DURATION seconds"
        echo ""
        
        # Category summaries
        for category in spatial temporal realworld heap; do
            local category_report="$REPORT_DIR/${category}_detailed_results.txt"
            if [[ -f "$category_report" ]]; then
                echo "${category^^} TESTS:"
                tail -n 3 "$category_report" | head -n 1
                echo ""
            fi
        done
        
        echo "================================================================="
        echo "Detailed reports available in: $REPORT_DIR"
        echo "Individual test outputs: $REPORT_DIR/*_output.txt"
        echo "Build logs: $REPORT_DIR/build_*.log"
        
    } > "$report_file"
    
    print_colored "$GREEN" "Comprehensive report generated: $report_file"
    log "Comprehensive report generated"
}

# Clean generated files
clean_all() {
    print_colored "$YELLOW" "Cleaning all generated files..."
    log "Starting cleanup"
    
    if make -f "$MAKEFILE" clean; then
        print_colored "$GREEN" "Cleanup completed successfully"
        log "Cleanup successful"
        return 0
    else
        print_colored "$RED" "Cleanup failed"
        log "Cleanup failed"
        return 1
    fi
}

# Parse command line arguments
parse_args() {
    while [[ $# -gt 0 ]]; do
        case $1 in
            --timeout=*)
                TIMEOUT_DURATION="${1#*=}"
                shift
                ;;
            --verbose)
                set -x
                shift
                ;;
            --quiet)
                exec > /dev/null 2>&1
                shift
                ;;
            --no-color)
                RED=''
                GREEN=''
                YELLOW=''
                BLUE=''
                PURPLE=''
                CYAN=''
                NC=''
                shift
                ;;
            *)
                break
                ;;
        esac
    done
}

# Main execution
main() {
    # Parse command line options first
    parse_args "$@"
    
    # Get the command (first non-option argument)
    local command="$1"
    
    # Initialize environment
    initialize
    
    # Print banner
    print_banner
    
    # Execute command
    case $command in
        "build-all")
            build_tests "all"
            ;;
        "build-spatial")
            build_tests "spatial"
            ;;
        "build-temporal")
            build_tests "temporal"
            ;;
        "build-realworld")
            build_tests "realworld"
            ;;
        "build-heap")
            build_tests "heap"
            ;;
        "test-all")
            run_all_tests
            ;;
        "test-spatial")
            run_test_category "spatial"
            ;;
        "test-temporal")
            run_test_category "temporal"
            ;;
        "test-realworld")
            run_test_category "realworld"
            ;;
        "test-heap")
            run_test_category "heap"
            ;;
        "validate-all")
            validate_compilation
            ;;
        "syntax-check")
            check_syntax
            ;;
        "generate-report")
            generate_comprehensive_report
            ;;
        "clean")
            clean_all
            ;;
        "help"|"--help"|"-h"|"")
            print_usage
            ;;
        *)
            print_colored "$RED" "Error: Unknown command '$command'"
            print_usage
            exit 1
            ;;
    esac
    
    local exit_code=$?
    log "Script execution completed with exit code: $exit_code"
    exit $exit_code
}

# Execute main function with all arguments
main "$@"
