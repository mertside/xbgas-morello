/*
 * ttu_s4_oob_read_refactored.c
 *
 * Copyright (C) 2024 Texas Tech University
 * All Rights Reserved
 *
 * Memory Safety Benchmark: Out-of-Bounds Read Test
 * Adapted for xBGAS Runtime on CHERI-Morello
 */

/*!
 * \file ttu_s4_oob_read_refactored.c
 * \brief Spatial Memory Safety Test - Out-of-Bounds Read
 *
 * This benchmark demonstrates an out-of-bounds read vulnerability where
 * an application attempts to read beyond the allocated boundaries of a
 * memory buffer. The test is designed to evaluate CHERI-Morello's
 * capability system effectiveness in preventing spatial memory safety
 * violations.
 *
 * \section test_description Test Description
 * 
 * The test allocates two separate memory buffers:
 * 1. A "public" buffer containing non-sensitive data
 * 2. A "private" buffer containing sensitive data (simulated password)
 *
 * The vulnerability occurs when the application attempts to read beyond
 * the bounds of the public buffer, potentially accessing the private
 * buffer's contents. On a traditional system, this could lead to
 * information disclosure. On CHERI-Morello, this should trigger a
 * capability violation.
 *
 * \section expected_behavior Expected Behavior
 *
 * - **Traditional System**: Out-of-bounds read succeeds, potentially
 *   exposing sensitive data from adjacent memory allocations
 * - **CHERI-Morello**: Capability violation occurs, preventing the
 *   out-of-bounds access and maintaining memory safety
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

/* ========================================================================= */
/*                           PROJECT INCLUDES                               */
/* ========================================================================= */

#include "xbrtime_morello.h"

/* ========================================================================= */
/*                           CONFIGURATION                                  */
/* ========================================================================= */

/** \brief Size of the public (non-sensitive) buffer */
#define PUBLIC_BUFFER_SIZE 6

/** \brief Size of the private (sensitive) buffer */
#define PRIVATE_BUFFER_SIZE 14

/** \brief Test string for public buffer */
#define PUBLIC_DATA "public"

/** \brief Test string for private buffer (simulated sensitive data) */
#define PRIVATE_DATA "secretpassword"

/* ========================================================================= */
/*                           TYPE DEFINITIONS                               */
/* ========================================================================= */

/*!
 * \struct thread_test_context
 * \brief Context structure for thread-based testing
 */
typedef struct {
    long thread_id;             /*!< Thread identifier */
    int test_result;            /*!< Test result (0 = fail, 1 = pass) */
    char *public_buffer;        /*!< Pointer to public buffer */
    char *private_buffer;       /*!< Pointer to private buffer */
} thread_test_context_t;

/* ========================================================================= */
/*                           UTILITY FUNCTIONS                              */
/* ========================================================================= */

/*!
 * \brief Print memory layout information for debugging
 * \param ctx Test context containing buffer pointers
 * 
 * This function displays detailed information about the allocated buffers,
 * including their addresses, CHERI capability metadata, and relative offsets.
 * This information is useful for understanding the memory layout and
 * analyzing the potential for out-of-bounds access.
 */
static void print_memory_layout(const thread_test_context_t *ctx) {
    printf("\n=== Memory Layout Analysis (Thread %ld) ===\n", ctx->thread_id);
    
    /* Display public buffer information */
    printf("Public Buffer:\n");
    printf("  Address: %p\n", (void *)ctx->public_buffer);
    printf("  Content: \"%s\"\n", ctx->public_buffer);
    printf("  Size: %d bytes\n", PUBLIC_BUFFER_SIZE);
    
    /* Display CHERI capability information for public buffer */
#ifdef __CHERI_PURE_CAPABILITY__
    printf("  CHERI Capability: %#p\n", (void *)ctx->public_buffer);
    printf("  Base: 0x%lx\n", cheri_base_get(ctx->public_buffer));
    printf("  Length: %lu\n", cheri_length_get(ctx->public_buffer));
    printf("  Offset: %lu\n", cheri_offset_get(ctx->public_buffer));
    printf("  Permissions: 0x%x\n", cheri_perms_get(ctx->public_buffer));
    printf("  Tag: %d\n", (int)cheri_tag_get(ctx->public_buffer));
#endif
    
    printf("\nPrivate Buffer:\n");
    printf("  Address: %p\n", (void *)ctx->private_buffer);
    printf("  Content: \"%s\"\n", ctx->private_buffer);
    printf("  Size: %d bytes\n", PRIVATE_BUFFER_SIZE);
    
    /* Display CHERI capability information for private buffer */
#ifdef __CHERI_PURE_CAPABILITY__
    printf("  CHERI Capability: %#p\n", (void *)ctx->private_buffer);
    printf("  Base: 0x%lx\n", cheri_base_get(ctx->private_buffer));
    printf("  Length: %lu\n", cheri_length_get(ctx->private_buffer));
    printf("  Offset: %lu\n", cheri_offset_get(ctx->private_buffer));
    printf("  Permissions: 0x%x\n", cheri_perms_get(ctx->private_buffer));
    printf("  Tag: %d\n", (int)cheri_tag_get(ctx->private_buffer));
#endif
    
    /* Calculate and display buffer offset */
    intptr_t offset = ctx->private_buffer - ctx->public_buffer;
    printf("\nBuffer Relationship:\n");
    printf("  Offset (private - public): %ld bytes\n", offset);
    printf("  Adjacent buffers: %s\n", 
           (abs(offset) <= PUBLIC_BUFFER_SIZE + PRIVATE_BUFFER_SIZE) ? "Yes" : "No");
    
    printf("===========================================\n\n");
}

/*!
 * \brief Perform safe buffer access (within bounds)
 * \param ctx Test context containing buffer pointers
 * \return 1 on success, 0 on failure
 * 
 * This function demonstrates safe memory access by reading data
 * only within the allocated bounds of the public buffer.
 */
static int perform_safe_access(thread_test_context_t *ctx) {
    printf("[Thread %ld] Performing safe buffer access...\n", ctx->thread_id);
    
    /* Read within bounds of public buffer */
    for (int i = 0; i < PUBLIC_BUFFER_SIZE - 1; i++) {
        char byte = ctx->public_buffer[i];
        printf("  public_buffer[%d] = '%c' (0x%02x)\n", i, 
               (byte >= 32 && byte <= 126) ? byte : '.', (unsigned char)byte);
    }
    
    printf("[Thread %ld] Safe access completed successfully\n", ctx->thread_id);
    return 1;
}

/*!
 * \brief Attempt out-of-bounds read (vulnerability demonstration)
 * \param ctx Test context containing buffer pointers
 * \return 1 if violation was caught, 0 if violation succeeded
 * 
 * This function intentionally attempts to read beyond the allocated
 * bounds of the public buffer. On CHERI-Morello, this should trigger
 * a capability violation. On traditional systems, this may succeed
 * and potentially expose sensitive data.
 */
static int attempt_oob_read(thread_test_context_t *ctx) {
    printf("[Thread %ld] Attempting out-of-bounds read...\n", ctx->thread_id);
    
    /* Calculate the potential offset to reach private buffer */
    intptr_t offset = ctx->private_buffer - ctx->public_buffer;
    
    printf("  Calculated offset to private buffer: %ld\n", offset);
    
    /* Attempt to read beyond the public buffer bounds */
    printf("  Attempting to read 16 bytes beyond public buffer...\n");
    
    for (int i = 0; i < 16; i++) {
        printf("  Attempting read at public_buffer[%d]...", 
               PUBLIC_BUFFER_SIZE + i);
        
        /* This is the vulnerable operation */
        char byte = ctx->public_buffer[PUBLIC_BUFFER_SIZE + i];
        
        printf(" Success: '%c' (0x%02x)\n", 
               (byte >= 32 && byte <= 126) ? byte : '.', (unsigned char)byte);
    }
    
    printf("[Thread %ld] Out-of-bounds read completed without violation!\n", 
           ctx->thread_id);
    printf("[Thread %ld] WARNING: Memory safety violation was not prevented!\n", 
           ctx->thread_id);
    
    return 0; /* Violation was not caught */
}

/* ========================================================================= */
/*                           MAIN TEST FUNCTION                             */
/* ========================================================================= */

/*!
 * \brief Main thread function for out-of-bounds read test
 * \param arg Thread argument (cast to thread ID)
 * \return Thread result pointer
 * 
 * This function implements the complete out-of-bounds read test,
 * including memory allocation, safe access demonstration, and
 * vulnerability attempt. It's designed to run in a multi-threaded
 * environment to test concurrent memory safety violations.
 */
void* out_of_bounds_read_test(void* arg) {
    long thread_id = (long)arg;
    thread_test_context_t ctx = {0};
    
    ctx.thread_id = thread_id;
    ctx.test_result = 0;
    
    printf("\n[Thread %ld] Starting Out-of-Bounds Read Test\n", thread_id);
    printf("[Thread %ld] =====================================\n", thread_id);
    
    /* Phase 1: Memory Allocation */
    printf("[Thread %ld] Phase 1: Allocating test buffers\n", thread_id);
    
    ctx.public_buffer = (char *)malloc(PUBLIC_BUFFER_SIZE);
    if (ctx.public_buffer == NULL) {
        printf("[Thread %ld] ERROR: Failed to allocate public buffer\n", thread_id);
        return &ctx.test_result;
    }
    
    ctx.private_buffer = (char *)malloc(PRIVATE_BUFFER_SIZE);
    if (ctx.private_buffer == NULL) {
        printf("[Thread %ld] ERROR: Failed to allocate private buffer\n", thread_id);
        free(ctx.public_buffer);
        return &ctx.test_result;
    }
    
    /* Phase 2: Buffer Initialization */
    printf("[Thread %ld] Phase 2: Initializing buffer contents\n", thread_id);
    
    strncpy(ctx.public_buffer, PUBLIC_DATA, PUBLIC_BUFFER_SIZE - 1);
    ctx.public_buffer[PUBLIC_BUFFER_SIZE - 1] = '\0';
    
    strncpy(ctx.private_buffer, PRIVATE_DATA, PRIVATE_BUFFER_SIZE - 1);
    ctx.private_buffer[PRIVATE_BUFFER_SIZE - 1] = '\0';
    
    /* Phase 3: Memory Layout Analysis */
    print_memory_layout(&ctx);
    
    /* Phase 4: Safe Access Demonstration */
    printf("[Thread %ld] Phase 4: Demonstrating safe access\n", thread_id);
    if (!perform_safe_access(&ctx)) {
        printf("[Thread %ld] ERROR: Safe access failed\n", thread_id);
        goto cleanup;
    }
    
    /* Phase 5: Vulnerability Attempt */
    printf("[Thread %ld] Phase 5: Attempting vulnerability exploit\n", thread_id);
    
    /* Note: On CHERI-Morello, this should trigger a capability violation */
    /* On traditional systems, this may succeed and expose sensitive data */
    int violation_caught = attempt_oob_read(&ctx);
    
    if (violation_caught) {
        printf("[Thread %ld] SUCCESS: Memory safety violation was prevented!\n", 
               thread_id);
        ctx.test_result = 1;
    } else {
        printf("[Thread %ld] FAILURE: Memory safety violation was not prevented!\n", 
               thread_id);
        ctx.test_result = 0;
    }
    
cleanup:
    /* Phase 6: Cleanup */
    printf("[Thread %ld] Phase 6: Cleaning up resources\n", thread_id);
    free(ctx.public_buffer);
    free(ctx.private_buffer);
    
    printf("[Thread %ld] Test completed with result: %s\n", 
           thread_id, ctx.test_result ? "PASS" : "FAIL");
    printf("[Thread %ld] =====================================\n\n", thread_id);
    
    return &ctx.test_result;
}

/* ========================================================================= */
/*                           MAIN PROGRAM                                   */
/* ========================================================================= */

/*!
 * \brief Main program entry point
 * \return 0 on success, non-zero on error
 * 
 * Initializes the xBGAS runtime and spawns multiple threads to perform
 * concurrent out-of-bounds read tests. This tests the effectiveness of
 * CHERI-Morello's memory safety protections under concurrent access.
 */
int main(void) {
    printf("=================================================================\n");
    printf("xBGAS Memory Safety Test: Out-of-Bounds Read (Spatial Safety)\n");
    printf("=================================================================\n");
    printf("Platform: CHERI-Morello\n");
    printf("Runtime: xBGAS\n");
    printf("Test Type: Spatial Memory Safety Violation\n");
    printf("Description: Attempting to read beyond allocated buffer bounds\n");
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
    
    printf("Starting %d concurrent out-of-bounds read tests...\n", num_pes);
    
    /* Launch test threads */
    for (int i = 0; i < num_pes; i++) {
        if (pthread_create(&threads[i], NULL, out_of_bounds_read_test, 
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
        printf("OVERALL RESULT: PASS - All memory safety violations were prevented\n");
    } else {
        printf("OVERALL RESULT: FAIL - Some memory safety violations were not prevented\n");
    }
    
    printf("=================================================================\n");
    
    /* Cleanup */
    xbrtime_close();
    
    return (passed_tests == total_tests) ? 0 : 1;
}

/* EOF */
