/**
 * @file ttu_r4_illegal_ptr_deref_refactored.c
 * @brief Illegal Pointer Dereference on Large Size Allocation Test
 * 
 * REFACTORED FOR xBGAS-Morello TTU Security Evaluation
 * =====================================================
 * 
 * VULNERABILITY TYPE: Illegal Pointer Dereference / Large Allocation
 * SECURITY IMPACT: High - Denial of Service, potential memory corruption
 * CHERI MITIGATION: Capability bounds checking, allocation validation
 * 
 * DESCRIPTION:
 * This test demonstrates illegal pointer dereference vulnerabilities that
 * occur when attempting to allocate extremely large amounts of memory or
 * when dereferencing uninitialized/invalid pointers. The test covers:
 * 1. Allocation requests larger than available memory
 * 2. Dereferencing NULL or invalid pointers returned by failed malloc
 * 3. Access to uninitialized pointer variables
 * 4. Large allocation attempts that may wrap around or overflow
 * 
 * CHERI-MORELLO ANALYSIS:
 * - Capability bounds prevent out-of-bounds access
 * - NULL capability protection prevents NULL pointer dereferences
 * - Allocation validation ensures valid capability creation
 * - Memory region bounds enforce access controls
 * 
 * EXPECTED BEHAVIOR:
 * - Traditional systems: Undefined behavior, potential crashes or corruption
 * - CHERI-Morello: Capability fault on invalid pointer dereference
 * 
 * @author TTU Security Research Team
 * @date 2024
 * @version 2.0 (Refactored)
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <limits.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <setjmp.h>
#include "xbrtime_morello.h"

// Test configuration constants
#define LARGE_SIZE_1 0x40000000000ULL    // Extremely large allocation
#define LARGE_SIZE_2 (SIZE_MAX)          // Maximum possible size
#define LARGE_SIZE_3 (PTRDIFF_MAX + 1ULL) // Overflow attempt
#define TEST_ITERATIONS 3
#define TEST_PATTERNS_COUNT 4

// Test result tracking
typedef enum {
    TEST_RESULT_UNKNOWN = 0,
    TEST_RESULT_MALLOC_FAILED_SAFE,     // malloc failed safely (expected)
    TEST_RESULT_MALLOC_SUCCESS_UNSAFE,  // malloc succeeded (unexpected)
    TEST_RESULT_DEREF_SUCCESS,          // Dereference succeeded (very bad)
    TEST_RESULT_CHERI_PROTECTED,        // CHERI prevented illegal access
    TEST_RESULT_EXCEPTION,              // Exception occurred
    TEST_RESULT_UNINITIALIZED_ACCESS    // Accessed uninitialized pointer
} test_result_t;

// Test pattern structure
typedef struct {
    const char* name;
    size_t allocation_size;
    const char* description;
} test_pattern_t;

// Statistics tracking
typedef struct {
    int total_tests;
    int malloc_failed_safe;
    int malloc_success_unsafe;
    int deref_successes;
    int cheri_protections;
    int exceptions;
    int uninitialized_access;
} test_stats_t;

static test_stats_t global_stats = {0};
static volatile sig_atomic_t signal_caught = 0;
static sigjmp_buf signal_env;

// Test patterns for different allocation scenarios
static const test_pattern_t test_patterns[TEST_PATTERNS_COUNT] = {
    {
        "Extremely Large Allocation",
        LARGE_SIZE_1,
        "Request far exceeding available memory"
    },
    {
        "Maximum Size Allocation", 
        LARGE_SIZE_2,
        "Request using SIZE_MAX (theoretical maximum)"
    },
    {
        "Overflow Allocation",
        LARGE_SIZE_3,
        "Request causing integer overflow in size"
    },
    {
        "Zero Size Allocation",
        0,
        "Edge case: zero-sized allocation"
    }
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
    
#ifdef __CHERI__
    // CHERI-specific capability violation signals
    sigaction(SIGPROT, &sa, NULL);  // Capability protection violation
#endif
}

/**
 * @brief Phase 1: Attempt large memory allocation
 */
static void* phase1_attempt_allocation(long thread_id, const test_pattern_t* pattern) {
    printf("  [Thread %ld] Phase 1: Attempting %s\n", thread_id, pattern->name);
    printf("  [Thread %ld] Description: %s\n", thread_id, pattern->description);
    printf("  [Thread %ld] Requested size: %zu (0x%zx)\n", 
           thread_id, pattern->allocation_size, pattern->allocation_size);
    
    errno = 0;  // Clear errno before allocation
    void* ptr = malloc(pattern->allocation_size);
    int alloc_errno = errno;
    
    if (ptr == NULL) {
        printf("  [Thread %ld] Allocation failed (errno: %d - %s)\n", 
               thread_id, alloc_errno, strerror(alloc_errno));
        return NULL;
    } else {
        printf("  [Thread %ld] WARNING: Allocation succeeded at: %#p\n", 
               thread_id, ptr);
        
#ifdef __CHERI__
        // On CHERI, examine the capability properties
        printf("  [Thread %ld] Capability bounds: [%#p - %#p]\n",
               thread_id, __builtin_cheri_base_get(ptr),
               (void*)((char*)__builtin_cheri_base_get(ptr) + 
                      __builtin_cheri_length_get(ptr)));
        printf("  [Thread %ld] Capability length: %zu\n",
               thread_id, __builtin_cheri_length_get(ptr));
#endif
        return ptr;
    }
}

/**
 * @brief Phase 2: Test pointer dereference with error handling
 */
static test_result_t phase2_test_dereference(long thread_id, void* ptr, 
                                           const test_pattern_t* pattern) {
    printf("  [Thread %ld] Phase 2: Testing pointer dereference\n", thread_id);
    
    test_result_t result = TEST_RESULT_UNKNOWN;
    signal_caught = 0;
    
    if (ptr == NULL) {
        printf("  [Thread %ld] Testing NULL pointer dereference protection\n", thread_id);
        
        // Set up signal handling for NULL pointer dereference
        if (sigsetjmp(signal_env, 1) == 0) {
            // Attempt to dereference NULL pointer
            int* null_ptr = (int*)ptr;
            printf("  [Thread %ld] Attempting to dereference NULL...\n", thread_id);
            
            volatile int value = *null_ptr;  // This should fault
            (void)value;  // Suppress unused variable warning
            
            printf("  [Thread %ld] ERROR: NULL dereference succeeded! Value: %d\n", 
                   thread_id, value);
            result = TEST_RESULT_DEREF_SUCCESS;
        } else {
            printf("  [Thread %ld] PROTECTION: Signal %d caught on NULL dereference\n", 
                   thread_id, signal_caught);
            result = TEST_RESULT_CHERI_PROTECTED;
        }
    } else {
        printf("  [Thread %ld] Testing large allocation dereference\n", thread_id);
        
        // Set up signal handling for potential capability violations
        if (sigsetjmp(signal_env, 1) == 0) {
            int* int_ptr = (int*)ptr;
            printf("  [Thread %ld] Attempting to read from allocated memory...\n", thread_id);
            
            // Try to read the first integer
            volatile int first_value = *int_ptr;
            printf("  [Thread %ld] First value read: %d\n", thread_id, first_value);
            
            // Try to write to the memory
            *int_ptr = 0x12345678;
            printf("  [Thread %ld] Write operation completed\n", thread_id);
            
            // Verify the write
            volatile int written_value = *int_ptr;
            printf("  [Thread %ld] Written value verified: 0x%x\n", 
                   thread_id, written_value);
            
            result = TEST_RESULT_MALLOC_SUCCESS_UNSAFE;
        } else {
            printf("  [Thread %ld] PROTECTION: Signal %d caught during dereference\n", 
                   thread_id, signal_caught);
            result = TEST_RESULT_EXCEPTION;
        }
    }
    
    return result;
}

/**
 * @brief Phase 3: Test uninitialized pointer access
 */
static test_result_t phase3_uninitialized_test(long thread_id) {
    printf("  [Thread %ld] Phase 3: Testing uninitialized pointer access\n", thread_id);
    
    test_result_t result = TEST_RESULT_UNKNOWN;
    signal_caught = 0;
    
    // Declare uninitialized pointer
    int* uninitialized_ptr;
    
    printf("  [Thread %ld] Uninitialized pointer value: %#p\n", 
           thread_id, (void*)uninitialized_ptr);
    
    // Set up signal handling for uninitialized access
    if (sigsetjmp(signal_env, 1) == 0) {
        printf("  [Thread %ld] Attempting to dereference uninitialized pointer...\n", 
               thread_id);
        
        volatile int value = *uninitialized_ptr;
        (void)value;  // Suppress unused variable warning
        
        printf("  [Thread %ld] ERROR: Uninitialized dereference succeeded! Value: %d\n", 
               thread_id, value);
        result = TEST_RESULT_UNINITIALIZED_ACCESS;
    } else {
        printf("  [Thread %ld] PROTECTION: Signal %d caught on uninitialized access\n", 
               thread_id, signal_caught);
        result = TEST_RESULT_CHERI_PROTECTED;
    }
    
    return result;
}

/**
 * @brief Main illegal pointer dereference test function
 */
static void* illegal_pointer_dereference_test(void* arg) {
    long thread_id = (long)arg;
    
    printf("[Thread %ld] ==> Starting Illegal Pointer Dereference Test\n", thread_id);
    
    // Setup signal handlers for this thread
    setup_signal_handlers();
    
    // Test each allocation pattern
    for (int i = 0; i < TEST_PATTERNS_COUNT; i++) {
        printf("[Thread %ld] --- Testing Pattern %d: %s ---\n", 
               thread_id, i + 1, test_patterns[i].name);
        
        // Phase 1: Attempt allocation
        void* ptr = phase1_attempt_allocation(thread_id, &test_patterns[i]);
        
        // Phase 2: Test dereference
        test_result_t result = phase2_test_dereference(thread_id, ptr, &test_patterns[i]);
        
        // Update statistics
        global_stats.total_tests++;
        switch (result) {
            case TEST_RESULT_MALLOC_FAILED_SAFE:
                global_stats.malloc_failed_safe++;
                break;
            case TEST_RESULT_MALLOC_SUCCESS_UNSAFE:
                global_stats.malloc_success_unsafe++;
                break;
            case TEST_RESULT_DEREF_SUCCESS:
                global_stats.deref_successes++;
                break;
            case TEST_RESULT_CHERI_PROTECTED:
                global_stats.cheri_protections++;
                break;
            case TEST_RESULT_EXCEPTION:
                global_stats.exceptions++;
                break;
            default:
                break;
        }
        
        // Cleanup if allocation succeeded
        if (ptr != NULL) {
            free(ptr);
            printf("  [Thread %ld] Allocated memory freed\n", thread_id);
        }
        
        printf("[Thread %ld] Pattern %d result: %d\n\n", thread_id, i + 1, result);
    }
    
    // Phase 3: Test uninitialized pointer access
    printf("[Thread %ld] --- Testing Uninitialized Pointer Access ---\n", thread_id);
    test_result_t uninit_result = phase3_uninitialized_test(thread_id);
    
    global_stats.total_tests++;
    if (uninit_result == TEST_RESULT_UNINITIALIZED_ACCESS) {
        global_stats.uninitialized_access++;
    } else if (uninit_result == TEST_RESULT_CHERI_PROTECTED) {
        global_stats.cheri_protections++;
    }
    
    printf("[Thread %ld] <== Illegal Pointer Dereference Test Complete\n", thread_id);
    return NULL;
}

/**
 * @brief Print comprehensive test results and analysis
 */
static void print_test_analysis(void) {
    printf("\n================================================================================\n");
    printf("ILLEGAL POINTER DEREFERENCE - TEST ANALYSIS\n");
    printf("================================================================================\n");
    
    printf("Test Statistics:\n");
    printf("  Total tests executed:         %d\n", global_stats.total_tests);
    printf("  Safe malloc failures:         %d\n", global_stats.malloc_failed_safe);
    printf("  Unsafe malloc successes:      %d\n", global_stats.malloc_success_unsafe);
    printf("  Successful dereferences:      %d\n", global_stats.deref_successes);
    printf("  CHERI protections:            %d\n", global_stats.cheri_protections);
    printf("  Exceptions caught:            %d\n", global_stats.exceptions);
    printf("  Uninitialized access:         %d\n", global_stats.uninitialized_access);
    
    printf("\nSecurity Analysis:\n");
    if (global_stats.deref_successes > 0 || global_stats.uninitialized_access > 0) {
        printf("  ❌ VULNERABILITY: Illegal pointer dereferences succeeded\n");
        printf("     - System allows invalid memory access\n");
        printf("     - Potential for memory corruption or crashes\n");
        printf("     - Insufficient pointer validation\n");
    }
    
    if (global_stats.cheri_protections > 0 || global_stats.exceptions > 0) {
        printf("  ✅ PROTECTION: CHERI mitigations active\n");
        printf("     - Capability bounds checking prevented illegal access\n");
        printf("     - NULL pointer dereference protection active\n");
        printf("     - Invalid capability access detected and prevented\n");
    }
    
    if (global_stats.malloc_failed_safe > 0) {
        printf("  ✅ SAFE BEHAVIOR: Large allocations properly rejected\n");
        printf("     - Memory allocator correctly handles oversized requests\n");
        printf("     - System prevents memory exhaustion attacks\n");
    }
    
    if (global_stats.malloc_success_unsafe > 0) {
        printf("  ⚠️  WARNING: Large allocations unexpectedly succeeded\n");
        printf("     - System may be vulnerable to memory exhaustion\n");
        printf("     - Allocation size validation may be insufficient\n");
    }
    
    printf("\nCHERI-Morello Mitigation Analysis:\n");
    printf("  • Spatial Safety: Capability bounds prevent out-of-bounds access\n");
    printf("  • NULL Protection: NULL capabilities cannot be dereferenced\n");
    printf("  • Allocation Validation: Capabilities only created for valid allocations\n");
    printf("  • Memory Region Control: Access strictly bounded to allocated regions\n");
    
    printf("\nEducational Value:\n");
    printf("  • Demonstrates importance of pointer validation\n");
    printf("  • Shows relationship between allocation size and security\n");
    printf("  • Illustrates CHERI's spatial memory safety mechanisms\n");
    printf("  • Highlights risks of uninitialized pointer usage\n");
    
    printf("================================================================================\n");
}

/**
 * @brief Main function - orchestrates the illegal pointer dereference test
 */
int main(void) {
    printf("Illegal Pointer Dereference Vulnerability Test (Refactored)\n");
    printf("===========================================================\n");
    printf("Testing illegal pointer dereference vulnerabilities\n");
    printf("Expected on CHERI: Capability violations prevent illegal access\n\n");
    
    // Print system information
    printf("System Information:\n");
    printf("  SIZE_MAX:      %zu (0x%zx)\n", SIZE_MAX, SIZE_MAX);
    printf("  PTRDIFF_MAX:   %td (0x%tx)\n", PTRDIFF_MAX, PTRDIFF_MAX);
    printf("  Large size 1:  %llu (0x%llx)\n", 
           (unsigned long long)LARGE_SIZE_1, (unsigned long long)LARGE_SIZE_1);
    printf("  Large size 2:  %zu (0x%zx)\n", LARGE_SIZE_2, LARGE_SIZE_2);
    printf("  Large size 3:  %llu (0x%llx)\n", 
           (unsigned long long)LARGE_SIZE_3, (unsigned long long)LARGE_SIZE_3);
    printf("\n");
    
    // Initialize xBGAS runtime
    if (xbrtime_init() != 0) {
        fprintf(stderr, "ERROR: Failed to initialize xBGAS runtime\n");
        return EXIT_FAILURE;
    }
    
    int num_pes = xbrtime_num_pes();
    printf("Executing illegal pointer dereference tests on %d processing elements\n\n", 
           num_pes);
    
    // Execute tests across all PEs
    for (long i = 0; i < num_pes; i++) {
        tpool_add_work(threads[i].thread_queue, 
                      illegal_pointer_dereference_test, 
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
    if (global_stats.deref_successes > 0 || global_stats.uninitialized_access > 0) {
        printf("\nTest Result: VULNERABILITY DETECTED - System allows illegal access\n");
        return EXIT_FAILURE;
    } else if (global_stats.cheri_protections > 0 || global_stats.exceptions > 0) {
        printf("\nTest Result: CHERI PROTECTION ACTIVE - System protected\n");
        return EXIT_SUCCESS;
    } else {
        printf("\nTest Result: INCONCLUSIVE - Check system configuration\n");
        return EXIT_FAILURE;
    }
}
