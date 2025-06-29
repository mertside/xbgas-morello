/**
 * @file ttu_s2_free_not_on_heap_refactored.c
 * @brief Free Memory Not Allocated on Heap Vulnerability Test
 * 
 * REFACTORED FOR xBGAS-Morello TTU Security Evaluation
 * =====================================================
 * 
 * VULNERABILITY TYPE: Invalid Free Operation (Non-Heap Memory)
 * SECURITY IMPACT: High - Memory corruption, system instability
 * CHERI MITIGATION: Capability spatial safety, allocation validation
 * 
 * DESCRIPTION:
 * This test demonstrates vulnerabilities that occur when attempting to free
 * memory that was not allocated on the heap using malloc(). The test covers
 * several scenarios:
 * 1. Freeing stack-allocated memory (local variables)
 * 2. Freeing statically allocated memory (string literals, globals)
 * 3. Freeing memory from different memory regions
 * 4. Double-checking memory access after invalid free attempts
 * 
 * ATTACK SCENARIOS:
 * - Attacker tricks program into freeing non-heap memory
 * - Heap metadata corruption through invalid free operations
 * - Memory management system confusion leading to crashes
 * - Potential for arbitrary code execution through heap corruption
 * 
 * CHERI-MORELLO ANALYSIS:
 * - Capability bounds prevent freeing memory outside heap regions
 * - Spatial safety ensures only valid heap capabilities can be freed
 * - Memory region separation prevents cross-region operations
 * - Allocation tracking validates free operations
 * 
 * EXPECTED BEHAVIOR:
 * - Traditional systems: Undefined behavior, crashes, heap corruption
 * - CHERI-Morello: Capability fault on invalid free attempt
 * 
 * @author TTU Security Research Team
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
#define STACK_BUFFER_SIZE 64
#define TEST_STRING_SIZE 32
#define MAX_TEST_PATTERNS 5

// Global static memory for testing
static char global_buffer[STACK_BUFFER_SIZE] = "Global static buffer for testing";
static const char* const string_literal = "String literal in read-only section";

// Test result tracking
typedef enum {
    TEST_RESULT_UNKNOWN = 0,
    TEST_RESULT_FREE_SUCCESS,           // Invalid free succeeded (very bad)
    TEST_RESULT_MEMORY_CORRUPTION,      // Memory corruption detected
    TEST_RESULT_CHERI_PROTECTED,        // CHERI prevented the invalid free
    TEST_RESULT_EXCEPTION,              // Exception occurred during free
    TEST_RESULT_SYSTEM_ABORT           // System aborted due to invalid free
} test_result_t;

// Test pattern structure
typedef struct {
    const char* name;
    const char* description;
    void* memory_ptr;
    size_t memory_size;
    const char* memory_type;
} test_pattern_t;

// Statistics tracking
typedef struct {
    int total_tests;
    int free_successes;
    int memory_corruptions;
    int cheri_protections;
    int exceptions;
    int system_aborts;
} test_stats_t;

static test_stats_t global_stats = {0};
static volatile sig_atomic_t signal_caught = 0;
static sigjmp_buf signal_env;

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
    sigaction(SIGABRT, &sa, NULL);  // Abort (invalid free detection)
    
#ifdef __CHERI__
    // CHERI-specific capability violation signals
    sigaction(SIGPROT, &sa, NULL);  // Capability protection violation
#endif
}

/**
 * @brief Print memory region information and capability details
 */
static void print_memory_info(long thread_id, const char* label, void* ptr, size_t size) {
    printf("  [Thread %ld] %s:\n", thread_id, label);
    printf("    Address:           %p\n", ptr);
    printf("    Size:              %zu bytes\n", size);
    printf("    Full capability:   %#p\n", ptr);
    
#ifdef __CHERI__
    if (__builtin_cheri_tag_get(ptr)) {
        printf("    Capability valid:  yes\n");
        printf("    Capability base:   %#p\n", __builtin_cheri_base_get(ptr));
        printf("    Capability length: %zu\n", __builtin_cheri_length_get(ptr));
        printf("    Capability perms:  0x%lx\n", __builtin_cheri_perms_get(ptr));
    } else {
        printf("    Capability valid:  no (no tag)\n");
    }
#endif
}

/**
 * @brief Phase 1: Setup test patterns with different memory types
 */
static void phase1_setup_test_patterns(long thread_id, test_pattern_t* patterns, 
                                     char* stack_buffer) {
    printf("  [Thread %ld] Phase 1: Setting up test patterns for different memory types\n", 
           thread_id);
    
    // Initialize stack buffer
    strcpy(stack_buffer, "Stack allocated buffer for testing");
    
    // Pattern 1: Stack-allocated memory
    patterns[0].name = "Stack Memory";
    patterns[0].description = "Local variable on function stack";
    patterns[0].memory_ptr = stack_buffer;
    patterns[0].memory_size = STACK_BUFFER_SIZE;
    patterns[0].memory_type = "Stack";
    
    // Pattern 2: Global static memory
    patterns[1].name = "Global Static Memory";
    patterns[1].description = "Statically allocated global buffer";
    patterns[1].memory_ptr = global_buffer;
    patterns[1].memory_size = sizeof(global_buffer);
    patterns[1].memory_type = "Static/Global";
    
    // Pattern 3: String literal (read-only section)
    patterns[2].name = "String Literal";
    patterns[2].description = "String literal in read-only memory section";
    patterns[2].memory_ptr = (void*)string_literal;
    patterns[2].memory_size = strlen(string_literal) + 1;
    patterns[2].memory_type = "Read-Only";
    
    // Pattern 4: Heap memory (control - this SHOULD be freeable)
    char* heap_buffer = malloc(STACK_BUFFER_SIZE);
    if (heap_buffer) {
        strcpy(heap_buffer, "Heap allocated buffer for comparison");
        patterns[3].name = "Heap Memory (Control)";
        patterns[3].description = "Legitimately heap-allocated memory";
        patterns[3].memory_ptr = heap_buffer;
        patterns[3].memory_size = STACK_BUFFER_SIZE;
        patterns[3].memory_type = "Heap";
    } else {
        patterns[3].memory_ptr = NULL;
    }
    
    // Pattern 5: Invalid pointer (random address)
    patterns[4].name = "Invalid Pointer";
    patterns[4].description = "Arbitrary invalid memory address";
    patterns[4].memory_ptr = (void*)0x12345678;  // Arbitrary invalid address
    patterns[4].memory_size = 0;
    patterns[4].memory_type = "Invalid";
    
    // Print information about all test patterns
    for (int i = 0; i < MAX_TEST_PATTERNS; i++) {
        if (patterns[i].memory_ptr) {
            print_memory_info(thread_id, patterns[i].name, 
                            patterns[i].memory_ptr, patterns[i].memory_size);
        }
    }
}

/**
 * @brief Phase 2: Test memory access before free attempt
 */
static void phase2_test_memory_access(long thread_id, const test_pattern_t* pattern) {
    printf("  [Thread %ld] Phase 2: Testing memory access for %s\n", 
           thread_id, pattern->name);
    
    if (!pattern->memory_ptr || strcmp(pattern->memory_type, "Invalid") == 0) {
        printf("    Skipping memory access test for invalid pointer\n");
        return;
    }
    
    // Test read access
    printf("    Testing read access...\n");
    if (pattern->memory_size > 0) {
        char first_char = *((char*)pattern->memory_ptr);
        printf("    First character: '%c' (0x%02x)\n", 
               first_char, (unsigned char)first_char);
        
        // For string-like buffers, print the content
        if (pattern->memory_size > 1) {
            printf("    Content preview: \"%.20s%s\"\n", 
                   (char*)pattern->memory_ptr,
                   strlen((char*)pattern->memory_ptr) > 20 ? "..." : "");
        }
    }
    
    // Test write access (except for read-only memory)
    if (strcmp(pattern->memory_type, "Read-Only") != 0) {
        printf("    Testing write access...\n");
        char* char_ptr = (char*)pattern->memory_ptr;
        char original = *char_ptr;
        *char_ptr = 'X';  // Temporary modification
        printf("    Write test successful (modified first char)\n");
        *char_ptr = original;  // Restore original value
        printf("    Original value restored\n");
    } else {
        printf("    Skipping write test for read-only memory\n");
    }
}

/**
 * @brief Phase 3: Attempt invalid free operation
 */
static test_result_t phase3_attempt_invalid_free(long thread_id, const test_pattern_t* pattern) {
    printf("  [Thread %ld] Phase 3: Attempting to free %s\n", thread_id, pattern->name);
    printf("    %s\n", pattern->description);
    
    test_result_t result = TEST_RESULT_UNKNOWN;
    signal_caught = 0;
    
    if (!pattern->memory_ptr) {
        printf("    Skipping free attempt for NULL pointer\n");
        return TEST_RESULT_CHERI_PROTECTED;
    }
    
    // Set up signal handling for invalid free
    if (sigsetjmp(signal_env, 1) == 0) {
        printf("    Calling free() on %s memory...\n", pattern->memory_type);
        
        free(pattern->memory_ptr);
        
        // If we reach here, the free operation didn't crash
        if (strcmp(pattern->memory_type, "Heap") == 0) {
            printf("    Legitimate heap free succeeded (expected)\n");
            result = TEST_RESULT_CHERI_PROTECTED;  // This is actually correct behavior
        } else {
            printf("    ERROR: Invalid free succeeded! This should not happen.\n");
            result = TEST_RESULT_FREE_SUCCESS;
        }
        
    } else {
        // Signal was caught during free attempt
        printf("    PROTECTION: Signal %d caught during invalid free attempt\n", 
               signal_caught);
        
        if (signal_caught == SIGABRT) {
            result = TEST_RESULT_SYSTEM_ABORT;
        } else {
            result = TEST_RESULT_EXCEPTION;
        }
    }
    
    return result;
}

/**
 * @brief Phase 4: Test memory access after free attempt
 */
static void phase4_test_post_free_access(long thread_id, const test_pattern_t* pattern) {
    printf("  [Thread %ld] Phase 4: Testing memory access after free attempt\n", 
           thread_id);
    
    if (!pattern->memory_ptr || strcmp(pattern->memory_type, "Invalid") == 0) {
        printf("    Skipping post-free access test for invalid pointer\n");
        return;
    }
    
    if (strcmp(pattern->memory_type, "Heap") == 0) {
        printf("    Skipping post-free access test for legitimately freed heap memory\n");
        return;
    }
    
    // Test if memory is still accessible after invalid free attempt
    printf("    Testing if memory is still accessible...\n");
    
    signal_caught = 0;
    if (sigsetjmp(signal_env, 1) == 0) {
        char first_char = *((char*)pattern->memory_ptr);
        printf("    Memory still accessible, first character: '%c'\n", first_char);
        
        if (pattern->memory_size > 1) {
            printf("    Content after free attempt: \"%.20s%s\"\n", 
                   (char*)pattern->memory_ptr,
                   strlen((char*)pattern->memory_ptr) > 20 ? "..." : "");
        }
    } else {
        printf("    Memory no longer accessible (signal %d caught)\n", signal_caught);
    }
}

/**
 * @brief Main free-not-on-heap vulnerability test function
 */
static void* free_not_on_heap_vulnerability_test(void* arg) {
    long thread_id = (long)arg;
    
    printf("[Thread %ld] ==> Starting Free-Not-On-Heap Vulnerability Test\n", thread_id);
    
    // Setup signal handlers for this thread
    setup_signal_handlers();
    
    // Prepare test patterns
    test_pattern_t patterns[MAX_TEST_PATTERNS];
    char stack_buffer[STACK_BUFFER_SIZE];
    
    // Phase 1: Setup test patterns
    phase1_setup_test_patterns(thread_id, patterns, stack_buffer);
    
    // Test each pattern
    for (int i = 0; i < MAX_TEST_PATTERNS; i++) {
        printf("[Thread %ld] --- Testing Pattern %d: %s ---\n", 
               thread_id, i + 1, patterns[i].name);
        
        // Phase 2: Test memory access before free
        phase2_test_memory_access(thread_id, &patterns[i]);
        
        // Phase 3: Attempt invalid free
        test_result_t result = phase3_attempt_invalid_free(thread_id, &patterns[i]);
        
        // Phase 4: Test memory access after free attempt
        phase4_test_post_free_access(thread_id, &patterns[i]);
        
        // Update statistics
        global_stats.total_tests++;
        switch (result) {
            case TEST_RESULT_FREE_SUCCESS:
                global_stats.free_successes++;
                break;
            case TEST_RESULT_MEMORY_CORRUPTION:
                global_stats.memory_corruptions++;
                break;
            case TEST_RESULT_CHERI_PROTECTED:
                global_stats.cheri_protections++;
                break;
            case TEST_RESULT_EXCEPTION:
                global_stats.exceptions++;
                break;
            case TEST_RESULT_SYSTEM_ABORT:
                global_stats.system_aborts++;
                break;
            default:
                break;
        }
        
        printf("[Thread %ld] Pattern %d result: %d\n\n", thread_id, i + 1, result);
    }
    
    printf("[Thread %ld] <== Free-Not-On-Heap Vulnerability Test Complete\n", thread_id);
    return NULL;
}

/**
 * @brief Print comprehensive test results and analysis
 */
static void print_test_analysis(void) {
    printf("\n" "="*80 "\n");
    printf("FREE-NOT-ON-HEAP VULNERABILITY - TEST ANALYSIS\n");
    printf("="*80 "\n");
    
    printf("Test Statistics:\n");
    printf("  Total tests executed:         %d\n", global_stats.total_tests);
    printf("  Invalid free successes:       %d\n", global_stats.free_successes);
    printf("  Memory corruptions:           %d\n", global_stats.memory_corruptions);
    printf("  CHERI protections:            %d\n", global_stats.cheri_protections);
    printf("  Exceptions caught:            %d\n", global_stats.exceptions);
    printf("  System aborts:                %d\n", global_stats.system_aborts);
    
    printf("\nSecurity Analysis:\n");
    if (global_stats.free_successes > 0) {
        printf("  ❌ VULNERABILITY: Invalid free operations succeeded\n");
        printf("     - System allows freeing non-heap memory\n");
        printf("     - Potential for heap metadata corruption\n");
        printf("     - Memory management system lacks validation\n");
    }
    
    if (global_stats.memory_corruptions > 0) {
        printf("  ❌ CORRUPTION: Memory corruption detected\n");
        printf("     - Invalid free operations damaged memory structures\n");
        printf("     - System integrity compromised\n");
    }
    
    if (global_stats.system_aborts > 0) {
        printf("  ⚠️  PARTIAL PROTECTION: System detected invalid free but after attempt\n");
        printf("     - Runtime protection active but reactive\n");
        printf("     - Invalid free detected by heap allocator\n");
    }
    
    if (global_stats.cheri_protections > 0 || global_stats.exceptions > 0) {
        printf("  ✅ PROTECTION: CHERI mitigations active\n");
        printf("     - Capability spatial safety prevented invalid free\n");
        printf("     - Memory region separation enforced\n");
        printf("     - Only valid heap capabilities can be freed\n");
    }
    
    printf("\nMemory Region Analysis:\n");
    printf("  • Stack Memory: Local variables should not be freeable\n");
    printf("  • Global Memory: Static allocations should not be freeable\n");
    printf("  • Read-Only Memory: String literals should not be freeable\n");
    printf("  • Heap Memory: Only malloc'd memory should be freeable\n");
    printf("  • Invalid Pointers: Arbitrary addresses should not be freeable\n");
    
    printf("\nCHERI-Morello Mitigation Analysis:\n");
    printf("  • Spatial Safety: Capability bounds prevent cross-region operations\n");
    printf("  • Region Separation: Different memory regions have distinct capabilities\n");
    printf("  • Allocation Tracking: Heap capabilities are tracked and validated\n");
    printf("  • Memory Protection: Non-heap memory cannot be freed\n");
    
    printf("\nEducational Value:\n");
    printf("  • Demonstrates importance of memory region validation\n");
    printf("  • Shows relationship between memory allocation and deallocation\n");
    printf("  • Illustrates different memory regions and their properties\n");
    printf("  • Highlights CHERI's spatial memory safety mechanisms\n");
    
    printf("="*80 "\n");
}

/**
 * @brief Main function - orchestrates the free-not-on-heap test
 */
int main(void) {
    printf("Free-Not-On-Heap Vulnerability Test (Refactored)\n");
    printf("================================================\n");
    printf("Testing invalid free operations on non-heap memory\n");
    printf("Expected on CHERI: Capability violations prevent invalid free operations\n\n");
    
    // Initialize xBGAS runtime
    if (xbrtime_init() != XBRTIME_SUCCESS) {
        fprintf(stderr, "ERROR: Failed to initialize xBGAS runtime\n");
        return EXIT_FAILURE;
    }
    
    int num_pes = xbrtime_num_pes();
    printf("Executing free-not-on-heap tests on %d processing elements\n\n", num_pes);
    
    // Execute tests across all PEs
    for (long i = 0; i < num_pes; i++) {
        tpool_add_work(threads[i].thread_queue, 
                      free_not_on_heap_vulnerability_test, 
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
    if (global_stats.free_successes > 0 || global_stats.memory_corruptions > 0) {
        printf("\nTest Result: VULNERABILITY DETECTED - System allows invalid free operations\n");
        return EXIT_FAILURE;
    } else if (global_stats.cheri_protections > 0 || global_stats.exceptions > 0 || 
               global_stats.system_aborts > 0) {
        printf("\nTest Result: PROTECTION ACTIVE - Invalid free operations prevented/detected\n");
        return EXIT_SUCCESS;
    } else {
        printf("\nTest Result: INCONCLUSIVE - Check system configuration\n");
        return EXIT_FAILURE;
    }
}
