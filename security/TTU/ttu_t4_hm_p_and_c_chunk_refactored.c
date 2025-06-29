/**
 * @file ttu_t4_hm_p_and_c_chunk_refactored.c
 * @brief Heap Manipulation via Parent and Child Chunk Vulnerability Test
 * 
 * REFACTORED FOR xBGAS-Morello TTU Security Evaluation
 * =====================================================
 * 
 * VULNERABILITY TYPE: Heap Manipulation (Parent/Child Chunk Overlap)
 * SECURITY IMPACT: Critical - Memory corruption, arbitrary read/write
 * CHERI MITIGATION: Capability spatial safety, bounds checking
 * 
 * DESCRIPTION:
 * This test demonstrates a sophisticated heap manipulation attack where
 * an attacker exploits heap metadata to create overlapping memory chunks.
 * The attack involves:
 * 1. Allocating multiple adjacent chunks on the heap
 * 2. Using out-of-bounds write to modify heap metadata (chunk size)
 * 3. Creating a "parent" chunk that encompasses a "child" chunk
 * 4. Exploiting the overlap to read/write data in the child chunk
 * 5. Demonstrating arbitrary memory access within heap regions
 * 
 * ATTACK TECHNIQUE:
 * - Heap metadata manipulation through buffer overflow
 * - Creation of overlapping memory regions
 * - Exploitation of heap allocator assumptions
 * - Arbitrary read/write primitive within heap
 * 
 * CHERI-MORELLO ANALYSIS:
 * - Capability bounds prevent out-of-bounds metadata modification
 * - Spatial safety ensures no overlap between distinct allocations
 * - Heap metadata protection prevents size field manipulation
 * - Bounds checking validates all memory accesses
 * 
 * EXPECTED BEHAVIOR:
 * - Traditional systems: Heap corruption, overlapping allocations
 * - CHERI-Morello: Capability fault on bounds violation
 * 
 * @author TTU Security Research Team
 * @date 2024
 * @version 2.0 (Refactored)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <signal.h>
#include <setjmp.h>
#include "xbrtime_morello.h"

// Test configuration constants
#define SMALL_CHUNK_SIZE 0x10
#define MEDIUM_CHUNK_SIZE 0x20
#define LARGE_CHUNK_SIZE 0x50
#define METADATA_OFFSET 0x18      // Typical offset to next chunk's size field
#define MANIPULATED_SIZE 0x61     // Size that would create overlap
#define VICTIM_DATA "victim's data"
#define ATTACK_PATTERN 'A'
#define PATTERN_SIZE 0xf

// Test result tracking
typedef enum {
    TEST_RESULT_UNKNOWN = 0,
    TEST_RESULT_HEAP_CORRUPTION,        // Heap corruption succeeded
    TEST_RESULT_OVERLAPPING_CHUNKS,     // Overlapping chunks detected
    TEST_RESULT_DATA_CORRUPTION,        // Victim data was corrupted
    TEST_RESULT_CHERI_PROTECTED,        // CHERI prevented the attack
    TEST_RESULT_EXCEPTION,              // Exception occurred
    TEST_RESULT_MALLOC_FAILED           // Memory allocation failed
} test_result_t;

// Chunk information structure
typedef struct {
    void* ptr;
    size_t size;
    const char* name;
    const char* purpose;
} chunk_info_t;

// Statistics tracking
typedef struct {
    int total_tests;
    int heap_corruptions;
    int overlapping_chunks;
    int data_corruptions;
    int cheri_protections;
    int exceptions;
    int malloc_failures;
} test_stats_t;

static test_stats_t global_stats = {0};
static volatile sig_atomic_t signal_caught = 0;
static sigjmp_buf signal_env;

/**
 * @brief Signal handler for capability violations and segmentation faults
 */
static void signal_handler(int sig) {
    signal_caught = sig;
    siglongjmp(signal_env, sig);
}

/**
 * @brief Setup signal handlers for capability violations
 */
static void setup_signal_handlers(void) {
    struct sigaction sa;
    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    
    sigaction(SIGSEGV, &sa, NULL);  // Segmentation fault
    sigaction(SIGBUS, &sa, NULL);   // Bus error (common on CHERI violations)
    sigaction(SIGABRT, &sa, NULL);  // Abort (heap corruption detection)
    
#ifdef __CHERI__
    // CHERI-specific capability violation signals
    sigaction(SIGPROT, &sa, NULL);  // Capability protection violation
#endif
}

/**
 * @brief Print detailed chunk information including capability details
 */
static void print_chunk_info(long thread_id, const chunk_info_t* chunk) {
    printf("  [Thread %ld] %s (%s):\n", thread_id, chunk->name, chunk->purpose);
    printf("    Address:           %p\n", chunk->ptr);
    printf("    Requested size:    %zu (0x%zx)\n", chunk->size, chunk->size);
    printf("    Full capability:   %#p\n", chunk->ptr);
    
#ifdef __CHERI__
    if (__builtin_cheri_tag_get(chunk->ptr)) {
        void* base = __builtin_cheri_base_get(chunk->ptr);
        size_t length = __builtin_cheri_length_get(chunk->ptr);
        void* top = (char*)base + length;
        
        printf("    Capability base:   %#p\n", base);
        printf("    Capability length: %zu (0x%zx)\n", length, length);
        printf("    Capability top:    %#p\n", top);
        printf("    Capability valid:  yes\n");
        
        // Calculate distance to next chunk
        printf("    Range:             [%p - %p]\n", chunk->ptr, 
               (char*)chunk->ptr + chunk->size);
    } else {
        printf("    Capability valid:  no (no tag)\n");
    }
#endif
}

/**
 * @brief Phase 1: Allocate initial chunks for manipulation
 */
static int phase1_allocate_chunks(long thread_id, chunk_info_t* chunks) {
    printf("  [Thread %ld] Phase 1: Allocating heap chunks for manipulation\n", thread_id);
    
    // Allocate chunk C (small)
    chunks[0].ptr = malloc(SMALL_CHUNK_SIZE);
    chunks[0].size = SMALL_CHUNK_SIZE;
    chunks[0].name = "Chunk C";
    chunks[0].purpose = "Manipulation base";
    
    // Allocate chunk D (small) 
    chunks[1].ptr = malloc(SMALL_CHUNK_SIZE);
    chunks[1].size = SMALL_CHUNK_SIZE;
    chunks[1].name = "Chunk D";
    chunks[1].purpose = "Victim chunk";
    
    // Allocate chunk E (small)
    chunks[2].ptr = malloc(SMALL_CHUNK_SIZE);
    chunks[2].size = SMALL_CHUNK_SIZE;
    chunks[2].name = "Chunk E";
    chunks[2].purpose = "Adjacent chunk";
    
    // Check if all allocations succeeded
    for (int i = 0; i < 3; i++) {
        if (!chunks[i].ptr) {
            printf("  [Thread %ld] ERROR: Failed to allocate %s\n", 
                   thread_id, chunks[i].name);
            
            // Cleanup any successful allocations
            for (int j = 0; j < i; j++) {
                if (chunks[j].ptr) {
                    free(chunks[j].ptr);
                    chunks[j].ptr = NULL;
                }
            }
            return -1;
        }
        
        print_chunk_info(thread_id, &chunks[i]);
    }
    
    return 0;
}

/**
 * @brief Phase 2: Attempt heap metadata manipulation
 */
static test_result_t phase2_manipulate_metadata(long thread_id, chunk_info_t* chunks) {
    printf("  [Thread %ld] Phase 2: Attempting heap metadata manipulation\n", thread_id);
    
    test_result_t result = TEST_RESULT_UNKNOWN;
    signal_caught = 0;
    
    char* base_chunk = (char*)chunks[0].ptr;  // Chunk C
    
    printf("  [Thread %ld] Attempting to modify chunk D's size field\n", thread_id);
    printf("  [Thread %ld] Writing to offset 0x%x from chunk C base\n", 
           thread_id, METADATA_OFFSET);
    
    // Set up signal handling for out-of-bounds write
    if (sigsetjmp(signal_env, 1) == 0) {
        // This is the vulnerability: writing beyond chunk boundaries
        // to modify the next chunk's metadata
        printf("  [Thread %ld] Overwriting chunk D size with 0x%02x\n", 
               thread_id, MANIPULATED_SIZE);
        
        *(base_chunk + METADATA_OFFSET) = MANIPULATED_SIZE;
        
        printf("  [Thread %ld] Metadata manipulation succeeded\n", thread_id);
        result = TEST_RESULT_HEAP_CORRUPTION;
        
    } else {
        printf("  [Thread %ld] PROTECTION: Signal %d caught during metadata write\n", 
               thread_id, signal_caught);
        result = TEST_RESULT_EXCEPTION;
    }
    
    return result;
}

/**
 * @brief Phase 3: Free chunks and attempt reallocation
 */
static int phase3_free_and_reallocate(long thread_id, chunk_info_t* chunks, 
                                    chunk_info_t* new_chunks) {
    printf("  [Thread %ld] Phase 3: Freeing chunks and attempting reallocation\n", thread_id);
    
    // Free chunks D and E to prepare for reallocation
    printf("  [Thread %ld] Freeing chunk D at %p\n", thread_id, chunks[1].ptr);
    free(chunks[1].ptr);
    chunks[1].ptr = NULL;
    
    printf("  [Thread %ld] Freeing chunk E at %p\n", thread_id, chunks[2].ptr);
    free(chunks[2].ptr);
    chunks[2].ptr = NULL;
    
    // Allocate new chunks to exploit the manipulated metadata
    printf("  [Thread %ld] Allocating large chunk G (size 0x%x)\n", 
           thread_id, LARGE_CHUNK_SIZE);
    new_chunks[0].ptr = malloc(LARGE_CHUNK_SIZE);
    new_chunks[0].size = LARGE_CHUNK_SIZE;
    new_chunks[0].name = "Chunk G";
    new_chunks[0].purpose = "Large parent chunk";
    
    printf("  [Thread %ld] Allocating small chunk H (size 0x%x)\n", 
           thread_id, SMALL_CHUNK_SIZE);
    new_chunks[1].ptr = malloc(SMALL_CHUNK_SIZE);
    new_chunks[1].size = SMALL_CHUNK_SIZE;
    new_chunks[1].name = "Chunk H";
    new_chunks[1].purpose = "Child chunk (should overlap)";
    
    if (!new_chunks[0].ptr || !new_chunks[1].ptr) {
        printf("  [Thread %ld] ERROR: Failed to allocate new chunks\n", thread_id);
        if (new_chunks[0].ptr) free(new_chunks[0].ptr);
        if (new_chunks[1].ptr) free(new_chunks[1].ptr);
        return -1;
    }
    
    // Print information about new allocations
    print_chunk_info(thread_id, &new_chunks[0]);
    print_chunk_info(thread_id, &new_chunks[1]);
    
    return 0;
}

/**
 * @brief Phase 4: Test for overlapping chunks and data corruption
 */
static test_result_t phase4_test_overlap_exploitation(long thread_id, 
                                                   chunk_info_t* new_chunks) {
    printf("  [Thread %ld] Phase 4: Testing for chunk overlap exploitation\n", thread_id);
    
    test_result_t result = TEST_RESULT_UNKNOWN;
    signal_caught = 0;
    
    char* large_chunk = (char*)new_chunks[0].ptr;   // Chunk G
    char* small_chunk = (char*)new_chunks[1].ptr;   // Chunk H
    
    // Check if chunks are overlapping
    void* large_end = (char*)large_chunk + new_chunks[0].size;
    printf("  [Thread %ld] Large chunk range: [%p - %p]\n", 
           thread_id, large_chunk, large_end);
    printf("  [Thread %ld] Small chunk start: %p\n", thread_id, small_chunk);
    
    // Initialize victim data in small chunk
    printf("  [Thread %ld] Writing victim data to chunk H\n", thread_id);
    memcpy(small_chunk, VICTIM_DATA, strlen(VICTIM_DATA));
    small_chunk[strlen(VICTIM_DATA)] = '\0';
    
    printf("  [Thread %ld] Chunk H contains: \"%s\"\n", thread_id, small_chunk);
    
    // Set up signal handling for overlap exploitation
    if (sigsetjmp(signal_env, 1) == 0) {
        // Calculate offset into large chunk that should overlap with small chunk
        size_t overlap_offset = MEDIUM_CHUNK_SIZE;  // Estimated overlap position
        
        printf("  [Thread %ld] Attempting to write pattern from large chunk at offset 0x%zx\n", 
               thread_id, overlap_offset);
        
        // This should overlap with the small chunk if heap manipulation succeeded
        memset(large_chunk + overlap_offset, ATTACK_PATTERN, PATTERN_SIZE);
        
        printf("  [Thread %ld] Pattern write completed\n", thread_id);
        
        // Check if victim data was corrupted
        printf("  [Thread %ld] Checking chunk H after pattern write: \"%s\"\n", 
               thread_id, small_chunk);
        
        if (small_chunk[0] == ATTACK_PATTERN) {
            printf("  [Thread %ld] SUCCESS: Victim data corrupted by overlap!\n", thread_id);
            result = TEST_RESULT_DATA_CORRUPTION;
        } else {
            printf("  [Thread %ld] Victim data unchanged - no overlap detected\n", thread_id);
            result = TEST_RESULT_CHERI_PROTECTED;
        }
        
        // Additional overlap detection: check if addresses overlap
        if (small_chunk >= large_chunk && 
            small_chunk < (char*)large_chunk + new_chunks[0].size) {
            printf("  [Thread %ld] Address overlap detected: chunk H within chunk G\n", 
                   thread_id);
            result = TEST_RESULT_OVERLAPPING_CHUNKS;
        }
        
    } else {
        printf("  [Thread %ld] PROTECTION: Signal %d caught during overlap test\n", 
               thread_id, signal_caught);
        result = TEST_RESULT_EXCEPTION;
    }
    
    return result;
}

/**
 * @brief Main heap manipulation test function
 */
static void* heap_manipulation_vulnerability_test(void* arg) {
    long thread_id = (long)arg;
    
    printf("[Thread %ld] ==> Starting Heap Manipulation (Parent/Child) Test\n", thread_id);
    
    // Setup signal handlers for this thread
    setup_signal_handlers();
    
    chunk_info_t initial_chunks[3] = {0};
    chunk_info_t new_chunks[2] = {0};
    test_result_t final_result = TEST_RESULT_UNKNOWN;
    
    // Phase 1: Allocate initial chunks
    if (phase1_allocate_chunks(thread_id, initial_chunks) != 0) {
        global_stats.malloc_failures++;
        printf("[Thread %ld] <== Test completed: MALLOC_FAILED\n", thread_id);
        return NULL;
    }
    
    // Phase 2: Attempt heap metadata manipulation
    test_result_t metadata_result = phase2_manipulate_metadata(thread_id, initial_chunks);
    
    // Phase 3: Free and reallocate (only if metadata manipulation didn't crash)
    if (metadata_result != TEST_RESULT_EXCEPTION) {
        if (phase3_free_and_reallocate(thread_id, initial_chunks, new_chunks) == 0) {
            // Phase 4: Test overlap exploitation
            final_result = phase4_test_overlap_exploitation(thread_id, new_chunks);
        } else {
            final_result = TEST_RESULT_MALLOC_FAILED;
        }
    } else {
        final_result = metadata_result;
    }
    
    // Cleanup
    if (initial_chunks[0].ptr) free(initial_chunks[0].ptr);
    for (int i = 0; i < 2; i++) {
        if (new_chunks[i].ptr) free(new_chunks[i].ptr);
    }
    
    // Update statistics
    global_stats.total_tests++;
    switch (final_result) {
        case TEST_RESULT_HEAP_CORRUPTION:
        case TEST_RESULT_OVERLAPPING_CHUNKS:
            global_stats.overlapping_chunks++;
            break;
        case TEST_RESULT_DATA_CORRUPTION:
            global_stats.data_corruptions++;
            break;
        case TEST_RESULT_CHERI_PROTECTED:
            global_stats.cheri_protections++;
            break;
        case TEST_RESULT_EXCEPTION:
            global_stats.exceptions++;
            break;
        case TEST_RESULT_MALLOC_FAILED:
            global_stats.malloc_failures++;
            break;
        default:
            break;
    }
    
    printf("[Thread %ld] <== Test completed: %d\n", thread_id, final_result);
    return NULL;
}

/**
 * @brief Print comprehensive test results and analysis
 */
static void print_test_analysis(void) {
    printf("\n================================================================================\n");
    printf("HEAP MANIPULATION (PARENT/CHILD CHUNK) - TEST ANALYSIS\n");
    printf("================================================================================\n");
    
    printf("Test Statistics:\n");
    printf("  Total tests executed:         %d\n", global_stats.total_tests);
    printf("  Heap corruptions:             %d\n", global_stats.heap_corruptions);
    printf("  Overlapping chunks:           %d\n", global_stats.overlapping_chunks);
    printf("  Data corruptions:             %d\n", global_stats.data_corruptions);
    printf("  CHERI protections:            %d\n", global_stats.cheri_protections);
    printf("  Exceptions caught:            %d\n", global_stats.exceptions);
    printf("  Memory allocation failures:   %d\n", global_stats.malloc_failures);
    
    printf("\nSecurity Analysis:\n");
    if (global_stats.overlapping_chunks > 0 || global_stats.data_corruptions > 0) {
        printf("  ❌ VULNERABILITY: Heap manipulation attack succeeded\n");
        printf("     - Out-of-bounds write enabled metadata corruption\n");
        printf("     - Overlapping memory chunks created\n");
        printf("     - Arbitrary read/write primitive achieved\n");
        printf("     - System lacks heap metadata protection\n");
    }
    
    if (global_stats.heap_corruptions > 0) {
        printf("  ⚠️  CORRUPTION: Heap metadata successfully modified\n");
        printf("     - Chunk size fields were manipulated\n");
        printf("     - Heap allocator assumptions violated\n");
    }
    
    if (global_stats.cheri_protections > 0 || global_stats.exceptions > 0) {
        printf("  ✅ PROTECTION: CHERI mitigations active\n");
        printf("     - Capability bounds prevented metadata modification\n");
        printf("     - Spatial safety prevented chunk overlap\n");
        printf("     - Out-of-bounds writes blocked by capability system\n");
    }
    
    printf("\nHeap Manipulation Technique Analysis:\n");
    printf("  • Metadata Corruption: Modifying heap chunk size fields\n");
    printf("  • Parent/Child Relationship: Creating overlapping memory regions\n");
    printf("  • Arbitrary Access: Exploiting overlap for unauthorized memory access\n");
    printf("  • Heap Feng Shui: Manipulating heap layout for exploitation\n");
    
    printf("\nCHERI-Morello Mitigation Analysis:\n");
    printf("  • Spatial Safety: Capability bounds prevent out-of-bounds writes\n");
    printf("  • Heap Protection: Metadata integrity maintained by capability system\n");
    printf("  • Bounds Checking: All memory accesses validated against capabilities\n");
    printf("  • Memory Isolation: Distinct allocations have separate capabilities\n");
    
    printf("\nEducational Value:\n");
    printf("  • Demonstrates advanced heap exploitation techniques\n");
    printf("  • Shows relationship between metadata and memory layout\n");
    printf("  • Illustrates importance of heap integrity protection\n");
    printf("  • Highlights CHERI's spatial memory safety advantages\n");
    
    printf("================================================================================\n");
}

/**
 * @brief Main function - orchestrates the heap manipulation test
 */
int main(void) {
    printf("Heap Manipulation (Parent/Child Chunk) Vulnerability Test (Refactored)\n");
    printf("======================================================================\n");
    printf("Testing heap manipulation via parent/child chunk overlap\n");
    printf("Expected on CHERI: Capability violations prevent heap metadata corruption\n\n");
    
    // Initialize xBGAS runtime
    if (xbrtime_init() != 0) {
        fprintf(stderr, "ERROR: Failed to initialize xBGAS runtime\n");
        return EXIT_FAILURE;
    }
    
    int num_pes = xbrtime_num_pes();
    printf("Executing heap manipulation tests on %d processing elements\n\n", num_pes);
    
    // Execute tests across all PEs
    for (long i = 0; i < num_pes; i++) {
        tpool_add_work(threads[i].thread_queue, 
                      heap_manipulation_vulnerability_test, 
                      (void*)i);
    }
    
    // Wait for all tests to complete
    for (int i = 0; i < num_pes; i++) {
        tpool_wait(threads[i].thread_queue);
    }
    
    // Print comprehensive analysis
    print_test_analysis();
    
    // Cleanup xBGAS runtime
    xbrtime_close();
    
    // Return appropriate exit code
    if (global_stats.overlapping_chunks > 0 || global_stats.data_corruptions > 0) {
        printf("\nTest Result: VULNERABILITY DETECTED - Heap manipulation succeeded\n");
        return EXIT_FAILURE;
    } else if (global_stats.cheri_protections > 0 || global_stats.exceptions > 0) {
        printf("\nTest Result: CHERI PROTECTION ACTIVE - Heap manipulation prevented\n");
        return EXIT_SUCCESS;
    } else {
        printf("\nTest Result: INCONCLUSIVE - Check system configuration\n");
        return EXIT_FAILURE;
    }
}
