/*
 * ttu_t1_double_free_refactored.c
 *
 * Copyright (C) 2024 Texas Tech University
 * All Rights Reserved
 *
 * Memory Safety Benchmark: Double-Free Test
 * Adapted for xBGAS Runtime on CHERI-Morello
 */

/*!
 * \file ttu_t1_double_free_refactored.c
 * \brief Temporal Memory Safety Test - Double-Free
 *
 * This benchmark demonstrates a double-free vulnerability where an
 * application attempts to free the same memory allocation twice. The test
 * evaluates CHERI-Morello's capability system and runtime effectiveness
 * in preventing temporal memory management violations.
 *
 * \section test_description Test Description
 * 
 * The test performs the following sequence:
 * 1. Allocates memory and stores multiple pointers to it
 * 2. Initializes the memory with known data
 * 3. Frees the memory through the first pointer
 * 4. Attempts to free the same memory through a second pointer (vulnerability)
 * 5. Attempts to free through a third pointer (additional vulnerability)
 *
 * Double-free vulnerabilities can lead to heap corruption, crashes, or
 * security exploits in traditional systems. CHERI-Morello should prevent
 * these through capability invalidation and runtime checks.
 *
 * \section expected_behavior Expected Behavior
 *
 * - **Traditional System**: Double-free may cause heap corruption,
 *   crashes, or exploitable conditions
 * - **CHERI-Morello**: Runtime or capability system prevents double-free,
 *   maintaining heap integrity and temporal memory safety
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
#include <errno.h>

/* ========================================================================= */
/*                           PROJECT INCLUDES                               */
/* ========================================================================= */

#include "xbrtime_morello.h"

/* ========================================================================= */
/*                           CONFIGURATION                                  */
/* ========================================================================= */

/** \brief Size of the test allocation */
#define ALLOCATION_SIZE 128

/** \brief Test pattern for memory initialization */
#define TEST_PATTERN "DOUBLE_FREE_TEST_PATTERN_ABCDEFGHIJKLMNOPQRSTUVWXYZ_0123456789"

/** \brief Number of pointers to the same allocation */
#define NUM_POINTERS 3

/** \brief Delay between operations (microseconds) */
#define OPERATION_DELAY 5000

/* ========================================================================= */
/*                           TYPE DEFINITIONS                               */
/* ========================================================================= */

/*!
 * \struct allocation_info
 * \brief Information about an allocation for tracking
 */
typedef struct {
    char signature[32];         /*!< Signature for identification */
    size_t allocation_size;     /*!< Size of the allocation */
    long thread_id;             /*!< Thread that made the allocation */
    int sequence_number;        /*!< Sequence number for this allocation */
    char data_payload[ALLOCATION_SIZE - 48]; /*!< Remaining space for data */
} allocation_info_t;

/*!
 * \struct thread_test_context
 * \brief Context structure for thread-based testing
 */
typedef struct {
    long thread_id;             /*!< Thread identifier */
    int test_result;            /*!< Test result (0 = fail, 1 = pass) */
    allocation_info_t *pointers[NUM_POINTERS]; /*!< Multiple pointers to same allocation */
    int free_attempts;          /*!< Number of free attempts made */
    int free_successes;         /*!< Number of successful frees */
    int free_failures;          /*!< Number of failed frees */
    volatile int violations_caught; /*!< Number of violations caught */
} thread_test_context_t;

/* ========================================================================= */
/*                           SIGNAL HANDLING                                */
/* ========================================================================= */

static jmp_buf violation_handler;
static volatile int signal_caught = 0;
static volatile thread_test_context_t *current_context = NULL;

/*!
 * \brief Signal handler for runtime errors and capability violations
 * \param sig Signal number
 */
static void runtime_error_handler(int sig) {
    signal_caught = sig;
    if (current_context) {
        ((thread_test_context_t *)current_context)->violations_caught++;
    }
    
    const char *sig_name;
    switch (sig) {
        case SIGABRT: sig_name = "SIGABRT (Abort)"; break;
        case SIGBUS: sig_name = "SIGBUS (Bus Error)"; break;
        case SIGSEGV: sig_name = "SIGSEGV (Segmentation Fault)"; break;
#ifdef SIGPROT
        case SIGPROT: sig_name = "SIGPROT (Protection Violation)"; break;
#endif
        default: sig_name = "Unknown Signal"; break;
    }
    
    printf("    [RUNTIME] Double-free violation caught: %s (%d)\n", sig_name, sig);
    longjmp(violation_handler, 1);
}

/*!
 * \brief Initialize signal handlers for violation detection
 */
static void setup_signal_handlers(void) {
    struct sigaction sa;
    sa.sa_handler = runtime_error_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    
    sigaction(SIGABRT, &sa, NULL);  /* Abort (often from runtime checks) */
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
 * \brief Print CHERI capability information for a pointer
 * \param ptr Pointer to analyze
 * \param description Description of the pointer
 * \param index Index in pointer array (or -1 if not applicable)
 */
static void print_capability_info(void *ptr, const char *description, int index) {
    if (index >= 0) {
        printf("  %s[%d]:\n", description, index);
    } else {
        printf("  %s:\n", description);
    }
    
    printf("    Address: %p\n", ptr);
    
#ifdef __CHERI_PURE_CAPABILITY__
    if (ptr != NULL) {
        printf("    CHERI Capability: %#p\n", ptr);
        printf("    Base: 0x%lx\n", cheri_base_get(ptr));
        printf("    Length: %lu\n", cheri_length_get(ptr));
        printf("    Offset: %lu\n", cheri_offset_get(ptr));
        printf("    Permissions: 0x%x\n", cheri_perms_get(ptr));
        printf("    Tag: %d\n", (int)cheri_tag_get(ptr));
        printf("    Valid: %s\n", cheri_tag_get(ptr) ? "Yes" : "No");
    } else {
        printf("    NULL pointer\n");
    }
#else
    printf("    (CHERI capability information not available)\n");
#endif
}

/*!
 * \brief Initialize allocation with test data
 * \param alloc Pointer to allocation
 * \param thread_id Thread identifier
 * \param sequence Sequence number
 * \return 1 on success, 0 on failure
 */
static int initialize_allocation(allocation_info_t *alloc, long thread_id, int sequence) {
    if (alloc == NULL) {
        return 0;
    }
    
    /* Initialize signature */
    strncpy(alloc->signature, "VALID_ALLOCATION", sizeof(alloc->signature) - 1);
    alloc->signature[sizeof(alloc->signature) - 1] = '\0';
    
    /* Set metadata */
    alloc->allocation_size = ALLOCATION_SIZE;
    alloc->thread_id = thread_id;
    alloc->sequence_number = sequence;
    
    /* Initialize payload */
    strncpy(alloc->data_payload, TEST_PATTERN, sizeof(alloc->data_payload) - 1);
    alloc->data_payload[sizeof(alloc->data_payload) - 1] = '\0';
    
    return 1;
}

/*!
 * \brief Validate allocation data
 * \param alloc Pointer to allocation
 * \param thread_id Expected thread identifier
 * \param sequence Expected sequence number
 * \return 1 if valid, 0 if invalid
 */
static int validate_allocation(const allocation_info_t *alloc, long thread_id, int sequence) {
    if (alloc == NULL) {
        return 0;
    }
    
    /* Check signature */
    if (strncmp(alloc->signature, "VALID_ALLOCATION", 
                strlen("VALID_ALLOCATION")) != 0) {
        printf("    Invalid signature: \"%.16s\"\n", alloc->signature);
        return 0;
    }
    
    /* Check metadata */
    if (alloc->thread_id != thread_id || alloc->sequence_number != sequence) {
        printf("    Invalid metadata: thread=%ld (expected %ld), seq=%d (expected %d)\n",
               alloc->thread_id, thread_id, alloc->sequence_number, sequence);
        return 0;
    }
    
    return 1;
}

/*!
 * \brief Attempt to free memory with error handling
 * \param ctx Test context
 * \param pointer_index Index of pointer to free
 * \return 1 if free succeeded, 0 if it was prevented
 */
static int attempt_free_with_protection(thread_test_context_t *ctx, int pointer_index) {
    printf("[Thread %ld] Attempting free via pointer[%d]...\n", 
           ctx->thread_id, pointer_index);
    
    if (pointer_index < 0 || pointer_index >= NUM_POINTERS) {
        printf("  ERROR: Invalid pointer index %d\n", pointer_index);
        return 0;
    }
    
    if (ctx->pointers[pointer_index] == NULL) {
        printf("  INFO: Pointer[%d] is already NULL\n", pointer_index);
        return 0;
    }
    
    print_capability_info(ctx->pointers[pointer_index], "Pre-free pointer", pointer_index);
    
    current_context = ctx;
    setup_signal_handlers();
    signal_caught = 0;
    
    ctx->free_attempts++;
    
    if (setjmp(violation_handler) == 0) {
        /* Attempt the free operation */
        printf("  Calling free() on pointer[%d] = %p\n", 
               pointer_index, (void *)ctx->pointers[pointer_index]);
        
        free(ctx->pointers[pointer_index]);
        
        printf("  Free completed successfully\n");
        ctx->free_successes++;
        
        /* Nullify the pointer after successful free */
        ctx->pointers[pointer_index] = NULL;
        
        return 1; /* Free succeeded */
        
    } else {
        /* Signal handler was triggered */
        printf("  Free was prevented by runtime protection\n");
        ctx->free_failures++;
        return 0; /* Free was prevented */
    }
}

/*!
 * \brief Print detailed memory analysis
 * \param ctx Test context
 */
static void print_memory_analysis(const thread_test_context_t *ctx) {
    printf("\n=== Memory Analysis (Thread %ld) ===\n", ctx->thread_id);
    
    printf("Pointer States:\n");
    for (int i = 0; i < NUM_POINTERS; i++) {
        print_capability_info(ctx->pointers[i], "Pointer", i);
    }
    
    printf("Free Operation Statistics:\n");
    printf("  Total free attempts: %d\n", ctx->free_attempts);
    printf("  Successful frees: %d\n", ctx->free_successes);
    printf("  Prevented frees: %d\n", ctx->free_failures);
    printf("  Violations caught: %d\n", ctx->violations_caught);
    
    /* Analysis */
    if (ctx->free_successes == 1 && ctx->free_failures > 0) {
        printf("  ANALYSIS: Proper double-free prevention detected\n");
    } else if (ctx->free_successes > 1) {
        printf("  ANALYSIS: Multiple frees succeeded - potential vulnerability\n");
    } else if (ctx->free_successes == 0) {
        printf("  ANALYSIS: No frees succeeded - possible over-protection\n");
    }
    
    printf("=====================================\n\n");
}

/* ========================================================================= */
/*                           MAIN TEST FUNCTION                             */
/* ========================================================================= */

/*!
 * \brief Main thread function for double-free test
 * \param arg Thread argument (cast to thread ID)
 * \return Thread result pointer
 */
void* double_free_test(void* arg) {
    long thread_id = (long)arg;
    thread_test_context_t ctx = {0};
    
    ctx.thread_id = thread_id;
    ctx.test_result = 0;
    
    printf("\n[Thread %ld] Starting Double-Free Test\n", thread_id);
    printf("[Thread %ld] ============================\n", thread_id);
    
    /* Phase 1: Memory Allocation */
    printf("[Thread %ld] Phase 1: Allocating test memory\n", thread_id);
    
    allocation_info_t *original_ptr = (allocation_info_t *)malloc(sizeof(allocation_info_t));
    if (original_ptr == NULL) {
        printf("[Thread %ld] ERROR: Failed to allocate memory\n", thread_id);
        goto cleanup;
    }
    
    printf("  Allocated %zu bytes at %p\n", sizeof(allocation_info_t), 
           (void *)original_ptr);
    
    /* Phase 2: Create multiple pointers to the same allocation */
    printf("[Thread %ld] Phase 2: Creating multiple pointers\n", thread_id);
    
    for (int i = 0; i < NUM_POINTERS; i++) {
        ctx.pointers[i] = original_ptr;
        printf("  pointer[%d] = %p\n", i, (void *)ctx.pointers[i]);
    }
    
    /* Phase 3: Initialize the allocation */
    printf("[Thread %ld] Phase 3: Initializing allocation data\n", thread_id);
    
    if (!initialize_allocation(original_ptr, thread_id, 1)) {
        printf("[Thread %ld] ERROR: Failed to initialize allocation\n", thread_id);
        goto cleanup;
    }
    
    /* Phase 4: Validate initial state */
    printf("[Thread %ld] Phase 4: Validating initial state\n", thread_id);
    
    if (!validate_allocation(original_ptr, thread_id, 1)) {
        printf("[Thread %ld] ERROR: Initial validation failed\n", thread_id);
        goto cleanup;
    }
    
    printf("  Initial validation successful\n");
    
    /* Print initial capability information */
    for (int i = 0; i < NUM_POINTERS; i++) {
        print_capability_info(ctx.pointers[i], "Initial pointer", i);
    }
    
    /* Phase 5: First free (should succeed) */
    printf("[Thread %ld] Phase 5: First free operation\n", thread_id);
    
    usleep(OPERATION_DELAY);
    int first_free_result = attempt_free_with_protection(&ctx, 0);
    
    if (first_free_result) {
        printf("  First free completed successfully\n");
    } else {
        printf("  First free was unexpectedly prevented\n");
    }
    
    /* Phase 6: Second free (should be prevented) */
    printf("[Thread %ld] Phase 6: Second free operation (double-free attempt)\n", thread_id);
    
    usleep(OPERATION_DELAY);
    int second_free_result = attempt_free_with_protection(&ctx, 1);
    
    if (second_free_result) {
        printf("  WARNING: Second free succeeded - double-free not prevented!\n");
    } else {
        printf("  SUCCESS: Second free was prevented\n");
    }
    
    /* Phase 7: Third free (additional double-free attempt) */
    printf("[Thread %ld] Phase 7: Third free operation (additional double-free attempt)\n", thread_id);
    
    usleep(OPERATION_DELAY);
    int third_free_result = attempt_free_with_protection(&ctx, 2);
    
    if (third_free_result) {
        printf("  WARNING: Third free succeeded - double-free not prevented!\n");
    } else {
        printf("  SUCCESS: Third free was prevented\n");
    }
    
    /* Phase 8: Analysis */
    print_memory_analysis(&ctx);
    
    /* Determine test result */
    if (first_free_result && !second_free_result && !third_free_result) {
        printf("[Thread %ld] SUCCESS: Double-free was properly prevented!\n", thread_id);
        ctx.test_result = 1;
    } else {
        printf("[Thread %ld] FAILURE: Double-free prevention was not effective!\n", thread_id);
        if (!first_free_result) {
            printf("  - First free was unexpectedly prevented\n");
        }
        if (second_free_result) {
            printf("  - Second free was not prevented (double-free vulnerability)\n");
        }
        if (third_free_result) {
            printf("  - Third free was not prevented (additional vulnerability)\n");
        }
        ctx.test_result = 0;
    }
    
cleanup:
    /* Phase 9: Cleanup */
    printf("[Thread %ld] Phase 9: Final cleanup\n", thread_id);
    
    /* Note: If the first free succeeded, the memory is already freed */
    /* The remaining pointers should not be freed again */
    for (int i = 0; i < NUM_POINTERS; i++) {
        if (ctx.pointers[i] != NULL) {
            printf("  WARNING: pointer[%d] is still non-NULL after test\n", i);
            /* Do NOT free these - they point to already-freed memory */
            ctx.pointers[i] = NULL;
        }
    }
    
    printf("[Thread %ld] Test completed with result: %s\n", 
           thread_id, ctx.test_result ? "PASS" : "FAIL");
    printf("[Thread %ld] ============================\n\n", thread_id);
    
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
    printf("xBGAS Memory Safety Test: Double-Free (Temporal Safety)\n");
    printf("=================================================================\n");
    printf("Platform: CHERI-Morello\n");
    printf("Runtime: xBGAS\n");
    printf("Test Type: Temporal Memory Management Violation\n");
    printf("Description: Attempting to free the same memory allocation twice\n");
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
    
    printf("Starting %d concurrent double-free tests...\n", num_pes);
    
    /* Launch test threads */
    for (int i = 0; i < num_pes; i++) {
        if (pthread_create(&threads[i], NULL, double_free_test, 
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
        printf("OVERALL RESULT: PASS - All double-free violations were prevented\n");
    } else {
        printf("OVERALL RESULT: FAIL - Some double-free violations were not prevented\n");
    }
    
    printf("=================================================================\n");
    
    /* Cleanup */
    xbrtime_close();
    
    return (passed_tests == total_tests) ? 0 : 1;
}

/* EOF */
