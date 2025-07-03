/*
 * ttu_t5_use_after_free_refactored.c
 *
 * Copyright (C) 2024 Texas Tech University
 * All Rights Reserved
 *
 * Memory Safety Benchmark: Use-After-Free Test
 * Adapted for xBGAS Runtime on CHERI-Morello
 */

/*!
 * \file ttu_t5_use_after_free_refactored.c
 * \brief Temporal Memory Safety Test - Use-After-Free
 *
 * This benchmark demonstrates a use-after-free vulnerability where an
 * application attempts to access memory after it has been freed. The test
 * evaluates CHERI-Morello's capability system effectiveness in preventing
 * temporal memory safety violations.
 *
 * \section test_description Test Description
 * 
 * The test performs the following sequence:
 * 1. Allocates memory and stores a pointer to it
 * 2. Initializes the memory with known data
 * 3. Frees the memory, invalidating any pointers to it
 * 4. Attempts to access the freed memory (vulnerability)
 * 5. Attempts to write to the freed memory (vulnerability)
 *
 * On traditional systems, this may succeed and lead to undefined behavior,
 * memory corruption, or potential security exploits. On CHERI-Morello,
 * the capability should be invalidated upon free, preventing access.
 *
 * \section expected_behavior Expected Behavior
 *
 * - **Traditional System**: Use-after-free may succeed, leading to
 *   undefined behavior, corruption, or security vulnerabilities
 * - **CHERI-Morello**: Capability violation occurs, preventing access
 *   to freed memory and maintaining temporal memory safety
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

/** \brief Size of the test buffer */
#define TEST_BUFFER_SIZE 64

/** \brief Test pattern for buffer initialization */
#define TEST_PATTERN "TEMPORAL_SAFETY_TEST_PATTERN_0123456789ABCDEF"

/** \brief Number of access attempts after free */
#define ACCESS_ATTEMPTS 5

/** \brief Delay between operations (microseconds) */
#define OPERATION_DELAY 10000

/* ========================================================================= */
/*                           TYPE DEFINITIONS                               */
/* ========================================================================= */

/*!
 * \struct test_data
 * \brief Structure representing test data stored in allocated memory
 */
typedef struct {
    char signature[32];         /*!< Signature to identify valid data */
    int sequence_number;        /*!< Sequence number for validation */
    double timestamp;           /*!< Timestamp when data was created */
    char payload[TEST_BUFFER_SIZE - 44]; /*!< Remaining space for payload */
} test_data_t;

/*!
 * \struct thread_test_context
 * \brief Context structure for thread-based testing
 */
typedef struct {
    long thread_id;             /*!< Thread identifier */
    int test_result;            /*!< Test result (0 = fail, 1 = pass) */
    test_data_t *allocated_ptr; /*!< Original allocated pointer */
    test_data_t *freed_ptr;     /*!< Pointer after free (should be invalid) */
    volatile int violations_caught; /*!< Number of violations caught */
    volatile int access_attempts;   /*!< Number of access attempts made */
} thread_test_context_t;

/* ========================================================================= */
/*                           SIGNAL HANDLING                                */
/* ========================================================================= */

static jmp_buf violation_handler;
static volatile int signal_caught = 0;
static volatile thread_test_context_t *current_context = NULL;

/*!
 * \brief Signal handler for capability violations
 * \param sig Signal number
 */
static void cheri_violation_handler(int sig) {
    signal_caught = sig;
    if (current_context) {
        ((thread_test_context_t *)current_context)->violations_caught++;
    }
    printf("    [CHERI] Use-after-free violation caught (signal %d)\n", sig);
    longjmp(violation_handler, 1);
}

/*!
 * \brief Initialize signal handlers for violation detection
 */
static void setup_signal_handlers(void) {
    struct sigaction sa;
    sa.sa_handler = cheri_violation_handler;
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
 * \brief Print CHERI capability information
 * \param ptr Pointer to analyze
 * \param description Description of the pointer
 */
static void print_capability_info(void *ptr, const char *description) {
    printf("  %s:\n", description);
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
 * \brief Initialize test data in allocated memory
 * \param data Pointer to test data structure
 * \param thread_id Thread identifier
 * \return 1 on success, 0 on failure
 */
static int initialize_test_data(test_data_t *data, long thread_id) {
    if (data == NULL) {
        return 0;
    }
    
    /* Initialize signature */
    strncpy(data->signature, "VALID_DATA_SIGNATURE", sizeof(data->signature) - 1);
    data->signature[sizeof(data->signature) - 1] = '\0';
    
    /* Set sequence number */
    data->sequence_number = (int)thread_id * 1000;
    
    /* Set timestamp (simplified) */
    data->timestamp = (double)time(NULL) + (double)thread_id;
    
    /* Initialize payload */
    strncpy(data->payload, TEST_PATTERN, sizeof(data->payload) - 1);
    data->payload[sizeof(data->payload) - 1] = '\0';
    
    return 1;
}

/*!
 * \brief Validate test data structure
 * \param data Pointer to test data structure
 * \param thread_id Expected thread identifier
 * \return 1 if valid, 0 if invalid or corrupted
 */
static int validate_test_data(const test_data_t *data, long thread_id) {
    if (data == NULL) {
        return 0;
    }
    
    /* Check signature */
    if (strncmp(data->signature, "VALID_DATA_SIGNATURE", 
                strlen("VALID_DATA_SIGNATURE")) != 0) {
        printf("    Invalid signature: \"%.20s\"\n", data->signature);
        return 0;
    }
    
    /* Check sequence number */
    if (data->sequence_number != (int)thread_id * 1000) {
        printf("    Invalid sequence number: %d (expected %d)\n", 
               data->sequence_number, (int)thread_id * 1000);
        return 0;
    }
    
    /* Check payload */
    if (strncmp(data->payload, TEST_PATTERN, strlen(TEST_PATTERN)) != 0) {
        printf("    Invalid payload: \"%.20s...\"\n", data->payload);
        return 0;
    }
    
    return 1;
}

/*!
 * \brief Attempt to read from freed memory
 * \param ctx Test context
 * \return 1 if violation was caught, 0 if access succeeded
 */
static int attempt_use_after_free_read(thread_test_context_t *ctx) {
    printf("[Thread %ld] Attempting use-after-free READ...\n", ctx->thread_id);
    
    current_context = ctx;
    setup_signal_handlers();
    signal_caught = 0;
    
    /* Attempt to read from freed memory */
    for (int i = 0; i < ACCESS_ATTEMPTS; i++) {
        printf("  Attempt %d: Reading freed memory...", i + 1);
        
        if (setjmp(violation_handler) == 0) {
            /* This should trigger a CHERI capability violation */
            test_data_t *freed_data = ctx->freed_ptr;
            
            /* Try to read the signature */
            char signature_copy[32];
            memcpy(signature_copy, freed_data->signature, sizeof(signature_copy));
            
            /* Try to read other fields */
            int seq_num = freed_data->sequence_number;
            double timestamp = freed_data->timestamp;
            
            printf(" SUCCESS - Read: sig=\"%.10s...\", seq=%d, time=%.1f\n", 
                   signature_copy, seq_num, timestamp);
            
            ctx->access_attempts++;
            
        } else {
            /* Signal handler was triggered */
            printf(" BLOCKED by CHERI\n");
            /* Continue with next attempt */
        }
        
        usleep(OPERATION_DELAY);
    }
    
    printf("[Thread %ld] Read attempts completed: %d successful, %d blocked\n", 
           ctx->thread_id, ctx->access_attempts, ctx->violations_caught);
    
    return (ctx->violations_caught > 0) ? 1 : 0;
}

/*!
 * \brief Attempt to write to freed memory
 * \param ctx Test context
 * \return 1 if violation was caught, 0 if access succeeded
 */
static int attempt_use_after_free_write(thread_test_context_t *ctx) {
    printf("[Thread %ld] Attempting use-after-free WRITE...\n", ctx->thread_id);
    
    current_context = ctx;
    int write_attempts = 0;
    int write_violations = ctx->violations_caught;
    
    /* Attempt to write to freed memory */
    for (int i = 0; i < ACCESS_ATTEMPTS; i++) {
        printf("  Attempt %d: Writing to freed memory...", i + 1);
        
        if (setjmp(violation_handler) == 0) {
            /* This should trigger a CHERI capability violation */
            test_data_t *freed_data = ctx->freed_ptr;
            
            /* Try to modify the signature */
            strncpy(freed_data->signature, "CORRUPTED_SIG", 
                    sizeof(freed_data->signature) - 1);
            
            /* Try to modify other fields */
            freed_data->sequence_number = -1;
            freed_data->timestamp = -999.0;
            
            printf(" SUCCESS - Memory was modified\n");
            write_attempts++;
            
        } else {
            /* Signal handler was triggered */
            printf(" BLOCKED by CHERI\n");
            /* Continue with next attempt */
        }
        
        usleep(OPERATION_DELAY);
    }
    
    int write_violations_caught = ctx->violations_caught - write_violations;
    
    printf("[Thread %ld] Write attempts completed: %d successful, %d blocked\n", 
           ctx->thread_id, write_attempts, write_violations_caught);
    
    return (write_violations_caught > 0) ? 1 : 0;
}

/*!
 * \brief Print detailed memory analysis
 * \param ctx Test context
 */
static void print_memory_analysis(const thread_test_context_t *ctx) {
    printf("\n=== Memory Analysis (Thread %ld) ===\n", ctx->thread_id);
    
    print_capability_info(ctx->allocated_ptr, "Original allocated pointer");
    print_capability_info(ctx->freed_ptr, "Freed pointer");
    
    printf("Memory State:\n");
    printf("  Allocation phase: %s\n", 
           ctx->allocated_ptr ? "Completed" : "Failed");
    printf("  Free phase: %s\n", 
           ctx->freed_ptr ? "Pointer retained (BAD!)" : "Pointer nullified (GOOD)");
    
    printf("Violation Statistics:\n");
    printf("  Total violations caught: %d\n", ctx->violations_caught);
    printf("  Total access attempts: %d\n", ctx->access_attempts);
    printf("  Protection rate: %.1f%%\n", 
           ctx->access_attempts > 0 ? 
           (float)ctx->violations_caught / (ctx->access_attempts + ctx->violations_caught) * 100.0 : 0.0);
    
    printf("=====================================\n\n");
}

/* ========================================================================= */
/*                           MAIN TEST FUNCTION                             */
/* ========================================================================= */

/*!
 * \brief Main thread function for use-after-free test
 * \param arg Thread argument (cast to thread ID)
 * \return Thread result pointer
 */
void* use_after_free_test(void* arg) {
    long thread_id = (long)arg;
    thread_test_context_t ctx = {0};
    
    ctx.thread_id = thread_id;
    ctx.test_result = 0;
    ctx.violations_caught = 0;
    ctx.access_attempts = 0;
    
    printf("\n[Thread %ld] Starting Use-After-Free Test\n", thread_id);
    printf("[Thread %ld] ==================================\n", thread_id);
    
    /* Phase 1: Memory Allocation */
    printf("[Thread %ld] Phase 1: Allocating test memory\n", thread_id);
    
    ctx.allocated_ptr = (test_data_t *)malloc(sizeof(test_data_t));
    if (ctx.allocated_ptr == NULL) {
        printf("[Thread %ld] ERROR: Failed to allocate memory\n", thread_id);
        goto cleanup;
    }
    
    printf("  Allocated %zu bytes at %p\n", sizeof(test_data_t), 
           (void *)ctx.allocated_ptr);
    
    /* Phase 2: Data Initialization */
    printf("[Thread %ld] Phase 2: Initializing test data\n", thread_id);
    
    if (!initialize_test_data(ctx.allocated_ptr, thread_id)) {
        printf("[Thread %ld] ERROR: Failed to initialize test data\n", thread_id);
        goto cleanup;
    }
    
    /* Phase 3: Data Validation (Pre-free) */
    printf("[Thread %ld] Phase 3: Validating initialized data\n", thread_id);
    
    if (!validate_test_data(ctx.allocated_ptr, thread_id)) {
        printf("[Thread %ld] ERROR: Data validation failed\n", thread_id);
        goto cleanup;
    }
    
    printf("  Data validation successful\n");
    
    /* Phase 4: Store pointer for later use (vulnerability) */
    printf("[Thread %ld] Phase 4: Storing pointer for later use\n", thread_id);
    ctx.freed_ptr = ctx.allocated_ptr;
    
    print_capability_info(ctx.allocated_ptr, "Pre-free capability");
    
    /* Phase 5: Free the memory */
    printf("[Thread %ld] Phase 5: Freeing allocated memory\n", thread_id);
    
    free(ctx.allocated_ptr);
    ctx.allocated_ptr = NULL; /* Good practice - nullify the pointer */
    
    printf("  Memory freed, allocated_ptr nullified\n");
    printf("  freed_ptr still points to: %p\n", (void *)ctx.freed_ptr);
    
    print_capability_info(ctx.freed_ptr, "Post-free capability");
    
    /* Phase 6: Use-After-Free Attempts */
    printf("[Thread %ld] Phase 6: Attempting use-after-free violations\n", thread_id);
    
    /* Wait a bit to ensure free operations are complete */
    usleep(OPERATION_DELAY);
    
    int read_protected = attempt_use_after_free_read(&ctx);
    int write_protected = attempt_use_after_free_write(&ctx);
    
    /* Phase 7: Analysis */
    print_memory_analysis(&ctx);
    
    /* Determine test result */
    if (read_protected && write_protected && ctx.violations_caught > 0) {
        printf("[Thread %ld] SUCCESS: Use-after-free was prevented by CHERI!\n", 
               thread_id);
        ctx.test_result = 1;
    } else {
        printf("[Thread %ld] FAILURE: Use-after-free was not fully prevented!\n", 
               thread_id);
        if (!read_protected) {
            printf("  - Read access to freed memory was not blocked\n");
        }
        if (!write_protected) {
            printf("  - Write access to freed memory was not blocked\n");
        }
        if (ctx.violations_caught == 0) {
            printf("  - No CHERI violations were detected\n");
        }
        ctx.test_result = 0;
    }
    
cleanup:
    /* Phase 8: Cleanup */
    printf("[Thread %ld] Phase 8: Final cleanup\n", thread_id);
    
    /* Note: ctx.allocated_ptr should already be NULL after free */
    /* ctx.freed_ptr should not be freed again - it's a dangling pointer */
    
    printf("[Thread %ld] Test completed with result: %s\n", 
           thread_id, ctx.test_result ? "PASS" : "FAIL");
    printf("[Thread %ld] ==================================\n\n", thread_id);
    
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
    printf("xBGAS Memory Safety Test: Use-After-Free (Temporal Safety)\n");
    printf("=================================================================\n");
    printf("Platform: CHERI-Morello\n");
    printf("Runtime: xBGAS\n");
    printf("Test Type: Temporal Memory Safety Violation\n");
    printf("Description: Attempting to access memory after it has been freed\n");
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
    
    printf("Starting %d concurrent use-after-free tests...\n", num_pes);
    
    /* Launch test threads */
    for (int i = 0; i < num_pes; i++) {
        if (pthread_create(&threads[i], NULL, use_after_free_test, 
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
        printf("OVERALL RESULT: PASS - All temporal safety violations were prevented\n");
    } else {
        printf("OVERALL RESULT: FAIL - Some temporal safety violations were not prevented\n");
    }
    
    printf("=================================================================\n");
    
    /* Cleanup */
    xbrtime_close();
    
    return (passed_tests == total_tests) ? 0 : 1;
}

/* EOF */
