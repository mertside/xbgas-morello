/**
 * @file ttu_t6_uaf_function_pointer_refactored.c
 * @brief Refactored Use-After-Free on Function Pointer Security Test
 * 
 * \section test_overview Test Overview
 * 
 * This test demonstrates a use-after-free vulnerability involving function pointers.
 * The vulnerability occurs when a function pointer stored in dynamically allocated
 * memory is used after that memory has been freed and potentially reallocated.
 * 
 * \section vulnerability_details Vulnerability Details
 * 
 * The attack scenario:
 * 1. Allocate memory to store a function pointer
 * 2. Initialize the function pointer to point to a legitimate function
 * 3. Free the memory containing the function pointer
 * 4. Reallocate memory (may reuse the same address)
 * 5. Initialize new memory with different function pointer
 * 6. Use the original (freed) function pointer - VULNERABILITY
 * 
 * \section cheri_analysis CHERI Analysis
 * 
 * On CHERI-Morello systems:
 * - Function pointers are capabilities with bounds and permissions
 * - When memory is freed, capabilities stored in that memory become invalid
 * - Attempting to use an invalid capability triggers a trap
 * - This prevents control-flow hijacking via dangling function pointers
 * 
 * \section expected_behavior Expected Behavior
 * 
 * - **Traditional System**: UAF may succeed, potentially calling wrong function
 * - **CHERI-Morello**: Capability violation prevents UAF, maintaining control flow integrity
 * 
 * \author Mert Side (Texas Tech University)
 * \date 2024
 */

/* ========================================================================= */
/*                           SYSTEM INCLUDES                                */
/* ========================================================================= */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <setjmp.h>
#include <unistd.h>
#include <pthread.h>

/* ========================================================================= */
/*                           PROJECT INCLUDES                               */
/* ========================================================================= */

#include "xbrtime_morello.h"

/* ========================================================================= */
/*                              CONSTANTS                                   */
/* ========================================================================= */

#define TEST_NAME "Function Pointer Use-After-Free"
#define FUNC_PTR_SIZE sizeof(function_ptr_t)

/* ========================================================================= */
/*                             TYPE DEFINITIONS                             */
/* ========================================================================= */

typedef void (*function_ptr_t)(const char* context);

typedef struct {
    int thread_id;
    int test_passed;
    jmp_buf recovery_point;
    volatile sig_atomic_t signal_caught;
    function_ptr_t* original_func_ptr;
    function_ptr_t* target_func_ptr;
} thread_test_context_t;

/* ========================================================================= */
/*                            GLOBAL VARIABLES                              */
/* ========================================================================= */

static volatile sig_atomic_t current_thread_id = 0;

/* ========================================================================= */
/*                          FUNCTION DECLARATIONS                           */
/* ========================================================================= */

static void default_function(const char* context);
static void target_function(const char* context);
static void malicious_function(const char* context);
static void signal_handler(int sig);
static void* execute_uaf_test(void* arg);

/* ========================================================================= */
/*                          SIGNAL HANDLING                                 */
/* ========================================================================= */

static void signal_handler(int sig) {
    thread_test_context_t* ctx = (thread_test_context_t*)pthread_getspecific(pthread_self());
    if (ctx) {
        ctx->signal_caught = sig;
        longjmp(ctx->recovery_point, sig);
    }
}

/* ========================================================================= */
/*                          TEST FUNCTIONS                                  */
/* ========================================================================= */

static void default_function(const char* context) {
    printf("[Thread %d] ✅ Default function called: %s\n", 
           (int)current_thread_id, context);
}

static void target_function(const char* context) {
    printf("[Thread %d] ⚠️  Target function called: %s\n", 
           (int)current_thread_id, context);
}

static void malicious_function(const char* context) {
    printf("[Thread %d] 💀 CRITICAL: Malicious function executed: %s\n", 
           (int)current_thread_id, context);
    printf("[Thread %d] 🚨 SYSTEM COMPROMISED!\n", 
           (int)current_thread_id);
}

/* ========================================================================= */
/*                          MEMORY ANALYSIS                                 */
/* ========================================================================= */

static void analyze_pointer(const char* name, void* ptr) {
    printf("[Thread %d] 🔍 %s: %p", (int)current_thread_id, name, ptr);
    
#ifdef __CHERI__
    if (ptr) {
        printf(" [tag=%d, base=%#lx, length=%#lx]",
               __builtin_cheri_tag_get(ptr),
               __builtin_cheri_base_get(ptr),
               __builtin_cheri_length_get(ptr));
    }
#endif
    
    printf("\n");
}

/* ========================================================================= */
/*                          TEST EXECUTION                                  */
/* ========================================================================= */

static void* execute_uaf_test(void* arg) {
    thread_test_context_t* ctx = (thread_test_context_t*)arg;
    current_thread_id = ctx->thread_id;
    int recovery_signal = 0;
    
    printf("\n[Thread %d] 🚀 Starting %s test\n", ctx->thread_id, TEST_NAME);
    
    // Setup signal handling
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGSEGV, &sa, NULL);
    sigaction(SIGBUS, &sa, NULL);
    sigaction(SIGABRT, &sa, NULL);
    
    // Set recovery point
    if ((recovery_signal = setjmp(ctx->recovery_point)) != 0) {
        printf("[Thread %d] 🛡️  CHERI Protection: Caught signal %d\n", 
               ctx->thread_id, recovery_signal);
        ctx->test_passed = 1;
        goto cleanup;
    }
    
    // Phase 1: Allocate and initialize function pointer
    printf("[Thread %d] 📋 Phase 1: Allocate function pointer\n", ctx->thread_id);
    ctx->original_func_ptr = (function_ptr_t*)malloc(FUNC_PTR_SIZE);
    if (!ctx->original_func_ptr) {
        printf("[Thread %d] ❌ Failed to allocate memory\n", ctx->thread_id);
        ctx->test_passed = 0;
        goto cleanup;
    }
    
    *ctx->original_func_ptr = default_function;
    analyze_pointer("Original function pointer", (void*)ctx->original_func_ptr);
    
    // Phase 2: Use function pointer (should work)
    printf("[Thread %d] 📋 Phase 2: Call function pointer\n", ctx->thread_id);
    (*ctx->original_func_ptr)("initial call");
    
    // Phase 3: Free the function pointer memory
    printf("[Thread %d] 📋 Phase 3: Free function pointer memory\n", ctx->thread_id);
    free(ctx->original_func_ptr);
    
    // Phase 4: Reallocate memory (may reuse same address)
    printf("[Thread %d] 📋 Phase 4: Reallocate memory\n", ctx->thread_id);
    ctx->target_func_ptr = (function_ptr_t*)malloc(FUNC_PTR_SIZE);
    if (!ctx->target_func_ptr) {
        printf("[Thread %d] ❌ Failed to reallocate memory\n", ctx->thread_id);
        ctx->test_passed = 0;
        goto cleanup;
    }
    
    *ctx->target_func_ptr = target_function;
    analyze_pointer("Target function pointer", (void*)ctx->target_func_ptr);
    
    if (ctx->target_func_ptr == ctx->original_func_ptr) {
        printf("[Thread %d] ⚠️  Memory reuse detected\n", ctx->thread_id);
    }
    
    // Phase 5: VULNERABILITY - Use freed function pointer
    printf("[Thread %d] 📋 Phase 5: ⚠️  VULNERABILITY ATTEMPT\n", ctx->thread_id);
    printf("[Thread %d] 💥 Calling freed function pointer...\n", ctx->thread_id);
    
    (*ctx->original_func_ptr)("use-after-free call");
    
    // If we reach here, the vulnerability succeeded
    printf("[Thread %d] 🚨 VULNERABILITY SUCCESS: UAF not detected!\n", ctx->thread_id);
    ctx->test_passed = 0;
    
cleanup:
    // Clean up valid allocations
    if (ctx->target_func_ptr) {
        free(ctx->target_func_ptr);
    }
    
    if (ctx->test_passed) {
        printf("[Thread %d] ✅ Test PASSED: Memory safety violation prevented\n", ctx->thread_id);
    } else {
        printf("[Thread %d] ❌ Test FAILED: Vulnerability exploitable\n", ctx->thread_id);
    }
    
    printf("[Thread %d] 🏁 %s test finished\n\n", ctx->thread_id, TEST_NAME);
    
    int* result = malloc(sizeof(int));
    *result = ctx->test_passed;
    return result;
}

/* ========================================================================= */
/*                              MAIN FUNCTION                               */
/* ========================================================================= */

int main(void) {
    printf("=================================================================\n");
    printf("🔬 xBGAS Security Test: %s\n", TEST_NAME);
    printf("🎯 Platform: CHERI-Morello | Runtime: xBGAS\n");
    printf("=================================================================\n\n");
    
    // Initialize xBGAS runtime
    xbrtime_init();
    int num_pes = xbrtime_num_pes();
    printf("📊 Number of processing elements: %d\n", num_pes);
    
    // Create threads and contexts
    pthread_t* threads = malloc(num_pes * sizeof(pthread_t));
    thread_test_context_t* contexts = malloc(num_pes * sizeof(thread_test_context_t));
    int* thread_results = malloc(num_pes * sizeof(int));
    
    // Initialize contexts
    for (int i = 0; i < num_pes; i++) {
        contexts[i].thread_id = i;
        contexts[i].test_passed = 0;
        contexts[i].signal_caught = 0;
        contexts[i].original_func_ptr = NULL;
        contexts[i].target_func_ptr = NULL;
    }
    
    // Create threads
    printf("🧵 Creating %d test threads...\n", num_pes);
    for (int i = 0; i < num_pes; i++) {
        if (pthread_create(&threads[i], NULL, execute_uaf_test, &contexts[i]) != 0) {
            printf("ERROR: Failed to create thread %d\n", i);
            thread_results[i] = 0;
        }
    }
    
    // Wait for threads to complete
    printf("⏳ Waiting for threads to complete...\n");
    for (int i = 0; i < num_pes; i++) {
        void *result;
        if (pthread_join(threads[i], &result) != 0) {
            printf("ERROR: Failed to join thread %d\n", i);
            thread_results[i] = 0;
        } else {
            thread_results[i] = *(int *)result;
            free(result);
        }
    }
    
    // Analyze results
    printf("=================================================================\n");
    printf("TEST RESULTS SUMMARY\n");
    printf("=================================================================\n");
    
    int passed_tests = 0;
    for (int i = 0; i < num_pes; i++) {
        printf("Thread %d: %s\n", i, thread_results[i] ? "PASS" : "FAIL");
        if (thread_results[i]) passed_tests++;
    }
    
    printf("-----------------------------------------------------------------\n");
    printf("Total Tests: %d | Passed: %d | Failed: %d\n", 
           num_pes, passed_tests, num_pes - passed_tests);
    printf("Success Rate: %.1f%%\n", (float)passed_tests / num_pes * 100.0);
    
    if (passed_tests == num_pes) {
        printf("OVERALL RESULT: PASS - All UAF attempts were prevented\n");
    } else {
        printf("OVERALL RESULT: FAIL - Some UAF attempts succeeded\n");
    }
    
    printf("=================================================================\n");
    
    // Cleanup
    free(threads);
    free(contexts);
    free(thread_results);
    xbrtime_close();
    
    return (passed_tests == num_pes) ? 0 : 1;
}

/* EOF */
