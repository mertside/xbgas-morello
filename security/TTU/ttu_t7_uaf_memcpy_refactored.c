/**
 * @file ttu_t7_uaf_memcpy_refactored.c
 * @brief Refactored Use-After-Free with memcpy Security Test
 * @author Mert Side (Texas Tech University)
 * @date 2024
 * 
 * @section DESCRIPTION
 * This program demonstrates a use-after-free (UAF) vulnerability involving memcpy operations.
 * The vulnerability occurs when:
 * 1. Memory is allocated and data is written to it
 * 2. The memory is freed
 * 3. New memory is allocated (potentially reusing the same address)
 * 4. memcpy is used with the original (freed) pointer to overwrite the new allocation
 * 
 * @section CHERI_ANALYSIS
 * On CHERI-Morello systems, this test evaluates:
 * - Temporal memory safety for data pointers
 * - Capability revocation upon free()
 * - Protection against data corruption via dangling pointers
 * - Detection of use-after-free in memory copy operations
 * 
 * @section EXPECTED_BEHAVIOR
 * - Traditional systems: May corrupt new allocation (data integrity violation)
 * - CHERI-Morello: Should trap on capability use-after-free, preventing corruption
 * 
 * @section TEST_PHASES
 * 1. SETUP: Initialize xBGAS runtime and thread environment
 * 2. ALLOCATE_FIRST: Create initial memory allocation
 * 3. WRITE_INITIAL: Write initial data using memcpy
 * 4. READ_INITIAL: Verify initial data content
 * 5. FREE_FIRST: Release initial allocation
 * 6. ALLOCATE_SECOND: Allocate new memory (may reuse address)
 * 7. WRITE_SECOND: Write data to new allocation
 * 8. UAF_MEMCPY: Use freed pointer with memcpy (vulnerability)
 * 9. VERIFY_CORRUPTION: Check for data corruption
 * 10. CLEANUP: Free remaining allocations
 * 11. TEARDOWN: Close xBGAS runtime
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <setjmp.h>
#include <unistd.h>
#include <sys/mman.h>

// Include xBGAS runtime headers
#include "xbrtime_morello.h"

//=============================================================================
// TEST CONFIGURATION AND CONSTANTS
//=============================================================================

/** @brief Test identification */
#define TEST_NAME "Use-After-Free memcpy"
#define TEST_ID "TTU_T7"
#define TEST_CATEGORY "TEMPORAL_MEMORY_SAFETY"

/** @brief Memory allocation constants */
#define BUFFER_SIZE 32
#define INITIAL_DATA "HelloWorld!"
#define SECONDARY_DATA "SecondaryData"
#define MALICIOUS_DATA "MALICIOUS_PAYLOAD"

/** @brief Test phases for structured execution */
typedef enum {
    PHASE_SETUP = 0,
    PHASE_ALLOCATE_FIRST,
    PHASE_WRITE_INITIAL,
    PHASE_READ_INITIAL,
    PHASE_FREE_FIRST,
    PHASE_ALLOCATE_SECOND,
    PHASE_WRITE_SECOND,
    PHASE_UAF_MEMCPY,
    PHASE_VERIFY_CORRUPTION,
    PHASE_CLEANUP,
    PHASE_TEARDOWN,
    PHASE_MAX
} test_phase_t;

//=============================================================================
// GLOBAL STATE AND SIGNAL HANDLING
//=============================================================================

/** @brief Global test state for signal handling */
static struct {
    jmp_buf recovery_point;
    volatile sig_atomic_t signal_caught;
    volatile sig_atomic_t current_phase;
    volatile sig_atomic_t thread_id;
    char* first_buffer;
    char* second_buffer;
    void* allocated_memory[2];
    size_t allocation_count;
    int corruption_detected;
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

/** @brief Display buffer contents in a safe manner */
static void display_buffer_contents(const char* name, char* buffer, size_t max_len) {
    if (!buffer) {
        printf("[Thread %ld] 📊 %s: NULL buffer\n", 
               (long)test_state.thread_id, name);
        return;
    }
    
    printf("[Thread %ld] 📊 %s contents: ", 
           (long)test_state.thread_id, name);
    
    // Safely display buffer contents
    for (size_t i = 0; i < max_len && i < strlen(buffer); i++) {
        char c = buffer[i];
        if (c >= 32 && c <= 126) {  // Printable ASCII
            printf("%c", c);
        } else {
            printf("\\x%02x", (unsigned char)c);
        }
    }
    printf("\n");
}

/** @brief Analyze memory state before and after operations */
static void analyze_memory_state(const char* phase) {
    printf("[Thread %ld] 📊 Memory analysis (%s):\n", 
           (long)test_state.thread_id, phase);
    
    analyze_pointer("First buffer", (void*)test_state.first_buffer);
    analyze_pointer("Second buffer", (void*)test_state.second_buffer);
    
    if (test_state.first_buffer == test_state.second_buffer) {
        printf("[Thread %ld] ⚠️  Address reuse detected\n", 
               (long)test_state.thread_id);
    } else {
        printf("[Thread %ld] ℹ️  Different addresses used\n", 
               (long)test_state.thread_id);
    }
}

//=============================================================================
// CORRUPTION DETECTION UTILITIES
//=============================================================================

/** @brief Check for data corruption in second buffer */
static int check_data_corruption(void) {
    if (!test_state.second_buffer) {
        return 0;
    }
    
    // Check if second buffer contains malicious data
    if (strncmp(test_state.second_buffer, MALICIOUS_DATA, strlen(MALICIOUS_DATA)) == 0) {
        printf("[Thread %ld] 🚨 DATA CORRUPTION DETECTED!\n", 
               (long)test_state.thread_id);
        printf("[Thread %ld] 💥 Second buffer corrupted with malicious payload\n", 
               (long)test_state.thread_id);
        return 1;
    }
    
    // Check if second buffer was modified unexpectedly
    if (strncmp(test_state.second_buffer, SECONDARY_DATA, strlen(SECONDARY_DATA)) != 0) {
        printf("[Thread %ld] ⚠️  Second buffer modified unexpectedly\n", 
               (long)test_state.thread_id);
        display_buffer_contents("Modified second buffer", test_state.second_buffer, BUFFER_SIZE);
        return 1;
    }
    
    return 0;
}

//=============================================================================
// CORE TEST LOGIC
//=============================================================================

/** @brief Execute the use-after-free memcpy vulnerability test */
static void execute_uaf_memcpy_test(void* arg) {
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
    test_state.corruption_detected = 0;
    
    // PHASE 2: ALLOCATE_FIRST
    test_state.current_phase = PHASE_ALLOCATE_FIRST;
    printf("[Thread %ld] 📋 Phase %d: Allocate first buffer\n", tid, PHASE_ALLOCATE_FIRST);
    
    test_state.first_buffer = (char*)malloc(BUFFER_SIZE);        if (!test_state.first_buffer) {
            printf("[Thread %ld] ❌ Failed to allocate first buffer\n", tid);
            return;
        }
    test_state.allocated_memory[test_state.allocation_count++] = test_state.first_buffer;
    
    analyze_pointer("First buffer allocation", (void*)test_state.first_buffer);
    
    // PHASE 3: WRITE_INITIAL
    test_state.current_phase = PHASE_WRITE_INITIAL;
    printf("[Thread %ld] 📋 Phase %d: Write initial data\n", tid, PHASE_WRITE_INITIAL);
    
    memset(test_state.first_buffer, 0, BUFFER_SIZE);
    memcpy(test_state.first_buffer, INITIAL_DATA, strlen(INITIAL_DATA));
    
    display_buffer_contents("First buffer after initial write", test_state.first_buffer, BUFFER_SIZE);
    
    // PHASE 4: READ_INITIAL
    test_state.current_phase = PHASE_READ_INITIAL;
    printf("[Thread %ld] 📋 Phase %d: Read and verify initial data\n", tid, PHASE_READ_INITIAL);
    
    if (strncmp(test_state.first_buffer, INITIAL_DATA, strlen(INITIAL_DATA)) == 0) {
        printf("[Thread %ld] ✅ Initial data verified successfully\n", tid);
    } else {
        printf("[Thread %ld] ❌ Initial data verification failed\n", tid);
    }
    
    // PHASE 5: FREE_FIRST
    test_state.current_phase = PHASE_FREE_FIRST;
    printf("[Thread %ld] 📋 Phase %d: Free first buffer\n", tid, PHASE_FREE_FIRST);
    
    printf("[Thread %ld] 🗑️  Freeing first buffer at %p\n", tid, (void*)test_state.first_buffer);
    free(test_state.first_buffer);
    
    // PHASE 6: ALLOCATE_SECOND
    test_state.current_phase = PHASE_ALLOCATE_SECOND;
    printf("[Thread %ld] 📋 Phase %d: Allocate second buffer\n", tid, PHASE_ALLOCATE_SECOND);
    
    // Allocate new memory that might reuse the same address
    test_state.second_buffer = (char*)malloc(BUFFER_SIZE);        if (!test_state.second_buffer) {
            printf("[Thread %ld] ❌ Failed to allocate second buffer\n", tid);
            return;
        }
    test_state.allocated_memory[test_state.allocation_count++] = test_state.second_buffer;
    
    analyze_memory_state("after second allocation");
    
    // PHASE 7: WRITE_SECOND
    test_state.current_phase = PHASE_WRITE_SECOND;
    printf("[Thread %ld] 📋 Phase %d: Write data to second buffer\n", tid, PHASE_WRITE_SECOND);
    
    memset(test_state.second_buffer, 0, BUFFER_SIZE);
    memcpy(test_state.second_buffer, SECONDARY_DATA, strlen(SECONDARY_DATA));
    
    display_buffer_contents("Second buffer after write", test_state.second_buffer, BUFFER_SIZE);
    
    // PHASE 8: UAF_MEMCPY (Critical vulnerability test)
    test_state.current_phase = PHASE_UAF_MEMCPY;
    printf("[Thread %ld] 📋 Phase %d: Attempt use-after-free memcpy\n", tid, PHASE_UAF_MEMCPY);
    printf("[Thread %ld] 🚨 CRITICAL: Attempting memcpy with freed pointer\n", tid);
    
    analyze_memory_state("before UAF memcpy");
    
    // This is the vulnerability - using freed pointer with memcpy
    printf("[Thread %ld] 💥 Calling memcpy with freed pointer...\n", tid);
    memcpy(test_state.first_buffer, MALICIOUS_DATA, strlen(MALICIOUS_DATA));
    
    // If we reach here without a signal, the vulnerability succeeded
    printf("[Thread %ld] 🚨 VULNERABILITY SUCCESS: memcpy UAF not detected!\n", tid);
    
    // PHASE 9: VERIFY_CORRUPTION
    test_state.current_phase = PHASE_VERIFY_CORRUPTION;
    printf("[Thread %ld] 📋 Phase %d: Verify data corruption\n", tid, PHASE_VERIFY_CORRUPTION);
    
    display_buffer_contents("First buffer after UAF", test_state.first_buffer, BUFFER_SIZE);
    display_buffer_contents("Second buffer after UAF", test_state.second_buffer, BUFFER_SIZE);
    
    test_state.corruption_detected = check_data_corruption();
    
    if (test_state.corruption_detected) {
        printf("[Thread %ld] 💥 DATA INTEGRITY VIOLATION: Corruption successful\n", tid);
    } else {
        printf("[Thread %ld] ✅ No data corruption detected\n", tid);
    }
    
cleanup_and_exit:
    // PHASE 10: CLEANUP
    test_state.current_phase = PHASE_CLEANUP;
    printf("[Thread %ld] 📋 Phase %d: Cleanup\n", tid, PHASE_CLEANUP);
    
    // Free any remaining valid allocations
    if (test_state.allocation_count > 1 && test_state.second_buffer) {
        printf("[Thread %ld] 🗑️  Freeing second buffer\n", tid);
        free(test_state.second_buffer);
    }
    
    // PHASE 11: TEARDOWN
    test_state.current_phase = PHASE_TEARDOWN;
    printf("[Thread %ld] 📋 Phase %d: Teardown\n", tid, PHASE_TEARDOWN);
    
    if (recovery_signal != 0) {
        printf("[Thread %ld] ✅ Test completed with CHERI protection (signal %d)\n", tid, recovery_signal);
        printf("[Thread %ld] 🔒 Temporal memory safety violation prevented\n", tid);
        printf("[Thread %ld] 🛡️  Data integrity preserved\n", tid);
    } else {
        printf("[Thread %ld] ❌ Test completed without protection\n", tid);
        if (test_state.corruption_detected) {
            printf("[Thread %ld] 💥 Data corruption vulnerability exploitable\n", tid);
        } else {
            printf("[Thread %ld] ℹ️  No corruption detected (may be due to address layout)\n", tid);
        }
    }
    
    printf("[Thread %ld] 🏁 %s test finished\n\n", tid, TEST_NAME);
}
}

//=============================================================================
// MAIN FUNCTION AND TEST ORCHESTRATION
//=============================================================================

/**
 * @brief Main function - orchestrates the multi-threaded UAF memcpy test
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
    printf("🧵 Starting multi-threaded memcpy UAF test...\n\n");
    
    // Execute test on all available processing elements
    for (long i = 0; i < num_pes; i++) {
        tpool_add_work(threads[i].thread_queue, execute_uaf_memcpy_test, (void*)(uintptr_t)i);
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
 * 1. **Temporal Memory Safety**: Tests pointer use-after-free with memcpy
 * 2. **Data Integrity Protection**: Prevents memory corruption attacks
 * 3. **CHERI Capability System**: Validates capability revocation
 * 4. **Signal Handling**: Graceful recovery from memory safety violations
 * 5. **Multi-threading**: Concurrent vulnerability testing
 * 6. **Corruption Detection**: Automated verification of data integrity
 * 7. **Phase-based Execution**: Structured test progression
 * 8. **Comprehensive Logging**: Detailed execution tracing
 * 
 * Expected behavior on CHERI-Morello:
 * - Capability should be revoked upon free()
 * - Attempt to use freed pointer with memcpy should trap
 * - Signal handler should catch the violation
 * - Data integrity should be preserved
 * 
 * On traditional systems:
 * - memcpy with freed pointer may succeed
 * - Data corruption may occur
 * - Memory safety violation would be exploitable
 */
