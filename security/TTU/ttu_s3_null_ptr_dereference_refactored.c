/*
 * ttu_s3_null_ptr_dereference_refactored.c
 *
 * Copyright (C) 2024 Texas Tech University
 * All Rights Reserved
 *
 * Memory Safety Benchmark: Null Pointer Dereference Test
 * Adapted for xBGAS Runtime on CHERI-Morello
 */

/*!
 * \file ttu_s3_null_ptr_dereference_refactored.c
 * \brief Spatial Memory Safety Test - Null Pointer Dereference
 *
 * This benchmark demonstrates null pointer dereference vulnerabilities where
 * an application attempts to access memory through a null pointer. The test
 * evaluates CHERI-Morello's capability system effectiveness in preventing
 * null pointer dereferences and related spatial memory safety violations.
 *
 * \section test_description Test Description
 * 
 * The test performs various null pointer operations:
 * 1. Direct null pointer dereference (read)
 * 2. Direct null pointer dereference (write)
 * 3. Function call through null function pointer
 * 4. Array access through null array pointer
 * 5. Structure member access through null structure pointer
 *
 * Null pointer dereferences are among the most common programming errors
 * and can lead to crashes, undefined behavior, or security vulnerabilities.
 * CHERI-Morello should prevent these through capability validation.
 *
 * \section expected_behavior Expected Behavior
 *
 * - **Traditional System**: Null pointer dereference may cause segmentation
 *   fault, but behavior is often undefined and system-dependent
 * - **CHERI-Morello**: Capability violation occurs immediately, preventing
 *   null pointer access and providing deterministic protection
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
#include <pthread.h>
#include <signal.h>
#include <setjmp.h>
#include <unistd.h>

/* ========================================================================= */
/*                           PROJECT INCLUDES                               */
/* ========================================================================= */

#include "xbrtime_morello.h"

/* ========================================================================= */
/*                           CONFIGURATION                                  */
/* ========================================================================= */

/** \brief Number of different null pointer tests */
#define NUM_NULL_TESTS 5

/** \brief Test data size for array operations */
#define TEST_DATA_SIZE 10

/** \brief Delay between test operations (microseconds) */
#define TEST_DELAY 5000

/* ========================================================================= */
/*                           TYPE DEFINITIONS                               */
/* ========================================================================= */

/*!
 * \struct test_structure
 * \brief Test structure for null structure pointer tests
 */
typedef struct {
    int id;                     /*!< Structure identifier */
    char name[32];              /*!< Structure name field */
    double value;               /*!< Structure value field */
    void *data_ptr;             /*!< Nested pointer field */
} test_structure_t;

/*!
 * \typedef test_function_t
 * \brief Function pointer type for null function pointer tests
 */
typedef int (*test_function_t)(int param);

/*!
 * \struct null_test_context
 * \brief Context for null pointer tests
 */
typedef struct {
    long thread_id;             /*!< Thread identifier */
    int test_result;            /*!< Overall test result */
    int tests_attempted;        /*!< Number of tests attempted */
    int violations_caught;      /*!< Number of violations caught */
    int successful_accesses;    /*!< Number of successful (bad) accesses */
    
    /* Null pointers for testing */
    int *null_int_ptr;          /*!< Null integer pointer */
    int *null_array_ptr;        /*!< Null array pointer */
    test_structure_t *null_struct_ptr; /*!< Null structure pointer */
    test_function_t null_func_ptr;     /*!< Null function pointer */
    char *null_string_ptr;      /*!< Null string pointer */
} null_test_context_t;

/* ========================================================================= */
/*                           SIGNAL HANDLING                                */
/* ========================================================================= */

static jmp_buf violation_handler;
static volatile int signal_caught = 0;
static volatile null_test_context_t *current_context = NULL;

/*!
 * \brief Signal handler for null pointer violations
 * \param sig Signal number
 */
static void null_pointer_violation_handler(int sig) {
    signal_caught = sig;
    if (current_context) {
        ((null_test_context_t *)current_context)->violations_caught++;
    }
    
    const char *sig_name;
    switch (sig) {
        case SIGBUS: sig_name = "SIGBUS (Bus Error)"; break;
        case SIGSEGV: sig_name = "SIGSEGV (Segmentation Fault)"; break;
#ifdef SIGPROT
        case SIGPROT: sig_name = "SIGPROT (Protection Violation)"; break;
#endif
        default: sig_name = "Unknown Signal"; break;
    }
    
    printf("    [CHERI] Null pointer violation caught: %s (%d)\n", sig_name, sig);
    longjmp(violation_handler, 1);
}

/*!
 * \brief Initialize signal handlers for violation detection
 */
static void setup_signal_handlers(void) {
    struct sigaction sa;
    sa.sa_handler = null_pointer_violation_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    
    sigaction(SIGBUS, &sa, NULL);   /* Bus error */
    sigaction(SIGSEGV, &sa, NULL);  /* Segmentation fault */
#ifdef SIGPROT
    sigaction(SIGPROT, &sa, NULL);  /* CHERI protection violation */
#endif
}

/* ========================================================================= */
/*                           UTILITY FUNCTIONS                              */
/* ========================================================================= */

/*!
 * \brief Print CHERI capability information for null pointers
 * \param ptr Pointer to analyze (should be NULL)
 * \param description Description of the pointer
 */
static void print_null_capability_info(void *ptr, const char *description) {
    printf("  %s:\n", description);
    printf("    Address: %p\n", ptr);
    
#ifdef __CHERI_PURE_CAPABILITY__
    if (ptr == NULL) {
        printf("    CHERI Capability: NULL\n");
        printf("    Tag: 0 (Invalid)\n");
        printf("    Base: N/A\n");
        printf("    Length: N/A\n");
        printf("    Permissions: N/A\n");
        printf("    Valid: No\n");
    } else {
        printf("    CHERI Capability: %#p\n", ptr);
        printf("    Base: 0x%lx\n", cheri_base_get(ptr));
        printf("    Length: %lu\n", cheri_length_get(ptr));
        printf("    Offset: %lu\n", cheri_offset_get(ptr));
        printf("    Permissions: 0x%x\n", cheri_perms_get(ptr));
        printf("    Tag: %d\n", (int)cheri_tag_get(ptr));
        printf("    Valid: %s\n", cheri_tag_get(ptr) ? "Yes" : "No");
    }
#else
    printf("    (CHERI capability information not available)\n");
#endif
}

/*!
 * \brief Sample function for null function pointer testing
 * \param param Test parameter
 * \return Test result
 */
static int sample_test_function(int param) {
    printf("    Sample function called with parameter: %d\n", param);
    return param * 2;
}

/*!
 * \brief Attempt null pointer dereference (read)
 * \param ctx Test context
 * \return 1 if violation was caught, 0 if access succeeded
 */
static int attempt_null_read(null_test_context_t *ctx) {
    printf("[Thread %ld] Test 1: Null pointer dereference (read)\n", ctx->thread_id);
    
    current_context = ctx;
    setup_signal_handlers();
    signal_caught = 0;
    ctx->tests_attempted++;
    
    if (setjmp(violation_handler) == 0) {
        /* Attempt to read through null pointer */
        printf("  Attempting to read *null_int_ptr...\n");
        int value = *(ctx->null_int_ptr);
        
        printf("  SUCCESS: Read value %d from null pointer!\n", value);
        printf("  WARNING: Null pointer dereference was not prevented!\n");
        ctx->successful_accesses++;
        return 0; /* Violation was not caught */
        
    } else {
        /* Signal handler was triggered */
        printf("  SUCCESS: CHERI prevented null pointer read access\n");
        return 1; /* Violation was caught */
    }
}

/*!
 * \brief Attempt null pointer dereference (write)
 * \param ctx Test context
 * \return 1 if violation was caught, 0 if access succeeded
 */
static int attempt_null_write(null_test_context_t *ctx) {
    printf("[Thread %ld] Test 2: Null pointer dereference (write)\n", ctx->thread_id);
    
    current_context = ctx;
    signal_caught = 0;
    ctx->tests_attempted++;
    
    if (setjmp(violation_handler) == 0) {
        /* Attempt to write through null pointer */
        printf("  Attempting to write to *null_int_ptr...\n");
        *(ctx->null_int_ptr) = 42;
        
        printf("  SUCCESS: Wrote value 42 to null pointer!\n");
        printf("  WARNING: Null pointer write was not prevented!\n");
        ctx->successful_accesses++;
        return 0; /* Violation was not caught */
        
    } else {
        /* Signal handler was triggered */
        printf("  SUCCESS: CHERI prevented null pointer write access\n");
        return 1; /* Violation was caught */
    }
}

/*!
 * \brief Attempt null function pointer call
 * \param ctx Test context
 * \return 1 if violation was caught, 0 if call succeeded
 */
static int attempt_null_function_call(null_test_context_t *ctx) {
    printf("[Thread %ld] Test 3: Null function pointer call\n", ctx->thread_id);
    
    current_context = ctx;
    signal_caught = 0;
    ctx->tests_attempted++;
    
    if (setjmp(violation_handler) == 0) {
        /* Attempt to call null function pointer */
        printf("  Attempting to call null_func_ptr(123)...\n");
        int result = ctx->null_func_ptr(123);
        
        printf("  SUCCESS: Function call returned %d!\n", result);
        printf("  WARNING: Null function pointer call was not prevented!\n");
        ctx->successful_accesses++;
        return 0; /* Violation was not caught */
        
    } else {
        /* Signal handler was triggered */
        printf("  SUCCESS: CHERI prevented null function pointer call\n");
        return 1; /* Violation was caught */
    }
}

/*!
 * \brief Attempt null array access
 * \param ctx Test context
 * \return 1 if violation was caught, 0 if access succeeded
 */
static int attempt_null_array_access(null_test_context_t *ctx) {
    printf("[Thread %ld] Test 4: Null array pointer access\n", ctx->thread_id);
    
    current_context = ctx;
    signal_caught = 0;
    ctx->tests_attempted++;
    
    if (setjmp(violation_handler) == 0) {
        /* Attempt to access null array */
        printf("  Attempting to access null_array_ptr[5]...\n");
        int value = ctx->null_array_ptr[5];
        
        printf("  SUCCESS: Read value %d from null array!\n", value);
        printf("  WARNING: Null array access was not prevented!\n");
        ctx->successful_accesses++;
        return 0; /* Violation was not caught */
        
    } else {
        /* Signal handler was triggered */
        printf("  SUCCESS: CHERI prevented null array access\n");
        return 1; /* Violation was caught */
    }
}

/*!
 * \brief Attempt null structure member access
 * \param ctx Test context
 * \return 1 if violation was caught, 0 if access succeeded
 */
static int attempt_null_struct_access(null_test_context_t *ctx) {
    printf("[Thread %ld] Test 5: Null structure pointer access\n", ctx->thread_id);
    
    current_context = ctx;
    signal_caught = 0;
    ctx->tests_attempted++;
    
    if (setjmp(violation_handler) == 0) {
        /* Attempt to access null structure member */
        printf("  Attempting to access null_struct_ptr->id...\n");
        int id = ctx->null_struct_ptr->id;
        
        printf("  SUCCESS: Read structure ID %d from null pointer!\n", id);
        printf("  WARNING: Null structure access was not prevented!\n");
        ctx->successful_accesses++;
        return 0; /* Violation was not caught */
        
    } else {
        /* Signal handler was triggered */
        printf("  SUCCESS: CHERI prevented null structure access\n");
        return 1; /* Violation was caught */
    }
}

/*!
 * \brief Print detailed analysis of null pointer tests
 * \param ctx Test context
 */
static void print_null_pointer_analysis(const null_test_context_t *ctx) {
    printf("\n=== Null Pointer Analysis (Thread %ld) ===\n", ctx->thread_id);
    
    printf("Null Pointer States:\n");
    print_null_capability_info(ctx->null_int_ptr, "Null integer pointer");
    print_null_capability_info(ctx->null_array_ptr, "Null array pointer");
    print_null_capability_info(ctx->null_struct_ptr, "Null structure pointer");
    print_null_capability_info((void *)ctx->null_func_ptr, "Null function pointer");
    print_null_capability_info(ctx->null_string_ptr, "Null string pointer");
    
    printf("Test Statistics:\n");
    printf("  Total tests attempted: %d\n", ctx->tests_attempted);
    printf("  Violations caught: %d\n", ctx->violations_caught);
    printf("  Successful accesses: %d\n", ctx->successful_accesses);
    printf("  Protection rate: %.1f%%\n", 
           ctx->tests_attempted > 0 ? 
           (float)ctx->violations_caught / ctx->tests_attempted * 100.0 : 0.0);
    
    /* Analysis */
    if (ctx->violations_caught == ctx->tests_attempted) {
        printf("  ANALYSIS: Perfect null pointer protection\n");
    } else if (ctx->violations_caught > 0) {
        printf("  ANALYSIS: Partial null pointer protection\n");
    } else {
        printf("  ANALYSIS: No null pointer protection detected\n");
    }
    
    printf("=========================================\n\n");
}

/* ========================================================================= */
/*                           MAIN TEST FUNCTION                             */
/* ========================================================================= */

/*!
 * \brief Main thread function for null pointer dereference test
 * \param arg Thread argument (cast to thread ID)
 * \return Thread result pointer
 */
void* null_pointer_test(void* arg) {
    long thread_id = (long)arg;
    null_test_context_t ctx = {0};
    
    ctx.thread_id = thread_id;
    ctx.test_result = 0;
    
    printf("\n[Thread %ld] Starting Null Pointer Dereference Test\n", thread_id);
    printf("[Thread %ld] =========================================\n", thread_id);
    
    /* Phase 1: Initialize null pointers */
    printf("[Thread %ld] Phase 1: Initializing null pointers\n", thread_id);
    
    ctx.null_int_ptr = NULL;
    ctx.null_array_ptr = NULL;
    ctx.null_struct_ptr = NULL;
    ctx.null_func_ptr = NULL;
    ctx.null_string_ptr = NULL;
    
    printf("  All pointers initialized to NULL\n");
    
    /* Phase 2: Demonstrate valid function for comparison */
    printf("[Thread %ld] Phase 2: Testing valid function pointer\n", thread_id);
    
    test_function_t valid_func = sample_test_function;
    printf("  Valid function pointer: %p\n", (void *)valid_func);
    int valid_result = valid_func(10);
    printf("  Valid function call result: %d\n", valid_result);
    
    /* Phase 3: Display null pointer capabilities */
    print_null_pointer_analysis(&ctx);
    
    /* Phase 4: Execute null pointer tests */
    printf("[Thread %ld] Phase 4: Executing null pointer violation tests\n", thread_id);
    
    int test_results[NUM_NULL_TESTS];
    
    usleep(TEST_DELAY);
    test_results[0] = attempt_null_read(&ctx);
    
    usleep(TEST_DELAY);
    test_results[1] = attempt_null_write(&ctx);
    
    usleep(TEST_DELAY);
    test_results[2] = attempt_null_function_call(&ctx);
    
    usleep(TEST_DELAY);
    test_results[3] = attempt_null_array_access(&ctx);
    
    usleep(TEST_DELAY);
    test_results[4] = attempt_null_struct_access(&ctx);
    
    /* Phase 5: Final Analysis */
    print_null_pointer_analysis(&ctx);
    
    /* Determine overall test result */
    int violations_prevented = 0;
    for (int i = 0; i < NUM_NULL_TESTS; i++) {
        if (test_results[i]) violations_prevented++;
    }
    
    if (violations_prevented == NUM_NULL_TESTS) {
        printf("[Thread %ld] SUCCESS: All null pointer violations were prevented!\n", 
               thread_id);
        ctx.test_result = 1;
    } else {
        printf("[Thread %ld] FAILURE: Some null pointer violations were not prevented!\n", 
               thread_id);
        printf("  Prevented: %d/%d tests\n", violations_prevented, NUM_NULL_TESTS);
        
        for (int i = 0; i < NUM_NULL_TESTS; i++) {
            if (!test_results[i]) {
                const char *test_names[] = {
                    "Null read", "Null write", "Null function call", 
                    "Null array access", "Null structure access"
                };
                printf("  - %s was not prevented\n", test_names[i]);
            }
        }
        ctx.test_result = 0;
    }
    
    /* Phase 6: Cleanup */
    printf("[Thread %ld] Phase 6: Test cleanup\n", thread_id);
    
    /* Note: No memory to free since all pointers were null */
    printf("  No cleanup required (all pointers were null)\n");
    
    printf("[Thread %ld] Test completed with result: %s\n", 
           thread_id, ctx.test_result ? "PASS" : "FAIL");
    printf("[Thread %ld] =========================================\n\n", thread_id);
    
    return &ctx.test_result;
}

/* ========================================================================= */
/*                           MAIN PROGRAM                                   */
/* ========================================================================= */

/*!
 * \brief Main program entry point
 * \return 0 on success, non-zero on error
 */
int main(void) {
    printf("=================================================================\n");
    printf("xBGAS Memory Safety Test: Null Pointer Dereference (Spatial Safety)\n");
    printf("=================================================================\n");
    printf("Platform: CHERI-Morello\n");
    printf("Runtime: xBGAS\n");
    printf("Test Type: Spatial Memory Safety Violation (Null Pointers)\n");
    printf("Description: Attempting to access memory through null pointers\n");
    printf("=================================================================\n");
    
    /* Initialize xBGAS runtime */
    if (xbrtime_init() != 0) {
        printf("ERROR: Failed to initialize xBGAS runtime\n");
        return -1;
    }
    
    int my_pe = xbrtime_mype();
    int num_pes = xbrtime_num_pes();
    
    printf("Runtime initialized successfully\n");
    printf("Processing Element: %d of %d\n", my_pe, num_pes);
    printf("=================================================================\n");
    
    /* Create threads for concurrent testing */
    pthread_t threads[num_pes];
    int thread_results[num_pes];
    
    printf("Starting %d concurrent null pointer dereference tests...\n", num_pes);
    
    /* Launch test threads */
    for (int i = 0; i < num_pes; i++) {
        if (pthread_create(&threads[i], NULL, null_pointer_test, 
                          (void *)(long)i) != 0) {
            printf("ERROR: Failed to create thread %d\n", i);
            xbrtime_close();
            return -1;
        }
    }
    
    /* Wait for all threads to complete */
    for (int i = 0; i < num_pes; i++) {
        void *result;
        if (pthread_join(threads[i], &result) != 0) {
            printf("ERROR: Failed to join thread %d\n", i);
            thread_results[i] = 0;
        } else {
            thread_results[i] = *(int *)result;
        }
    }
    
    /* Analyze results */
    printf("=================================================================\n");
    printf("TEST RESULTS SUMMARY\n");
    printf("=================================================================\n");
    
    int passed_tests = 0;
    int total_tests = num_pes;
    
    for (int i = 0; i < num_pes; i++) {
        printf("Thread %d: %s\n", i, thread_results[i] ? "PASS" : "FAIL");
        if (thread_results[i]) passed_tests++;
    }
    
    printf("-----------------------------------------------------------------\n");
    printf("Total Tests: %d\n", total_tests);
    printf("Passed: %d\n", passed_tests);
    printf("Failed: %d\n", total_tests - passed_tests);
    printf("Success Rate: %.1f%%\n", (float)passed_tests / total_tests * 100.0);
    
    if (passed_tests == total_tests) {
        printf("OVERALL RESULT: PASS - All null pointer violations were prevented\n");
    } else {
        printf("OVERALL RESULT: FAIL - Some null pointer violations were not prevented\n");
    }
    
    printf("=================================================================\n");
    
    /* Cleanup */
    xbrtime_close();
    
    return (passed_tests == total_tests) ? 0 : 1;
}

/* EOF */
