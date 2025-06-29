/**
 * @file ttu_s1_free_not_at_start_refactored.c
 * @brief Refactored Free Not at Start Spatial Safety Test
 * @author Mert Side (Texas Tech University)
 * @date 2024
 * 
 * @section DESCRIPTION
 * This program demonstrates a spatial memory safety violation where free() is
 * called on a pointer that does not point to the start of an allocated block.
 * This vulnerability occurs when:
 * 1. Memory is allocated using malloc/calloc
 * 2. A pointer is advanced (offset) into the allocated region
 * 3. free() is called on the offset pointer instead of the original pointer
 * 4. This causes undefined behavior and potential heap corruption
 * 
 * @section VULNERABILITY_DETAILS
 * Free Not at Start violations:
 * - Cause heap metadata corruption
 * - Lead to double-free vulnerabilities
 * - Can result in arbitrary code execution
 * - Are common in string manipulation and array processing
 * - Often occur with pointer arithmetic mistakes
 * 
 * @section CHERI_ANALYSIS
 * On CHERI-Morello systems, this test evaluates:
 * - Heap allocation capability bounds enforcement
 * - Detection of invalid free() operations
 * - Protection against heap metadata corruption
 * - Capability validation on heap deallocation
 * 
 * @section EXPECTED_BEHAVIOR
 * - Traditional systems: May corrupt heap metadata (undefined behavior)
 * - CHERI-Morello: Should trap on invalid capability in free(), preventing corruption
 * 
 * @section TEST_PHASES
 * 1. SETUP: Initialize test environment
 * 2. ALLOCATE_BUFFER: Create memory buffer for testing
 * 3. POPULATE_DATA: Fill buffer with test data
 * 4. CREATE_OFFSET: Generate offset pointer into allocated memory
 * 5. ATTEMPT_FREE: Try to free offset pointer (vulnerability)
 * 6. VERIFY_CORRUPTION: Check for heap corruption
 * 7. ACCESS_TEST: Attempt to access memory after invalid free
 * 8. ANALYZE_PROTECTION: Evaluate CHERI protection effectiveness
 * 9. CLEANUP: Properly clean up remaining memory
 * 10. REPORT: Generate comprehensive test report
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <setjmp.h>
#include <unistd.h>
#include <pthread.h>
#include <stdint.h>

// Include legacy header for compatibility
#include "xbrtime_morello.h"

//=============================================================================
// TEST CONFIGURATION AND CONSTANTS
//=============================================================================

/** @brief Test identification */
#define TEST_NAME "Free Not at Start"
#define TEST_ID "TTU_S1"
#define TEST_CATEGORY "SPATIAL_MEMORY_SAFETY"

/** @brief Buffer and offset constants */
#define BUFFER_SIZE 128
#define INVALID_OFFSET 16
#define TEST_DATA "Hello World! This is a test string for spatial safety validation. Lorem ipsum dolor sit amet consectetur."

/** @brief Test execution parameters */
#define NUM_THREADS 4
#define MAX_ACCESS_ATTEMPTS 10

/** @brief Test phases for structured execution */
typedef enum {
    PHASE_SETUP = 0,
    PHASE_ALLOCATE_BUFFER,
    PHASE_POPULATE_DATA,
    PHASE_CREATE_OFFSET,
    PHASE_ATTEMPT_FREE,
    PHASE_VERIFY_CORRUPTION,
    PHASE_ACCESS_TEST,
    PHASE_ANALYZE_PROTECTION,
    PHASE_CLEANUP,
    PHASE_REPORT,
    PHASE_MAX
} test_phase_t;

//=============================================================================
// DATA STRUCTURES
//=============================================================================

/** @brief Thread test context */
typedef struct {
    pthread_t thread_id;
    int thread_index;
    char* original_buffer;
    char* offset_pointer;
    size_t buffer_size;
    size_t offset_amount;
    int free_attempted;
    int free_successful;
    int heap_corrupted;
    int access_after_free_successful;
} free_context_t;

//=============================================================================
// GLOBAL STATE AND SIGNAL HANDLING
//=============================================================================

/** @brief Global test state */
static struct {
    jmp_buf recovery_point;
    volatile sig_atomic_t signal_caught;
    volatile sig_atomic_t current_phase;
    free_context_t contexts[NUM_THREADS];
    int total_free_attempts;
    int successful_frees;
    int heap_corruptions_detected;
} test_state = {0};

/** @brief Signal handler for memory safety violations */
static void signal_handler(int sig) {
    test_state.signal_caught = sig;
    
    const char* signal_name = (sig == SIGSEGV) ? "SIGSEGV" :
                              (sig == SIGBUS) ? "SIGBUS" :
                              (sig == SIGABRT) ? "SIGABRT" : "UNKNOWN";
    
    printf("🛡️  CHERI Protection: Caught %s during phase %d\n", 
           signal_name, (int)test_state.current_phase);
    
    // Jump back to recovery point
    longjmp(test_state.recovery_point, sig);
}

/** @brief Setup signal handlers */
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
// MEMORY ANALYSIS UTILITIES
//=============================================================================

/** @brief Analyze pointer properties (CHERI-specific) */
static void analyze_pointer(const char* name, void* ptr, int thread_idx) {
    if (!ptr) {
        printf("[Thread %d] 🔍 %s: NULL pointer\n", thread_idx, name);
        return;
    }
    
    printf("[Thread %d] 🔍 %s: %p", thread_idx, name, ptr);
    
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

/** @brief Display memory layout and pointer relationships */
static void display_memory_layout(free_context_t* ctx) {
    printf("[Thread %d] 🏗️  Memory Layout Analysis:\n", ctx->thread_index);
    
    analyze_pointer("Original buffer", ctx->original_buffer, ctx->thread_index);
    analyze_pointer("Offset pointer", ctx->offset_pointer, ctx->thread_index);
    
    if (ctx->original_buffer && ctx->offset_pointer) {
        ptrdiff_t actual_offset = ctx->offset_pointer - ctx->original_buffer;
        printf("[Thread %d] 📏 Pointer offset: %td bytes\n", ctx->thread_index, actual_offset);
        printf("[Thread %d] 📏 Expected offset: %zu bytes\n", ctx->thread_index, ctx->offset_amount);
        
        if (actual_offset == (ptrdiff_t)ctx->offset_amount) {
            printf("[Thread %d] ✅ Offset calculation correct\n", ctx->thread_index);
        } else {
            printf("[Thread %d] ⚠️  Offset calculation mismatch\n", ctx->thread_index);
        }
    }
}

/** @brief Display buffer contents safely */
static void display_buffer_contents(const char* label, char* buffer, size_t max_len, int thread_idx) {
    if (!buffer) {
        printf("[Thread %d] 📊 %s: NULL buffer\n", thread_idx, label);
        return;
    }
    
    printf("[Thread %d] 📊 %s content (first 32 bytes): \"", thread_idx, label);
    
    for (size_t i = 0; i < max_len && i < 32; i++) {
        char c = buffer[i];
        if (c >= 32 && c <= 126) {  // Printable ASCII
            printf("%c", c);
        } else if (c == '\0') {
            printf("\\0");
            break;
        } else {
            printf("\\x%02x", (unsigned char)c);
        }
    }
    printf("\"\n");
}

//=============================================================================
// VULNERABILITY SIMULATION
//=============================================================================

/** @brief Attempt invalid free operation on offset pointer */
static int attempt_invalid_free(free_context_t* ctx) {
    printf("[Thread %d] 🚨 CRITICAL: Attempting free() on offset pointer\n", ctx->thread_index);
    
    if (!ctx->offset_pointer) {
        printf("[Thread %d] ❌ Offset pointer not available\n", ctx->thread_index);
        return 0;
    }
    
    printf("[Thread %d] 💥 Calling free() on %p (offset: +%zu bytes)\n", 
           ctx->thread_index, (void*)ctx->offset_pointer, ctx->offset_amount);
    
    // This is the vulnerability - calling free() on a non-start pointer
    // Should trigger CHERI protection or heap corruption on traditional systems
    free(ctx->offset_pointer);
    
    printf("[Thread %d] 🚨 VULNERABILITY: Invalid free() completed without immediate crash\n", 
           ctx->thread_index);
    
    ctx->free_attempted = 1;
    ctx->free_successful = 1;
    
    return 1;
}

/** @brief Test memory access after invalid free */
static void test_access_after_invalid_free(free_context_t* ctx) {
    printf("[Thread %d] 📖 Testing memory access after invalid free\n", ctx->thread_index);
    
    if (!ctx->original_buffer) {
        printf("[Thread %d] ❌ Original buffer not available for access test\n", ctx->thread_index);
        return;
    }
    
    printf("[Thread %d] 🔍 Attempting to read from original buffer...\n", ctx->thread_index);
    
    // Try to access the original buffer
    display_buffer_contents("Post-free buffer", ctx->original_buffer, BUFFER_SIZE, ctx->thread_index);
    
    // Try to write to the original buffer
    printf("[Thread %d] ✏️  Attempting to write to original buffer...\n", ctx->thread_index);
    
    strncpy(ctx->original_buffer, "MODIFIED_AFTER_FREE", BUFFER_SIZE - 1);
    ctx->original_buffer[BUFFER_SIZE - 1] = '\0';
    
    printf("[Thread %d] 📊 Buffer modification after invalid free succeeded\n", ctx->thread_index);
    display_buffer_contents("Modified buffer", ctx->original_buffer, BUFFER_SIZE, ctx->thread_index);
    
    ctx->access_after_free_successful = 1;
}

//=============================================================================
// HEAP CORRUPTION DETECTION
//=============================================================================

/** @brief Detect potential heap corruption */
static int detect_heap_corruption(free_context_t* ctx) {
    printf("[Thread %d] 🔍 Checking for heap corruption indicators\n", ctx->thread_index);
    
    // Try to allocate new memory to see if heap is still functional
    void* test_alloc = malloc(64);
    if (!test_alloc) {
        printf("[Thread %d] ⚠️  Heap corruption detected: malloc failed\n", ctx->thread_index);
        return 1;
    }
    
    // Try to write to new allocation
    memset(test_alloc, 0xAA, 64);
    
    // Try to free the new allocation
    free(test_alloc);
    
    printf("[Thread %d] ✅ Heap appears functional after invalid free\n", ctx->thread_index);
    return 0;
}

//=============================================================================
// CORE TEST LOGIC
//=============================================================================

/** @brief Execute free not at start test for a single thread */
static void* execute_free_not_at_start_test(void* arg) {
    free_context_t* ctx = (free_context_t*)arg;
    int recovery_signal = 0;
    
    printf("\n[Thread %d] 🚀 Starting %s test\n", ctx->thread_index, TEST_NAME);
    
    // Setup signal handling
    setup_signal_handlers();
    
    // Set recovery point for signal handling
    if ((recovery_signal = setjmp(test_state.recovery_point)) != 0) {
        printf("[Thread %d] 🔄 Recovered from signal %d in phase %d\n", 
               ctx->thread_index, recovery_signal, (int)test_state.current_phase);
        goto cleanup_and_exit;
    }
    
    // PHASE 1: SETUP
    test_state.current_phase = PHASE_SETUP;
    printf("[Thread %d] 📋 Phase %d: Setup\n", ctx->thread_index, PHASE_SETUP);
    ctx->buffer_size = BUFFER_SIZE;
    ctx->offset_amount = INVALID_OFFSET;
    ctx->free_attempted = 0;
    ctx->free_successful = 0;
    ctx->heap_corrupted = 0;
    ctx->access_after_free_successful = 0;
    
    // PHASE 2: ALLOCATE_BUFFER
    test_state.current_phase = PHASE_ALLOCATE_BUFFER;
    printf("[Thread %d] 📋 Phase %d: Allocate buffer\n", ctx->thread_index, PHASE_ALLOCATE_BUFFER);
    
    ctx->original_buffer = malloc(ctx->buffer_size);
    if (!ctx->original_buffer) {
        printf("[Thread %d] ❌ Failed to allocate buffer\n", ctx->thread_index);
        return NULL;
    }
    
    printf("[Thread %d] ✅ Allocated %zu bytes\n", ctx->thread_index, ctx->buffer_size);
    
    // PHASE 3: POPULATE_DATA
    test_state.current_phase = PHASE_POPULATE_DATA;
    printf("[Thread %d] 📋 Phase %d: Populate buffer with test data\n", 
           ctx->thread_index, PHASE_POPULATE_DATA);
    
    memset(ctx->original_buffer, 0, ctx->buffer_size);
    strncpy(ctx->original_buffer, TEST_DATA, ctx->buffer_size - 1);
    ctx->original_buffer[ctx->buffer_size - 1] = '\0';
    
    display_buffer_contents("Original buffer", ctx->original_buffer, ctx->buffer_size, ctx->thread_index);
    
    // PHASE 4: CREATE_OFFSET
    test_state.current_phase = PHASE_CREATE_OFFSET;
    printf("[Thread %d] 📋 Phase %d: Create offset pointer\n", ctx->thread_index, PHASE_CREATE_OFFSET);
    
    ctx->offset_pointer = ctx->original_buffer + ctx->offset_amount;
    
    display_memory_layout(ctx);
    display_buffer_contents("Offset view", ctx->offset_pointer, 
                           ctx->buffer_size - ctx->offset_amount, ctx->thread_index);
    
    // PHASE 5: ATTEMPT_FREE
    test_state.current_phase = PHASE_ATTEMPT_FREE;
    printf("[Thread %d] 📋 Phase %d: Attempt invalid free\n", ctx->thread_index, PHASE_ATTEMPT_FREE);
    printf("[Thread %d] 🚨 CRITICAL: Executing spatial safety violation\n", ctx->thread_index);
    
    ctx->free_successful = attempt_invalid_free(ctx);
    
    // PHASE 6: VERIFY_CORRUPTION
    test_state.current_phase = PHASE_VERIFY_CORRUPTION;
    printf("[Thread %d] 📋 Phase %d: Verify heap corruption\n", ctx->thread_index, PHASE_VERIFY_CORRUPTION);
    
    ctx->heap_corrupted = detect_heap_corruption(ctx);
    if (ctx->heap_corrupted) {
        test_state.heap_corruptions_detected++;
    }
    
    // PHASE 7: ACCESS_TEST
    test_state.current_phase = PHASE_ACCESS_TEST;
    printf("[Thread %d] 📋 Phase %d: Test memory access after free\n", 
           ctx->thread_index, PHASE_ACCESS_TEST);
    
    test_access_after_invalid_free(ctx);
    
    test_state.total_free_attempts++;
    if (ctx->free_successful) {
        test_state.successful_frees++;
    }
    
cleanup_and_exit:
    // PHASE 8: CLEANUP
    test_state.current_phase = PHASE_CLEANUP;
    printf("[Thread %d] 📋 Phase %d: Cleanup\n", ctx->thread_index, PHASE_CLEANUP);
    
    // Note: We can't safely free the original buffer if we called free() on the offset
    // This demonstrates the heap corruption issue
    if (!ctx->free_successful && ctx->original_buffer) {
        printf("[Thread %d] 🗑️  Safely freeing original buffer\n", ctx->thread_index);
        free(ctx->original_buffer);
        ctx->original_buffer = NULL;
    } else if (ctx->free_successful) {
        printf("[Thread %d] ⚠️  Cannot safely free original buffer (heap may be corrupted)\n", 
               ctx->thread_index);
    }
    
    // PHASE 9: REPORT
    test_state.current_phase = PHASE_REPORT;
    printf("[Thread %d] 📋 Phase %d: Generate report\n", ctx->thread_index, PHASE_REPORT);
    
    if (recovery_signal != 0) {
        printf("[Thread %d] ✅ Test completed with CHERI protection (signal %d)\n", 
               ctx->thread_index, recovery_signal);
        printf("[Thread %d] 🔒 Invalid free() prevented by capability validation\n", 
               ctx->thread_index);
    } else {
        printf("[Thread %d] ❌ Test completed without protection\n", ctx->thread_index);
        if (ctx->free_successful) {
            printf("[Thread %d] 💥 Spatial safety violation succeeded\n", ctx->thread_index);
            if (ctx->heap_corrupted) {
                printf("[Thread %d] 🚨 Heap corruption detected\n", ctx->thread_index);
            }
        }
    }
    
    printf("[Thread %d] 🏁 %s test finished\n\n", ctx->thread_index, TEST_NAME);
    return NULL;
}

//=============================================================================
// MAIN FUNCTION AND TEST ORCHESTRATION
//=============================================================================

/**
 * @brief Main function - orchestrates the multi-threaded free not at start test
 * @return 0 on success, non-zero on failure
 */
int main(void) {
    printf("=================================================================\n");
    printf("🔬 xBGAS Security Test: %s\n", TEST_NAME);
    printf("📊 Test ID: %s | Category: %s\n", TEST_ID, TEST_CATEGORY);
    printf("🎯 Platform: CHERI-Morello | Violation: Invalid free() operation\n");
    printf("=================================================================\n\n");
    
    printf("📖 Free Not at Start Vulnerability Background:\n");
    printf("   - Common mistake in pointer arithmetic and string manipulation\n");
    printf("   - Causes heap metadata corruption and undefined behavior\n");
    printf("   - Can lead to double-free vulnerabilities and crashes\n");
    printf("   - Often exploited for arbitrary code execution\n\n");
    
    // Initialize test state
    memset(&test_state, 0, sizeof(test_state));
    
    printf("🧵 Starting multi-threaded spatial safety test...\n");
    printf("📊 Number of threads: %d\n\n", NUM_THREADS);
    
    // Create and start threads
    for (int i = 0; i < NUM_THREADS; i++) {
        test_state.contexts[i].thread_index = i;
        
        if (pthread_create(&test_state.contexts[i].thread_id, NULL, 
                          execute_free_not_at_start_test, &test_state.contexts[i]) != 0) {
            printf("❌ Failed to create thread %d\n", i);
            return 1;
        }
    }
    
    // Wait for all threads to complete
    printf("⏳ Waiting for all threads to complete...\n");
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(test_state.contexts[i].thread_id, NULL);
    }
    
    // Generate final report
    printf("=================================================================\n");
    printf("📈 Free Not at Start Test Summary Report\n");
    printf("=================================================================\n");
    printf("🎯 Total free attempts: %d\n", test_state.total_free_attempts);
    printf("💥 Successful invalid frees: %d\n", test_state.successful_frees);
    printf("📊 Vulnerability success rate: %.1f%%\n", 
           test_state.total_free_attempts > 0 ? 
           (100.0 * test_state.successful_frees / test_state.total_free_attempts) : 0.0);
    printf("🚨 Heap corruptions detected: %d\n", test_state.heap_corruptions_detected);
    
    if (test_state.successful_frees > 0) {
        printf("🚨 VULNERABILITY STATUS: EXPLOITABLE\n");
        printf("💀 Invalid free() operations succeeded\n");
        printf("⚠️  System vulnerable to heap corruption and crashes\n");
    } else {
        printf("✅ VULNERABILITY STATUS: MITIGATED\n");
        printf("🔒 Invalid free() operations prevented\n");
        printf("🛡️  CHERI capability system provided protection\n");
    }
    
    printf("=================================================================\n");
    printf("🔒 CHERI-Morello spatial memory safety evaluation complete\n");
    printf("=================================================================\n");
    
    return 0;
}

/**
 * @brief Test Summary
 * 
 * This refactored test provides comprehensive evaluation of:
 * 
 * 1. **Spatial Memory Safety**: Protection against invalid free() operations
 * 2. **Heap Integrity**: Prevention of heap metadata corruption
 * 3. **CHERI Capability System**: Validation of heap allocation capabilities
 * 4. **Pointer Arithmetic Safety**: Detection of offset pointer abuse
 * 5. **Multi-threaded Safety**: Concurrent spatial safety violations
 * 6. **Heap Corruption Detection**: Automated identification of heap damage
 * 7. **Memory Layout Analysis**: Understanding pointer relationships
 * 8. **Educational Value**: Demonstration of common programming mistakes
 * 
 * Expected behavior on CHERI-Morello:
 * - free() should validate capability bounds and permissions
 * - Attempt to free offset pointer should trap
 * - Heap integrity should be preserved
 * - Spatial safety violation should be prevented
 * 
 * On traditional systems:
 * - Invalid free() may corrupt heap metadata
 * - Subsequent allocations may fail unpredictably
 * - System may crash or become unstable
 * - Vulnerability would be exploitable for attacks
 */
