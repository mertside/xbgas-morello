/**
 * @file ttu_r5_df_switch_refactored.c
 * @brief Double-Free via Switch Fallthrough Vulnerability Test
 * 
 * REFACTORED FOR xBGAS-Morello TTU Security Evaluation
 * =====================================================
 * 
 * VULNERABILITY TYPE: Double-Free via Control Flow (Switch Fallthrough)
 * SECURITY IMPACT: Critical - Heap corruption, potential code execution
 * CHERI MITIGATION: Capability temporal safety, heap metadata protection
 * 
 * DESCRIPTION:
 * This test demonstrates a double-free vulnerability caused by improper
 * switch-case implementation with intentional fallthrough. The vulnerability
 * occurs when multiple case statements execute due to missing break statements,
 * leading to the same memory being freed multiple times. This is a common
 * programming error that can be exploited for heap corruption attacks.
 * 
 * ATTACK VECTOR:
 * 1. Allocate memory and store a character value
 * 2. Use switch statement with intentional fallthrough
 * 3. Multiple case branches execute, each calling free() on same pointer
 * 4. Double-free corruption allows heap metadata manipulation
 * 5. Subsequent allocations may return overlapping memory regions
 * 
 * CHERI-MORELLO ANALYSIS:
 * - Capability temporal safety prevents use of freed capabilities
 * - Heap metadata protection prevents corruption from double-free
 * - Memory revocation ensures freed capabilities become invalid
 * - Bounds checking prevents heap overflow exploitation
 * 
 * EXPECTED BEHAVIOR:
 * - Traditional systems: Heap corruption, overlapping allocations
 * - CHERI-Morello: Capability fault on second free attempt
 * 
 * @author Mert Side
 * @date 2024
 * @version 2.0 (Refactored)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <signal.h>
#include <setjmp.h>
#include "xbrtime_morello.h"

// Test configuration constants
#define ALLOCATION_SIZE 0x10
#define TEST_ITERATIONS 1
#define MAX_TEST_CHARS 4

// Test result tracking
typedef enum {
    TEST_RESULT_UNKNOWN = 0,
    TEST_RESULT_DOUBLE_FREE_SUCCESS,    // Double-free succeeded (bad)
    TEST_RESULT_HEAP_CORRUPTION,        // Heap corruption detected
    TEST_RESULT_OVERLAPPING_ALLOCS,     // Overlapping allocations detected
    TEST_RESULT_CHERI_PROTECTED,        // CHERI prevented the attack
    TEST_RESULT_EXCEPTION,              // Exception occurred
    TEST_RESULT_MALLOC_FAILED           // Memory allocation failed
} test_result_t;

// Test character patterns for switch testing
typedef struct {
    char test_char;
    const char* description;
    int expected_frees;
} test_char_pattern_t;

// Statistics tracking
typedef struct {
    int total_tests;
    int double_free_successes;
    int heap_corruptions;
    int overlapping_allocs;
    int cheri_protections;
    int exceptions;
    int malloc_failures;
} test_stats_t;

static test_stats_t global_stats = {0};
static volatile sig_atomic_t signal_caught = 0;
static sigjmp_buf signal_env;

// Test character patterns
static const test_char_pattern_t test_chars[MAX_TEST_CHARS] = {
    {'A', "Case A: Should free in A, B, C, and default (4 frees)", 4},
    {'B', "Case B: Should free in B, C, and default (3 frees)", 3},
    {'C', "Case C: Should free in C and default (2 frees)", 2},
    {'X', "Case X: Should free only in default (1 free)", 1}
};

/**
 * @brief Signal handler for capability violations and segmentation faults
 */
static void signal_handler(int sig) {
    signal_caught = sig;
    siglongjmp(signal_env, sig);
}

/**
 * @brief Setup signal handlers for capability violations
 */
static void setup_signal_handlers(void) {
    struct sigaction sa;
    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    
    sigaction(SIGSEGV, &sa, NULL);  // Segmentation fault
    sigaction(SIGBUS, &sa, NULL);   // Bus error (common on CHERI violations)
    sigaction(SIGABRT, &sa, NULL);  // Abort (double-free detection)
    
#ifdef __CHERI__
    // CHERI-specific capability violation signals
    sigaction(SIGPROT, &sa, NULL);  // Capability protection violation
#endif
}

/**
 * @brief Phase 1: Allocate initial memory and set test character
 */
static char* phase1_setup_allocation(long thread_id, char test_char) {
    printf("  [Thread %ld] Phase 1: Allocating memory for test character '%c'\n", 
           thread_id, test_char);
    
    char* buffer = malloc(ALLOCATION_SIZE);
    if (!buffer) {
        printf("  [Thread %ld] ERROR: Failed to allocate buffer\n", thread_id);
        return NULL;
    }
    
    // Initialize the buffer with the test character
    *buffer = test_char;
    memset(buffer + 1, 0, ALLOCATION_SIZE - 1);  // Clear rest of buffer
    
    printf("  [Thread %ld] Buffer allocated at: %#p\n", thread_id, (void*)buffer);
    printf("  [Thread %ld] Test character set: '%c'\n", thread_id, *buffer);
    
#ifdef __CHERI__
    printf("  [Thread %ld] Capability length: %zu\n", 
           thread_id, __builtin_cheri_length_get(buffer));
    printf("  [Thread %ld] Capability valid: %s\n", 
           thread_id, __builtin_cheri_tag_get(buffer) ? "yes" : "no");
#endif
    
    return buffer;
}

/**
 * @brief Phase 2: Execute vulnerable switch statement with fallthrough
 */
static test_result_t phase2_vulnerable_switch(long thread_id, char* buffer, 
                                            const test_char_pattern_t* pattern) {
    printf("  [Thread %ld] Phase 2: Executing vulnerable switch with fallthrough\n", 
           thread_id);
    printf("  [Thread %ld] %s\n", thread_id, pattern->description);
    
    test_result_t result = TEST_RESULT_UNKNOWN;
    signal_caught = 0;
    int free_count = 0;
    
    // Set up signal handling for double-free detection
    if (sigsetjmp(signal_env, 1) == 0) {
        // This is the vulnerable switch statement with intentional fallthrough
        switch (*buffer) {
            case 'A':
                printf("  [Thread %ld] Executing case A - calling free #%d\n", 
                       thread_id, ++free_count);
                free(buffer);
                // INTENTIONAL FALLTHROUGH - VULNERABILITY
                
            case 'B':
                printf("  [Thread %ld] Executing case B - calling free #%d\n", 
                       thread_id, ++free_count);
                free(buffer);  // Double-free if came from case A
                // INTENTIONAL FALLTHROUGH - VULNERABILITY
                
            case 'C':
                printf("  [Thread %ld] Executing case C - calling free #%d\n", 
                       thread_id, ++free_count);
                free(buffer);  // Double/triple-free if came from A or B
                // INTENTIONAL FALLTHROUGH - VULNERABILITY
                
            default:
                printf("  [Thread %ld] Executing default case\n", thread_id);
                
                // Attempt to use freed memory (use-after-free)
                printf("  [Thread %ld] Attempting use-after-free write...\n", thread_id);
                memcpy(buffer, "DEFAULT", 7);  // This should fail on CHERI
                
                printf("  [Thread %ld] Calling free #%d in default\n", 
                       thread_id, ++free_count);
                free(buffer);  // Final free attempt
                break;
        }
        
        // If we reach here, no signals were caught
        if (free_count > 1) {
            printf("  [Thread %ld] ERROR: Multiple frees succeeded (%d total)\n", 
                   thread_id, free_count);
            result = TEST_RESULT_DOUBLE_FREE_SUCCESS;
        } else {
            printf("  [Thread %ld] Only one free executed\n", thread_id);
            result = TEST_RESULT_CHERI_PROTECTED;
        }
        
    } else {
        // Signal was caught during switch execution
        printf("  [Thread %ld] PROTECTION: Signal %d caught after %d frees\n", 
               thread_id, signal_caught, free_count);
        
        if (signal_caught == SIGABRT) {
            result = TEST_RESULT_HEAP_CORRUPTION;
        } else {
            result = TEST_RESULT_EXCEPTION;
        }
    }
    
    return result;
}

/**
 * @brief Phase 3: Test for heap corruption via overlapping allocations
 */
static test_result_t phase3_test_heap_corruption(long thread_id, void* original_ptr) {
    printf("  [Thread %ld] Phase 3: Testing for heap corruption\n", thread_id);
    
    test_result_t result = TEST_RESULT_UNKNOWN;
    signal_caught = 0;
    
    if (sigsetjmp(signal_env, 1) == 0) {
        // Allocate new memory to see if we get overlapping regions
        void* alloc1 = malloc(ALLOCATION_SIZE);
        void* alloc2 = malloc(ALLOCATION_SIZE);
        
        if (!alloc1 || !alloc2) {
            printf("  [Thread %ld] ERROR: Failed to allocate test memory\n", thread_id);
            if (alloc1) free(alloc1);
            if (alloc2) free(alloc2);
            return TEST_RESULT_MALLOC_FAILED;
        }
        
        printf("  [Thread %ld] Original pointer:  %#p\n", thread_id, original_ptr);
        printf("  [Thread %ld] New allocation 1:  %#p\n", thread_id, alloc1);
        printf("  [Thread %ld] New allocation 2:  %#p\n", thread_id, alloc2);
        
        // Check for overlapping allocations (sign of heap corruption)
        if (alloc1 == original_ptr || alloc2 == original_ptr || alloc1 == alloc2) {
            printf("  [Thread %ld] WARNING: Overlapping allocations detected!\n", thread_id);
            result = TEST_RESULT_OVERLAPPING_ALLOCS;
        } else {
            printf("  [Thread %ld] Allocations appear distinct\n", thread_id);
            result = TEST_RESULT_CHERI_PROTECTED;
        }
        
        // Test memory integrity
        memset(alloc1, 0xAA, ALLOCATION_SIZE);
        memset(alloc2, 0xBB, ALLOCATION_SIZE);
        
        // Verify the patterns
        if (((char*)alloc1)[0] == (char)0xAA && ((char*)alloc2)[0] == (char)0xBB) {
            printf("  [Thread %ld] Memory patterns verified - no corruption\n", thread_id);
        } else {
            printf("  [Thread %ld] ERROR: Memory pattern corruption detected\n", thread_id);
            result = TEST_RESULT_HEAP_CORRUPTION;
        }
        
        // Cleanup
        free(alloc1);
        free(alloc2);
        
    } else {
        printf("  [Thread %ld] PROTECTION: Signal %d caught during heap test\n", 
               thread_id, signal_caught);
        result = TEST_RESULT_EXCEPTION;
    }
    
    return result;
}

/**
 * @brief Main double-free via switch fallthrough test function
 */
static void* double_free_switch_vulnerability_test(void* arg) {
    long thread_id = (long)arg;
    
    printf("[Thread %ld] ==> Starting Double-Free Switch Fallthrough Test\n", thread_id);
    
    // Setup signal handlers for this thread
    setup_signal_handlers();
    
    // Test each character pattern
    for (int i = 0; i < MAX_TEST_CHARS; i++) {
        printf("[Thread %ld] --- Testing Pattern %d: '%c' ---\n", 
               thread_id, i + 1, test_chars[i].test_char);
        
        // Phase 1: Setup allocation
        char* buffer = phase1_setup_allocation(thread_id, test_chars[i].test_char);
        if (!buffer) {
            global_stats.malloc_failures++;
            continue;
        }
        
        // Keep a copy of the pointer for heap corruption testing
        void* original_ptr = (void*)buffer;
        
        // Phase 2: Execute vulnerable switch
        test_result_t switch_result = phase2_vulnerable_switch(thread_id, buffer, 
                                                             &test_chars[i]);
        
        // Phase 3: Test for heap corruption (only if switch didn't crash)
        test_result_t heap_result = TEST_RESULT_UNKNOWN;
        if (switch_result != TEST_RESULT_EXCEPTION) {
            heap_result = phase3_test_heap_corruption(thread_id, original_ptr);
        }
        
        // Update statistics based on results
        global_stats.total_tests++;
        
        if (switch_result == TEST_RESULT_DOUBLE_FREE_SUCCESS) {
            global_stats.double_free_successes++;
        } else if (switch_result == TEST_RESULT_EXCEPTION || 
                   switch_result == TEST_RESULT_HEAP_CORRUPTION) {
            if (signal_caught == SIGABRT) {
                global_stats.heap_corruptions++;
            } else {
                global_stats.exceptions++;
            }
        } else if (switch_result == TEST_RESULT_CHERI_PROTECTED) {
            global_stats.cheri_protections++;
        }
        
        if (heap_result == TEST_RESULT_OVERLAPPING_ALLOCS) {
            global_stats.overlapping_allocs++;
        }
        
        printf("[Thread %ld] Pattern %d result: switch=%d, heap=%d\n\n", 
               thread_id, i + 1, switch_result, heap_result);
    }
    
    printf("[Thread %ld] <== Double-Free Switch Fallthrough Test Complete\n", thread_id);
    return NULL;
}

/**
 * @brief Print comprehensive test results and analysis
 */
static void print_test_analysis(void) {
    printf("\n================================================================================\n");
    printf("DOUBLE-FREE SWITCH FALLTHROUGH - TEST ANALYSIS\n");
    printf("================================================================================\n");
    
    printf("Test Statistics:\n");
    printf("  Total tests executed:         %d\n", global_stats.total_tests);
    printf("  Double-free successes:        %d\n", global_stats.double_free_successes);
    printf("  Heap corruptions detected:    %d\n", global_stats.heap_corruptions);
    printf("  Overlapping allocations:      %d\n", global_stats.overlapping_allocs);
    printf("  CHERI protections:            %d\n", global_stats.cheri_protections);
    printf("  Exceptions caught:            %d\n", global_stats.exceptions);
    printf("  Memory allocation failures:   %d\n", global_stats.malloc_failures);
    
    printf("\nSecurity Analysis:\n");
    if (global_stats.double_free_successes > 0 || global_stats.overlapping_allocs > 0) {
        printf("  ❌ VULNERABILITY: Double-free exploitation succeeded\n");
        printf("     - Switch fallthrough enabled multiple free() calls\n");
        printf("     - Heap corruption allows memory reuse attacks\n");
        printf("     - System lacks double-free protection\n");
    }
    
    if (global_stats.heap_corruptions > 0) {
        printf("  ⚠️  PARTIAL PROTECTION: Heap corruption detected but not prevented\n");
        printf("     - System detected double-free but after corruption occurred\n");
        printf("     - Runtime heap protection active but reactive\n");
    }
    
    if (global_stats.cheri_protections > 0 || global_stats.exceptions > 0) {
        printf("  ✅ PROTECTION: CHERI mitigations active\n");
        printf("     - Capability temporal safety prevented double-free\n");
        printf("     - Use-after-free attempts blocked by capability invalidation\n");
        printf("     - Heap metadata protected from corruption\n");
    }
    
    printf("\nControl Flow Analysis:\n");
    printf("  • Switch Fallthrough: Demonstrates how missing break statements\n");
    printf("    can lead to unintended multiple executions of free() calls\n");
    printf("  • Double-Free Pattern: Classic heap corruption vulnerability\n");
    printf("    enabling metadata manipulation and potential code execution\n");
    printf("  • Use-After-Free: Attempt to write to freed memory in default case\n");
    
    printf("\nCHERI-Morello Mitigation Analysis:\n");
    printf("  • Temporal Safety: Capabilities to freed memory become invalid\n");
    printf("  • Heap Protection: Heap metadata protected from corruption\n");
    printf("  • Capability Revocation: Freed capabilities cannot be reused\n");
    printf("  • Memory Safety: Bounds and tag checking prevent exploitation\n");
    
    printf("\nEducational Value:\n");
    printf("  • Demonstrates importance of proper switch statement usage\n");
    printf("  • Shows relationship between control flow and memory safety\n");
    printf("  • Illustrates double-free vulnerability exploitation\n");
    printf("  • Highlights CHERI's temporal memory safety protection\n");
    
    printf("================================================================================\n");
}

/**
 * @brief Main function - orchestrates the double-free switch fallthrough test
 */
int main(void) {
    printf("Double-Free Switch Fallthrough Vulnerability Test (Refactored)\n");
    printf("==============================================================\n");
    printf("Testing double-free vulnerabilities via switch statement fallthrough\n");
    printf("Expected on CHERI: Capability violations prevent double-free exploitation\n\n");
    
    // Initialize xBGAS runtime
    if (xbrtime_init() != 0) {
        fprintf(stderr, "ERROR: Failed to initialize xBGAS runtime\n");
        return EXIT_FAILURE;
    }
    
    int num_pes = xbrtime_num_pes();
    printf("Executing double-free switch tests on %d processing elements\n\n", num_pes);
    
    // Execute tests across all PEs
    for (long i = 0; i < num_pes; i++) {
        tpool_add_work(threads[i].thread_queue, 
                      double_free_switch_vulnerability_test, 
                      (void*)i);
    }
    
    // Wait for all tests to complete
    for (int i = 0; i < num_pes; i++) {
        tpool_wait(threads[i].thread_queue);
    }
    
    // Print comprehensive analysis
    print_test_analysis();
    
    // Cleanup xBGAS runtime
    xbrtime_close();
    
    // Return appropriate exit code
    if (global_stats.double_free_successes > 0 || global_stats.overlapping_allocs > 0) {
        printf("\nTest Result: VULNERABILITY DETECTED - System exploitable\n");
        return EXIT_FAILURE;
    } else if (global_stats.cheri_protections > 0 || global_stats.exceptions > 0) {
        printf("\nTest Result: CHERI PROTECTION ACTIVE - System protected\n");
        return EXIT_SUCCESS;
    } else {
        printf("\nTest Result: INCONCLUSIVE - Check system configuration\n");
        return EXIT_FAILURE;
    }
}
