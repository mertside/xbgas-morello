/**
 * @file ttu_t6_uaf_function_pointer_refactored.c
 * @brief Refactored Use-After-Free on Function Pointer Security Test
 * @author Mert Side (Texas Tech University)
 * @date 2024
 * 
 * @section DESCRIPTION
 * This program demonstrates a use-after-free (UAF) vulnerability involving function pointers.
 * The vulnerability occurs when:
 * 1. A function pointer is allocated and initialized
 * 2. The memory containing the function pointer is freed
 * 3. New memory is allocated (potentially reusing the same address)
 * 4. The original (freed) function pointer is used, potentially calling an unintended function
 * 
 * @section CHERI_ANALYSIS
 * On CHERI-Morello systems, this test evaluates:
 * - Temporal memory safety for function pointers
 * - Capability revocation upon free()
 * - Protection against control-flow hijacking via dangling function pointers
 * - Detection of use-after-free on executable capabilities
 * 
 * @section EXPECTED_BEHAVIOR
 * - Traditional systems: May call unintended function (security vulnerability)
 * - CHERI-Morello: Should trap on capability use-after-free, preventing exploitation
 * 
 * @section TEST_PHASES
 * 1. SETUP: Initialize xBGAS runtime and thread environment
 * 2. ALLOCATE: Create memory for function pointer storage
 * 3. INITIALIZE: Set function pointer to default function
 * 4. USE_INITIAL: Call function through pointer (should succeed)
 * 5. FREE: Release function pointer memory
 * 6. REALLOCATE: Allocate new memory (may reuse same address)
 * 7. REASSIGN: Set new pointer to target function
 * 8. UAF_ATTEMPT: Use original (freed) function pointer (vulnerability)
 * 9. CLEANUP: Free remaining allocations
 * 10. TEARDOWN: Close xBGAS runtime
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <setjmp.h>
#include <unistd.h>
#include <sys/mman.h>

// Include new modular headers
#include "../../runtime/xbrtime_common.h"
#include "../../runtime/xbrtime_api.h"
#include "../../runtime/test.h"

//=============================================================================
// TEST CONFIGURATION AND CONSTANTS
//=============================================================================

/** @brief Test identification */
#define TEST_NAME "Use-After-Free Function Pointer"
#define TEST_ID "TTU_T6"
#define TEST_CATEGORY "TEMPORAL_MEMORY_SAFETY"

/** @brief Memory allocation constants */
#define FUNC_PTR_SIZE sizeof(function_ptr_t)
#define MAX_FUNCTION_NAME 64

/** @brief Test phases for structured execution */
typedef enum {
    PHASE_SETUP = 0,
    PHASE_ALLOCATE,
    PHASE_INITIALIZE,
    PHASE_USE_INITIAL,
    PHASE_FREE,
    PHASE_REALLOCATE,
    PHASE_REASSIGN,
    PHASE_UAF_ATTEMPT,
    PHASE_CLEANUP,
    PHASE_TEARDOWN,
    PHASE_MAX
} test_phase_t;

/** @brief Function pointer type definition */
typedef void (*function_ptr_t)(const char* context);

//=============================================================================
// GLOBAL STATE AND SIGNAL HANDLING
//=============================================================================

/** @brief Global test state for signal handling */
static struct {
    jmp_buf recovery_point;
    volatile sig_atomic_t signal_caught;
    volatile sig_atomic_t current_phase;
    volatile sig_atomic_t thread_id;
    function_ptr_t* original_func_ptr;
    function_ptr_t* target_func_ptr;
    void* allocated_memory[2];
    size_t allocation_count;
} test_state = {0};

/** @brief Signal handler for memory safety violations */
static void signal_handler(int sig) {
    test_state.signal_caught = sig;
    
    const char* signal_name = (sig == SIGSEGV) ? "SIGSEGV" :
                              (sig == SIGBUS) ? "SIGBUS" :
                              (sig == SIGABRT) ? "SIGABRT" : "UNKNOWN";
    
    printf("[Thread %ld] 🛡️  CHERI Protection: Caught %s during phase %d\n", 
           (long)test_state.thread_id, signal_name, (int)test_state.current_phase);
    
    // Jump back to recovery point
    longjmp(test_state.recovery_point, sig);
}

/** @brief Setup signal handlers for the test */
static void setup_signal_handlers(void) {
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    
    sigaction(SIGSEGV, &sa, NULL);
    sigaction(SIGBUS, &sa, NULL);
    sigaction(SIGABRT, &sa, NULL);
}

//=============================================================================
// TEST FUNCTION IMPLEMENTATIONS
//=============================================================================

/** @brief Default function - represents legitimate functionality */
static void default_function(const char* context) {
    printf("[Thread %ld] ✅ Default function called: %s\n", 
           (long)test_state.thread_id, context);
}

/** @brief Target function - represents potentially malicious code */
static void target_function(const char* context) {
    printf("[Thread %ld] ❌ Target function called: %s\n", 
           (long)test_state.thread_id, context);
    printf("[Thread %ld] 🚨 VULNERABILITY: Control flow hijacked!\n", 
           (long)test_state.thread_id);
}

/** @brief Malicious function - should never be reachable */
static void malicious_function(const char* context) {
    printf("[Thread %ld] 💀 CRITICAL: Malicious function executed: %s\n", 
           (long)test_state.thread_id, context);
    printf("[Thread %ld] 🚨 SYSTEM COMPROMISED!\n", 
           (long)test_state.thread_id);
}

//=============================================================================
// MEMORY ANALYSIS UTILITIES
//=============================================================================

/** @brief Analyze pointer properties (CHERI-specific) */
static void analyze_pointer(const char* name, void* ptr) {
    if (!ptr) {
        printf("[Thread %ld] 🔍 %s: NULL pointer\n", 
               (long)test_state.thread_id, name);
        return;
    }
    
    printf("[Thread %ld] 🔍 %s: %p", 
           (long)test_state.thread_id, name, ptr);
    
#ifdef __CHERI__
    // CHERI-specific capability analysis
    printf(" [tag=%d, base=%#lx, length=%#lx, offset=%#lx]",
           __builtin_cheri_tag_get(ptr),
           __builtin_cheri_base_get(ptr),
           __builtin_cheri_length_get(ptr),
           __builtin_cheri_offset_get(ptr));
#endif
    
    printf("\n");
}

/** @brief Analyze function pointer before and after operations */
static void analyze_function_pointer_state(const char* phase, function_ptr_t* fptr) {
    printf("[Thread %ld] 📊 Function pointer analysis (%s):\n", 
           (long)test_state.thread_id, phase);
    
    analyze_pointer("Function pointer address", (void*)fptr);
    
    if (fptr) {
        analyze_pointer("Function address", (void*)*fptr);
        
        // Try to identify which function this points to
        if (*fptr == default_function) {
            printf("[Thread %ld] 🎯 Points to: default_function\n", 
                   (long)test_state.thread_id);
        } else if (*fptr == target_function) {
            printf("[Thread %ld] 🎯 Points to: target_function\n", 
                   (long)test_state.thread_id);
        } else if (*fptr == malicious_function) {
            printf("[Thread %ld] 🎯 Points to: malicious_function\n", 
                   (long)test_state.thread_id);
        } else {
            printf("[Thread %ld] 🎯 Points to: unknown function\n", 
                   (long)test_state.thread_id);
        }
    }
}

//=============================================================================
// CORE TEST LOGIC
//=============================================================================

/** @brief Execute the use-after-free function pointer vulnerability test */
static void execute_uaf_function_pointer_test(void* arg) {
    long tid = (long)arg;
    test_state.thread_id = tid;
    int recovery_signal = 0;
    
    printf("\n[Thread %ld] 🚀 Starting %s test\n", tid, TEST_NAME);
    printf("[Thread %ld] 📝 Test ID: %s, Category: %s\n", tid, TEST_ID, TEST_CATEGORY);
    
    // Setup signal handling
    setup_signal_handlers();
    
    // Set recovery point for signal handling
    if ((recovery_signal = setjmp(test_state.recovery_point)) != 0) {
        printf("[Thread %ld] 🔄 Recovered from signal %d in phase %d\n", 
               tid, recovery_signal, (int)test_state.current_phase);
        goto cleanup_and_exit;
    }
    
    // PHASE 1: SETUP
    test_state.current_phase = PHASE_SETUP;
    printf("[Thread %ld] 📋 Phase %d: Setup\n", tid, PHASE_SETUP);
    test_state.allocation_count = 0;
    
    // PHASE 2: ALLOCATE
    test_state.current_phase = PHASE_ALLOCATE;
    printf("[Thread %ld] 📋 Phase %d: Allocate function pointer memory\n", tid, PHASE_ALLOCATE);
    
    test_state.original_func_ptr = (function_ptr_t*)malloc(FUNC_PTR_SIZE);        if (!test_state.original_func_ptr) {
            printf("[Thread %ld] ❌ Failed to allocate memory for function pointer\n", tid);
            return;
        }
    test_state.allocated_memory[test_state.allocation_count++] = test_state.original_func_ptr;
    
    analyze_function_pointer_state("after allocation", test_state.original_func_ptr);
    
    // PHASE 3: INITIALIZE
    test_state.current_phase = PHASE_INITIALIZE;
    printf("[Thread %ld] 📋 Phase %d: Initialize function pointer\n", tid, PHASE_INITIALIZE);
    
    *test_state.original_func_ptr = default_function;
    analyze_function_pointer_state("after initialization", test_state.original_func_ptr);
    
    // PHASE 4: USE_INITIAL
    test_state.current_phase = PHASE_USE_INITIAL;
    printf("[Thread %ld] 📋 Phase %d: Use initial function pointer\n", tid, PHASE_USE_INITIAL);
    
    (*test_state.original_func_ptr)("initial call");
    
    // PHASE 5: FREE
    test_state.current_phase = PHASE_FREE;
    printf("[Thread %ld] 📋 Phase %d: Free function pointer memory\n", tid, PHASE_FREE);
    
    printf("[Thread %ld] 🗑️  Freeing function pointer at %p\n", tid, (void*)test_state.original_func_ptr);
    free(test_state.original_func_ptr);
    
    // PHASE 6: REALLOCATE
    test_state.current_phase = PHASE_REALLOCATE;
    printf("[Thread %ld] 📋 Phase %d: Reallocate memory\n", tid, PHASE_REALLOCATE);
    
    // Allocate new memory that might reuse the same address
    test_state.target_func_ptr = (function_ptr_t*)malloc(FUNC_PTR_SIZE);        if (!test_state.target_func_ptr) {
            printf("[Thread %ld] ❌ Failed to reallocate memory\n", tid);
            return;
        }
    test_state.allocated_memory[test_state.allocation_count++] = test_state.target_func_ptr;
    
    analyze_pointer("New allocation", (void*)test_state.target_func_ptr);
    
    if (test_state.target_func_ptr == test_state.original_func_ptr) {
        printf("[Thread %ld] ⚠️  Memory reuse detected: same address reused\n", tid);
    } else {
        printf("[Thread %ld] ℹ️  Different address allocated\n", tid);
    }
    
    // PHASE 7: REASSIGN
    test_state.current_phase = PHASE_REASSIGN;
    printf("[Thread %ld] 📋 Phase %d: Assign new function pointer\n", tid, PHASE_REASSIGN);
    
    *test_state.target_func_ptr = target_function;
    analyze_function_pointer_state("new assignment", test_state.target_func_ptr);
    
    // PHASE 8: UAF_ATTEMPT (Critical vulnerability test)
    test_state.current_phase = PHASE_UAF_ATTEMPT;
    printf("[Thread %ld] 📋 Phase %d: Attempt use-after-free\n", tid, PHASE_UAF_ATTEMPT);
    printf("[Thread %ld] 🚨 CRITICAL: Attempting to use freed function pointer\n", tid);
    
    analyze_function_pointer_state("before UAF attempt", test_state.original_func_ptr);
    
    // This is the vulnerability - using freed function pointer
    printf("[Thread %ld] 💥 Calling freed function pointer...\n", tid);
    (*test_state.original_func_ptr)("use-after-free call");
    
    // If we reach here without a signal, the vulnerability succeeded
    printf("[Thread %ld] 🚨 VULNERABILITY SUCCESS: Function pointer UAF not detected!\n", tid);
    
cleanup_and_exit:
    // PHASE 9: CLEANUP
    test_state.current_phase = PHASE_CLEANUP;
    printf("[Thread %ld] 📋 Phase %d: Cleanup\n", tid, PHASE_CLEANUP);
    
    // Free any remaining valid allocations
    if (test_state.allocation_count > 1 && test_state.target_func_ptr) {
        printf("[Thread %ld] 🗑️  Freeing target function pointer\n", tid);
        free(test_state.target_func_ptr);
    }
    
    // PHASE 10: TEARDOWN
    test_state.current_phase = PHASE_TEARDOWN;
    printf("[Thread %ld] 📋 Phase %d: Teardown\n", tid, PHASE_TEARDOWN);
    
    if (recovery_signal != 0) {
        printf("[Thread %ld] ✅ Test completed with CHERI protection (signal %d)\n", tid, recovery_signal);
        printf("[Thread %ld] 🔒 Temporal memory safety violation prevented\n", tid);
    } else {
        printf("[Thread %ld] ❌ Test completed without protection - vulnerability exploitable\n", tid);
    }
    
    printf("[Thread %ld] 🏁 %s test finished\n\n", tid, TEST_NAME);
}

//=============================================================================
// MAIN FUNCTION AND TEST ORCHESTRATION
//=============================================================================

/**
 * @brief Main function - orchestrates the multi-threaded UAF function pointer test
 * @return 0 on success, non-zero on failure
 */
int main(void) {
    printf("=================================================================\n");
    printf("🔬 xBGAS Security Test: %s\n", TEST_NAME);
    printf("📊 Test ID: %s | Category: %s\n", TEST_ID, TEST_CATEGORY);
    printf("🎯 Platform: CHERI-Morello | Runtime: xBGAS\n");
    printf("=================================================================\n\n");
    
    // Initialize xBGAS runtime
    printf("🚀 Initializing xBGAS runtime...\n");
    xbrtime_init();
    
    int num_pes = xbrtime_num_pes();
    printf("📊 Number of processing elements: %d\n", num_pes);
    printf("🧵 Starting multi-threaded function pointer UAF test...\n\n");
    
    // Execute test on all available processing elements
    for (long i = 0; i < num_pes; i++) {
        tpool_add_work(threads[i].thread_queue, execute_uaf_function_pointer_test, (void*)(uintptr_t)i);
    }
    
    // Wait for all threads to complete
    printf("⏳ Waiting for all threads to complete...\n");
    for (int i = 0; i < num_pes; i++) {
        tpool_wait(threads[i].thread_queue);
    }
    
    printf("=================================================================\n");
    printf("✅ Multi-threaded %s test completed\n", TEST_NAME);
    printf("📈 All %d processing elements finished execution\n", num_pes);
    printf("🔒 CHERI-Morello temporal memory safety evaluation complete\n");
    printf("=================================================================\n");
    
    // Close xBGAS runtime
    xbrtime_close();
    
    return 0;
}

/**
 * @brief Test Summary
 * 
 * This refactored test provides comprehensive evaluation of:
 * 
 * 1. **Temporal Memory Safety**: Tests function pointer use-after-free
 * 2. **Control Flow Integrity**: Prevents function pointer hijacking
 * 3. **CHERI Capability System**: Validates capability revocation
 * 4. **Signal Handling**: Graceful recovery from memory safety violations
 * 5. **Multi-threading**: Concurrent vulnerability testing
 * 6. **Detailed Analysis**: Memory and capability inspection
 * 7. **Phase-based Execution**: Structured test progression
 * 8. **Comprehensive Logging**: Detailed execution tracing
 * 
 * Expected behavior on CHERI-Morello:
 * - Capability should be revoked upon free()
 * - Attempt to use freed function pointer should trap
 * - Signal handler should catch the violation
 * - Test should complete with protection confirmation
 * 
 * On traditional systems:
 * - Function pointer UAF may succeed
 * - Control flow may be hijacked
 * - Vulnerability would be exploitable
 */
