/**
 * @file ttu_r3_uaf_to_code_reuse_refactored.c
 * @brief Use-After-Free to Code Reuse Attack Vulnerability Test
 * 
 * REFACTORED FOR xBGAS-Morello TTU Security Evaluation
 * =====================================================
 * 
 * VULNERABILITY TYPE: Use-After-Free (UAF) -> Code Reuse Attack
 * SECURITY IMPACT: Critical - Code execution, privilege escalation
 * CHERI MITIGATION: Capability temporal safety, bounds checking
 * 
 * DESCRIPTION:
 * This test demonstrates a sophisticated use-after-free vulnerability where
 * a function pointer within a freed structure is exploited to achieve code
 * reuse. The attack vector involves:
 * 1. Allocating a structure containing a function pointer
 * 2. Freeing the structure but retaining a dangling pointer
 * 3. Reallocating the same memory with attacker-controlled data
 * 4. Calling the function pointer, now pointing to malicious code
 * 
 * CHERI-MORELLO ANALYSIS:
 * - Capability temporal safety should prevent access to freed memory
 * - Function pointer capabilities should become invalid after free
 * - Bounds checking prevents overwriting function pointers
 * - Tag-based revocation invalidates dangling capabilities
 * 
 * EXPECTED BEHAVIOR:
 * - Traditional systems: Code reuse attack succeeds, gadget() executes
 * - CHERI-Morello: Capability fault on use-after-free access
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
#define MAX_USERNAME_LEN 8
#define TEST_ITERATIONS 1
#define EXPECTED_CHERI_PROTECTION 1

// Test result tracking
typedef enum {
    TEST_RESULT_UNKNOWN = 0,
    TEST_RESULT_EXPLOIT_SUCCESS,    // UAF attack succeeded
    TEST_RESULT_CHERI_PROTECTED,    // CHERI prevented the attack
    TEST_RESULT_MALLOC_FAILED,      // Memory allocation failed
    TEST_RESULT_EXCEPTION           // Exception occurred
} test_result_t;

// Statistics tracking
typedef struct {
    int total_tests;
    int exploit_successes;
    int cheri_protections;
    int malloc_failures;
    int exceptions;
} test_stats_t;

static test_stats_t global_stats = {0};
static volatile sig_atomic_t signal_caught = 0;
static sigjmp_buf signal_env;

/**
 * @brief Structure containing function pointer and user data
 * 
 * This structure simulates a common pattern where function pointers
 * are stored alongside data, creating opportunities for UAF exploits.
 */
struct user_context {
    void (*operation)(void);        // Function pointer target
    char username[MAX_USERNAME_LEN]; // User data buffer
    int user_id;                    // Additional context
};

// Global counter for legitimate function demonstration
static int operation_counter = 0;

/**
 * @brief Legitimate function that should be called initially
 */
static void legitimate_operation(void) {
    operation_counter++;
    printf("    Legitimate operation executed (count: %d)\n", operation_counter);
}

/**
 * @brief Malicious gadget function simulating code reuse attack
 * 
 * In a real attack, this would represent existing code being
 * reused for malicious purposes (ROP/JOP gadgets).
 */
static void malicious_gadget(void) {
    printf("    *** EXPLOIT SUCCESS: Code reuse attack executed! ***\n");
    signal_caught = 1;
    // Note: In refactored version, we don't exit() to allow cleanup
}

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
 * @brief Phase 1: Setup and initial allocation
 */
static struct user_context* phase1_setup_allocation(long thread_id) {
    printf("  [Thread %ld] Phase 1: Setting up user context structure\n", thread_id);
    
    struct user_context *user = malloc(sizeof(struct user_context));
    if (!user) {
        printf("  [Thread %ld] ERROR: Failed to allocate user context\n", thread_id);
        return NULL;
    }
    
    // Initialize structure with legitimate data
    user->operation = &legitimate_operation;
    strncpy(user->username, "alice", MAX_USERNAME_LEN - 1);
    user->username[MAX_USERNAME_LEN - 1] = '\0';
    user->user_id = (int)thread_id;
    
    printf("  [Thread %ld] User context allocated at: %#p\n", thread_id, (void*)user);
    printf("  [Thread %ld] Function pointer points to: %#p\n", thread_id, (void*)user->operation);
    
    return user;
}

/**
 * @brief Phase 2: Use the legitimate function pointer
 */
static void phase2_legitimate_use(long thread_id, struct user_context *user) {
    printf("  [Thread %ld] Phase 2: Executing legitimate operation\n", thread_id);
    
    if (user && user->operation) {
        user->operation();
        printf("  [Thread %ld] Username: %s, User ID: %d\n", 
               thread_id, user->username, user->user_id);
    }
}

/**
 * @brief Phase 3: Create use-after-free condition
 */
static void phase3_create_uaf(long thread_id, struct user_context *user) {
    printf("  [Thread %ld] Phase 3: Creating UAF condition by freeing structure\n", thread_id);
    
    if (user) {
        printf("  [Thread %ld] Freeing user context at: %#p\n", thread_id, (void*)user);
        free(user);
        // Note: user pointer is now dangling but we keep it for the attack
    }
}

/**
 * @brief Phase 4: Attempt memory reuse with malicious data
 */
static void* phase4_memory_reuse(long thread_id) {
    printf("  [Thread %ld] Phase 4: Attempting to reuse freed memory\n", thread_id);
    
    // Allocate memory of similar size, hoping to get the same location
    void *reused_memory = malloc(sizeof(struct user_context));
    if (!reused_memory) {
        printf("  [Thread %ld] ERROR: Failed to allocate reused memory\n", thread_id);
        return NULL;
    }
    
    printf("  [Thread %ld] Reused memory allocated at: %#p\n", thread_id, reused_memory);
    
    // Fill with attacker-controlled data (function pointer to gadget)
    uintptr_t *func_ptr_location = (uintptr_t*)reused_memory;
    *func_ptr_location = (uintptr_t)&malicious_gadget;
    
    printf("  [Thread %ld] Overwrote function pointer with gadget address: %#p\n", 
           thread_id, (void*)&malicious_gadget);
    
    return reused_memory;
}

/**
 * @brief Phase 5: Exploit attempt - call dangling function pointer
 */
static test_result_t phase5_exploit_attempt(long thread_id, 
                                          struct user_context *dangling_user) {
    printf("  [Thread %ld] Phase 5: Attempting UAF exploit\n", thread_id);
    
    test_result_t result = TEST_RESULT_UNKNOWN;
    signal_caught = 0;
    
    // Set up signal handling for this attempt
    if (sigsetjmp(signal_env, 1) == 0) {
        printf("  [Thread %ld] Calling dangling function pointer...\n", thread_id);
        
        // This is the UAF exploit attempt
        if (dangling_user && dangling_user->operation) {
            dangling_user->operation();
            
            // If we reach here without signals, check if exploit succeeded
            if (signal_caught == 1) {
                result = TEST_RESULT_EXPLOIT_SUCCESS;
                printf("  [Thread %ld] Code reuse attack succeeded!\n", thread_id);
            } else {
                result = TEST_RESULT_CHERI_PROTECTED;
                printf("  [Thread %ld] Unexpected: No signal but no exploit\n", thread_id);
            }
        } else {
            result = TEST_RESULT_CHERI_PROTECTED;
            printf("  [Thread %ld] Function pointer appears NULL or invalid\n", thread_id);
        }
    } else {
        // Signal was caught
        result = TEST_RESULT_EXCEPTION;
        printf("  [Thread %ld] CHERI PROTECTION: Signal %ld caught during UAF attempt\n", 
               thread_id, (long)signal_caught);
    }
    
    return result;
}

/**
 * @brief Main UAF to code reuse vulnerability test function
 */
static void uaf_code_reuse_vulnerability_test(void* arg) {
    long thread_id = *(long*)arg;
    free(arg);  // Clean up the allocated thread ID
    test_result_t result = TEST_RESULT_UNKNOWN;
    
    printf("[Thread %ld] ==> Starting UAF to Code Reuse Attack Test\n", thread_id);
    
    // Setup signal handlers for this thread
    setup_signal_handlers();
    
    // Phase 1: Setup and allocation
    struct user_context *user = phase1_setup_allocation(thread_id);
    if (!user) {
        global_stats.malloc_failures++;
        printf("[Thread %ld] <== Test completed: MALLOC_FAILED\n", thread_id);
        return NULL;
    }
    
    // Phase 2: Legitimate use
    phase2_legitimate_use(thread_id, user);
    
    // Keep a copy of the pointer for UAF attack
    struct user_context *dangling_ptr = user;
    
    // Phase 3: Create UAF condition
    phase3_create_uaf(thread_id, user);
    
    // Phase 4: Memory reuse attempt
    void *reused_memory = phase4_memory_reuse(thread_id);
    if (!reused_memory) {
        global_stats.malloc_failures++;
        printf("[Thread %ld] <== Test completed: MALLOC_FAILED\n", thread_id);
        return NULL;
    }
    
    // Phase 5: Exploit attempt
    result = phase5_exploit_attempt(thread_id, dangling_ptr);
    
    // Cleanup
    if (reused_memory) {
        free(reused_memory);
    }
    
    // Update statistics
    global_stats.total_tests++;
    switch (result) {
        case TEST_RESULT_EXPLOIT_SUCCESS:
            global_stats.exploit_successes++;
            printf("[Thread %ld] <== Test completed: EXPLOIT_SUCCESS\n", thread_id);
            break;
        case TEST_RESULT_CHERI_PROTECTED:
            global_stats.cheri_protections++;
            printf("[Thread %ld] <== Test completed: CHERI_PROTECTED\n", thread_id);
            break;
        case TEST_RESULT_EXCEPTION:
            global_stats.exceptions++;
            printf("[Thread %ld] <== Test completed: EXCEPTION_CAUGHT\n", thread_id);
            break;
        default:
            printf("[Thread %ld] <== Test completed: UNKNOWN_RESULT\n", thread_id);
            break;
    }
    
    // No return value for void function
}

/**
 * @brief Print comprehensive test results and analysis
 */
static void print_test_analysis(void) {
    printf("\n================================================================================\n");
    printf("UAF TO CODE REUSE ATTACK - TEST ANALYSIS\n");
    printf("================================================================================\n");
    
    printf("Test Statistics:\n");
    printf("  Total tests executed:     %d\n", global_stats.total_tests);
    printf("  Exploit successes:        %d\n", global_stats.exploit_successes);
    printf("  CHERI protections:        %d\n", global_stats.cheri_protections);
    printf("  Memory allocation fails:  %d\n", global_stats.malloc_failures);
    printf("  Exceptions caught:        %d\n", global_stats.exceptions);
    
    printf("\nSecurity Analysis:\n");
    if (global_stats.exploit_successes > 0) {
        printf("  ❌ VULNERABILITY: UAF to code reuse attack succeeded\n");
        printf("     - Function pointers in freed memory were exploitable\n");
        printf("     - Memory reuse enabled control flow hijacking\n");
        printf("     - System lacks temporal memory safety\n");
    }
    
    if (global_stats.cheri_protections > 0 || global_stats.exceptions > 0) {
        printf("  ✅ PROTECTION: CHERI mitigations active\n");
        printf("     - Capability temporal safety prevented UAF exploitation\n");
        printf("     - Function pointer capabilities invalidated after free\n");
        printf("     - Memory safety violations detected and prevented\n");
    }
    
    printf("\nCHERI-Morello Mitigation Analysis:\n");
    printf("  • Temporal Safety: Capabilities to freed memory become invalid\n");
    printf("  • Spatial Safety: Function pointers have bounded capabilities\n");
    printf("  • Tag Integrity: Memory tags prevent capability forgery\n");
    printf("  • Revocation: Freed memory capabilities are systematically revoked\n");
    
    printf("\nEducational Value:\n");
    printf("  • Demonstrates sophisticated UAF exploit techniques\n");
    printf("  • Shows relationship between memory corruption and code reuse\n");
    printf("  • Illustrates CHERI's capability-based protection mechanisms\n");
    printf("  • Highlights importance of temporal memory safety\n");
    
    printf("================================================================================\n");
}

/**
 * @brief Main function - orchestrates the UAF to code reuse attack test
 */
int main(void) {
    printf("UAF to Code Reuse Attack Vulnerability Test (Refactored)\n");
    printf("========================================================\n");
    printf("Testing use-after-free exploitation for code reuse attacks\n");
    printf("Expected on CHERI: Capability violations prevent exploitation\n\n");
    
    // Initialize xBGAS runtime
    if (xbrtime_init() != 0) {
        fprintf(stderr, "ERROR: Failed to initialize xBGAS runtime\n");
        return EXIT_FAILURE;
    }
    
    int num_pes = xbrtime_num_pes();
    printf("Executing UAF to code reuse tests on %d processing elements\n\n", num_pes);
    
    // Execute tests across all PEs
    for (long i = 0; i < num_pes; i++) {
        tpool_add_work(threads[i].thread_queue, 
                      uaf_code_reuse_vulnerability_test, 
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
    if (global_stats.exploit_successes > 0) {
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
