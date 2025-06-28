/*
 * ttu_s5_oob_write_refactored.c
 *
 * Copyright (C) 2024 Texas Tech University
 * All Rights Reserved
 *
 * Memory Safety Benchmark: Out-of-Bounds Write Test
 * Adapted for xBGAS Runtime on CHERI-Morello
 */

/*!
 * \file ttu_s5_oob_write_refactored.c
 * \brief Spatial Memory Safety Test - Out-of-Bounds Write
 *
 * This benchmark demonstrates an out-of-bounds write vulnerability where
 * an application attempts to write beyond the allocated boundaries of a
 * memory buffer. The test evaluates CHERI-Morello's capability system
 * effectiveness in preventing spatial memory corruption.
 *
 * \section test_description Test Description
 * 
 * The test allocates two separate memory buffers:
 * 1. A "source" buffer containing data to be copied
 * 2. A "target" buffer with insufficient space for the full copy
 * 3. A "protected" buffer that should remain unmodified
 *
 * The vulnerability occurs when the application attempts to copy more data
 * than the target buffer can hold, potentially overwriting adjacent memory.
 * On traditional systems, this could lead to memory corruption and potential
 * code execution. On CHERI-Morello, this should trigger a capability violation.
 *
 * \section expected_behavior Expected Behavior
 *
 * - **Traditional System**: Out-of-bounds write succeeds, potentially
 *   corrupting adjacent memory and compromising system integrity
 * - **CHERI-Morello**: Capability violation occurs, preventing the
 *   out-of-bounds write and maintaining memory safety
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

/* ========================================================================= */
/*                           PROJECT INCLUDES                               */
/* ========================================================================= */

#include "xbrtime_morello.h"

/* ========================================================================= */
/*                           CONFIGURATION                                  */
/* ========================================================================= */

/** \brief Size of the source buffer (data to copy) */
#define SOURCE_BUFFER_SIZE 32

/** \brief Size of the target buffer (insufficient for full copy) */
#define TARGET_BUFFER_SIZE 16

/** \brief Size of the protected buffer (should remain unchanged) */
#define PROTECTED_BUFFER_SIZE 16

/** \brief Test pattern for source buffer */
#define SOURCE_PATTERN "ABCDEFGHIJKLMNOPQRSTUVWXYZ123456"

/** \brief Test pattern for protected buffer */
#define PROTECTED_PATTERN "PROTECTED_DATA!!"

/** \brief Canary value for memory corruption detection */
#define CANARY_VALUE 0xDEADBEEF

/* ========================================================================= */
/*                           TYPE DEFINITIONS                               */
/* ========================================================================= */

/*!
 * \struct memory_layout
 * \brief Structure representing the test memory layout
 */
typedef struct {
    char *source_buffer;        /*!< Source data buffer */
    char *target_buffer;        /*!< Target buffer (vulnerable) */
    char *protected_buffer;     /*!< Protected buffer (should not be modified) */
    uint32_t *canary;          /*!< Canary value for corruption detection */
} memory_layout_t;

/*!
 * \struct thread_test_context
 * \brief Context structure for thread-based testing
 */
typedef struct {
    long thread_id;             /*!< Thread identifier */
    int test_result;            /*!< Test result (0 = fail, 1 = pass) */
    memory_layout_t layout;     /*!< Memory layout for this thread */
    volatile int violation_caught; /*!< Flag indicating if violation was caught */
} thread_test_context_t;

/* ========================================================================= */
/*                           SIGNAL HANDLING                                */
/* ========================================================================= */

static jmp_buf violation_handler;
static volatile int signal_caught = 0;

/*!
 * \brief Signal handler for capability violations
 * \param sig Signal number
 * 
 * This handler catches CHERI capability violations (SIGPROT) and other
 * memory-related signals, allowing the test to detect and report when
 * the CHERI protection system prevents out-of-bounds writes.
 */
static void cheri_violation_handler(int sig) {
    signal_caught = sig;
    printf("    [CHERI] Capability violation caught (signal %d)\n", sig);
    longjmp(violation_handler, 1);
}

/* ========================================================================= */
/*                           UTILITY FUNCTIONS                              */
/* ========================================================================= */

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

/*!
 * \brief Print detailed memory layout information
 * \param ctx Test context containing memory layout
 */
static void print_memory_layout(const thread_test_context_t *ctx) {
    printf("\n=== Memory Layout Analysis (Thread %ld) ===\n", ctx->thread_id);
    
    /* Display source buffer information */
    printf("Source Buffer:\n");
    printf("  Address: %p\n", (void *)ctx->layout.source_buffer);
    printf("  Content: \"%.32s\"\n", ctx->layout.source_buffer);
    printf("  Size: %d bytes\n", SOURCE_BUFFER_SIZE);
    
    /* Display CHERI capability information for source buffer */
#ifdef __CHERI_PURE_CAPABILITY__
    printf("  CHERI Capability: %#p\n", (void *)ctx->layout.source_buffer);
    printf("  Base: 0x%lx\n", cheri_base_get(ctx->layout.source_buffer));
    printf("  Length: %lu\n", cheri_length_get(ctx->layout.source_buffer));
    printf("  Permissions: 0x%x\n", cheri_perms_get(ctx->layout.source_buffer));
    printf("  Tag: %d\n", (int)cheri_tag_get(ctx->layout.source_buffer));
#endif
    
    printf("\nTarget Buffer (Vulnerable):\n");
    printf("  Address: %p\n", (void *)ctx->layout.target_buffer);
    printf("  Size: %d bytes (insufficient for %d byte copy)\n", 
           TARGET_BUFFER_SIZE, SOURCE_BUFFER_SIZE);
    
#ifdef __CHERI_PURE_CAPABILITY__
    printf("  CHERI Capability: %#p\n", (void *)ctx->layout.target_buffer);
    printf("  Base: 0x%lx\n", cheri_base_get(ctx->layout.target_buffer));
    printf("  Length: %lu\n", cheri_length_get(ctx->layout.target_buffer));
    printf("  Permissions: 0x%x\n", cheri_perms_get(ctx->layout.target_buffer));
    printf("  Tag: %d\n", (int)cheri_tag_get(ctx->layout.target_buffer));
#endif
    
    printf("\nProtected Buffer:\n");
    printf("  Address: %p\n", (void *)ctx->layout.protected_buffer);
    printf("  Content: \"%.16s\"\n", ctx->layout.protected_buffer);
    printf("  Size: %d bytes\n", PROTECTED_BUFFER_SIZE);
    
    printf("\nCanary Value:\n");
    printf("  Address: %p\n", (void *)ctx->layout.canary);
    printf("  Value: 0x%08x\n", *ctx->layout.canary);
    
    /* Calculate buffer relationships */
    intptr_t target_to_protected = ctx->layout.protected_buffer - ctx->layout.target_buffer;
    intptr_t target_to_canary = (char *)ctx->layout.canary - ctx->layout.target_buffer;
    
    printf("\nBuffer Relationships:\n");
    printf("  Target to Protected offset: %ld bytes\n", target_to_protected);
    printf("  Target to Canary offset: %ld bytes\n", target_to_canary);
    printf("  Overflow potential: %d bytes beyond target buffer\n", 
           SOURCE_BUFFER_SIZE - TARGET_BUFFER_SIZE);
    
    printf("===========================================\n\n");
}

/*!
 * \brief Perform safe copy operation (within bounds)
 * \param ctx Test context
 * \return 1 on success, 0 on failure
 */
static int perform_safe_copy(thread_test_context_t *ctx) {
    printf("[Thread %ld] Performing safe copy operation...\n", ctx->thread_id);
    
    /* Copy only what fits in the target buffer */
    size_t safe_copy_size = TARGET_BUFFER_SIZE - 1; /* Leave room for null terminator */
    
    memcpy(ctx->layout.target_buffer, ctx->layout.source_buffer, safe_copy_size);
    ctx->layout.target_buffer[safe_copy_size] = '\0';
    
    printf("  Copied %zu bytes safely\n", safe_copy_size);
    printf("  Target buffer content: \"%.16s\"\n", ctx->layout.target_buffer);
    
    /* Verify protected buffer and canary are unchanged */
    if (strncmp(ctx->layout.protected_buffer, PROTECTED_PATTERN, 
                strlen(PROTECTED_PATTERN)) == 0 &&
        *ctx->layout.canary == CANARY_VALUE) {
        printf("[Thread %ld] Safe copy completed successfully\n", ctx->thread_id);
        return 1;
    } else {
        printf("[Thread %ld] ERROR: Safe copy corrupted adjacent memory!\n", ctx->thread_id);
        return 0;
    }
}

/*!
 * \brief Attempt vulnerable copy operation (out-of-bounds write)
 * \param ctx Test context
 * \return 1 if violation was caught, 0 if violation succeeded
 */
static int attempt_vulnerable_copy(thread_test_context_t *ctx) {
    printf("[Thread %ld] Attempting vulnerable copy operation...\n", ctx->thread_id);
    printf("  Attempting to copy %d bytes into %d byte buffer...\n", 
           SOURCE_BUFFER_SIZE, TARGET_BUFFER_SIZE);
    
    /* Setup signal handling for this attempt */
    setup_signal_handlers();
    signal_caught = 0;
    ctx->violation_caught = 0;
    
    /* Attempt the vulnerable operation */
    if (setjmp(violation_handler) == 0) {
        /* This should trigger a CHERI capability violation */
        memcpy(ctx->layout.target_buffer, ctx->layout.source_buffer, SOURCE_BUFFER_SIZE);
        
        printf("  Copy completed without violation!\n");
        printf("  Target buffer content: \"%.32s\"\n", ctx->layout.target_buffer);
        
        /* Check for memory corruption */
        printf("  Checking for memory corruption...\n");
        printf("  Protected buffer: \"%.16s\"\n", ctx->layout.protected_buffer);
        printf("  Canary value: 0x%08x\n", *ctx->layout.canary);
        
        if (strncmp(ctx->layout.protected_buffer, PROTECTED_PATTERN, 
                    strlen(PROTECTED_PATTERN)) != 0) {
            printf("[Thread %ld] WARNING: Protected buffer was corrupted!\n", ctx->thread_id);
        }
        
        if (*ctx->layout.canary != CANARY_VALUE) {
            printf("[Thread %ld] WARNING: Canary value was overwritten!\n", ctx->thread_id);
        }
        
        printf("[Thread %ld] FAILURE: Out-of-bounds write was not prevented!\n", 
               ctx->thread_id);
        return 0; /* Violation was not caught */
        
    } else {
        /* Signal handler was triggered */
        ctx->violation_caught = 1;
        printf("[Thread %ld] SUCCESS: CHERI prevented the out-of-bounds write!\n", 
               ctx->thread_id);
        return 1; /* Violation was caught */
    }
}

/*!
 * \brief Verify memory integrity after test
 * \param ctx Test context
 * \return 1 if integrity maintained, 0 if corrupted
 */
static int verify_memory_integrity(const thread_test_context_t *ctx) {
    printf("[Thread %ld] Verifying memory integrity...\n", ctx->thread_id);
    
    int integrity_ok = 1;
    
    /* Check protected buffer */
    if (strncmp(ctx->layout.protected_buffer, PROTECTED_PATTERN, 
                strlen(PROTECTED_PATTERN)) != 0) {
        printf("  CORRUPTION: Protected buffer was modified\n");
        printf("  Expected: \"%.16s\"\n", PROTECTED_PATTERN);
        printf("  Actual:   \"%.16s\"\n", ctx->layout.protected_buffer);
        integrity_ok = 0;
    } else {
        printf("  ✓ Protected buffer integrity maintained\n");
    }
    
    /* Check canary */
    if (*ctx->layout.canary != CANARY_VALUE) {
        printf("  CORRUPTION: Canary value was overwritten\n");
        printf("  Expected: 0x%08x\n", CANARY_VALUE);
        printf("  Actual:   0x%08x\n", *ctx->layout.canary);
        integrity_ok = 0;
    } else {
        printf("  ✓ Canary value integrity maintained\n");
    }
    
    return integrity_ok;
}

/* ========================================================================= */
/*                           MAIN TEST FUNCTION                             */
/* ========================================================================= */

/*!
 * \brief Main thread function for out-of-bounds write test
 * \param arg Thread argument (cast to thread ID)
 * \return Thread result pointer
 */
void* out_of_bounds_write_test(void* arg) {
    long thread_id = (long)arg;
    thread_test_context_t ctx = {0};
    
    ctx.thread_id = thread_id;
    ctx.test_result = 0;
    
    printf("\n[Thread %ld] Starting Out-of-Bounds Write Test\n", thread_id);
    printf("[Thread %ld] ======================================\n", thread_id);
    
    /* Phase 1: Memory Allocation */
    printf("[Thread %ld] Phase 1: Allocating test buffers\n", thread_id);
    
    ctx.layout.source_buffer = (char *)malloc(SOURCE_BUFFER_SIZE);
    ctx.layout.target_buffer = (char *)malloc(TARGET_BUFFER_SIZE);
    ctx.layout.protected_buffer = (char *)malloc(PROTECTED_BUFFER_SIZE);
    ctx.layout.canary = (uint32_t *)malloc(sizeof(uint32_t));
    
    if (!ctx.layout.source_buffer || !ctx.layout.target_buffer || 
        !ctx.layout.protected_buffer || !ctx.layout.canary) {
        printf("[Thread %ld] ERROR: Failed to allocate memory\n", thread_id);
        goto cleanup;
    }
    
    /* Phase 2: Buffer Initialization */
    printf("[Thread %ld] Phase 2: Initializing buffer contents\n", thread_id);
    
    strncpy(ctx.layout.source_buffer, SOURCE_PATTERN, SOURCE_BUFFER_SIZE - 1);
    ctx.layout.source_buffer[SOURCE_BUFFER_SIZE - 1] = '\0';
    
    memset(ctx.layout.target_buffer, 0, TARGET_BUFFER_SIZE);
    
    strncpy(ctx.layout.protected_buffer, PROTECTED_PATTERN, PROTECTED_BUFFER_SIZE - 1);
    ctx.layout.protected_buffer[PROTECTED_BUFFER_SIZE - 1] = '\0';
    
    *ctx.layout.canary = CANARY_VALUE;
    
    /* Phase 3: Memory Layout Analysis */
    print_memory_layout(&ctx);
    
    /* Phase 4: Safe Copy Demonstration */
    printf("[Thread %ld] Phase 4: Demonstrating safe copy\n", thread_id);
    if (!perform_safe_copy(&ctx)) {
        printf("[Thread %ld] ERROR: Safe copy failed\n", thread_id);
        goto cleanup;
    }
    
    /* Phase 5: Reset target buffer for vulnerability test */
    printf("[Thread %ld] Phase 5: Resetting for vulnerability test\n", thread_id);
    memset(ctx.layout.target_buffer, 0, TARGET_BUFFER_SIZE);
    
    /* Phase 6: Vulnerability Attempt */
    printf("[Thread %ld] Phase 6: Attempting vulnerability exploit\n", thread_id);
    
    int violation_caught = attempt_vulnerable_copy(&ctx);
    int integrity_maintained = verify_memory_integrity(&ctx);
    
    /* Determine test result */
    if (violation_caught && integrity_maintained) {
        printf("[Thread %ld] SUCCESS: Memory safety violation was prevented!\n", 
               thread_id);
        ctx.test_result = 1;
    } else {
        printf("[Thread %ld] FAILURE: Memory safety violation was not fully prevented!\n", 
               thread_id);
        ctx.test_result = 0;
    }
    
cleanup:
    /* Phase 7: Cleanup */
    printf("[Thread %ld] Phase 7: Cleaning up resources\n", thread_id);
    free(ctx.layout.source_buffer);
    free(ctx.layout.target_buffer);
    free(ctx.layout.protected_buffer);
    free(ctx.layout.canary);
    
    printf("[Thread %ld] Test completed with result: %s\n", 
           thread_id, ctx.test_result ? "PASS" : "FAIL");
    printf("[Thread %ld] ======================================\n\n", thread_id);
    
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
    printf("xBGAS Memory Safety Test: Out-of-Bounds Write (Spatial Safety)\n");
    printf("=================================================================\n");
    printf("Platform: CHERI-Morello\n");
    printf("Runtime: xBGAS\n");
    printf("Test Type: Spatial Memory Safety Violation (Write)\n");
    printf("Description: Attempting to write beyond allocated buffer bounds\n");
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
    
    printf("Starting %d concurrent out-of-bounds write tests...\n", num_pes);
    
    /* Launch test threads */
    for (int i = 0; i < num_pes; i++) {
        if (pthread_create(&threads[i], NULL, out_of_bounds_write_test, 
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
