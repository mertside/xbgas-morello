/**
 * @file ttu_r1_HeartBleed_refactored.c
 * @brief Refactored HeartBleed Vulnerability Security Test
 * @author Mert Side (Texas Tech University)
 * @date 2024
 * 
 * @section DESCRIPTION
 * This program demonstrates a HeartBleed-like vulnerability, which is a famous
 * real-world exploit that affected OpenSSL. The vulnerability occurs when:
 * 1. A client sends a heartbeat request with a payload and length parameter
 * 2. The server responds by echoing back the specified length of data
 * 3. If the length parameter is larger than the actual payload, the server
 *    reads beyond the payload buffer, potentially exposing sensitive data
 * 
 * @section VULNERABILITY_DETAILS
 * HeartBleed (CVE-2014-0160) allowed attackers to:
 * - Read up to 64KB of server memory per request
 * - Extract sensitive information like private keys, passwords, and session data
 * - Perform attacks repeatedly without detection
 * - Compromise SSL/TLS encryption integrity
 * 
 * @section CHERI_ANALYSIS
 * On CHERI-Morello systems, this test evaluates:
 * - Spatial memory safety for buffer operations
 * - Capability bounds checking on memory reads
 * - Protection against information disclosure attacks
 * - Detection of out-of-bounds read operations
 * 
 * @section EXPECTED_BEHAVIOR
 * - Traditional systems: May read sensitive data beyond buffer bounds (vulnerability)
 * - CHERI-Morello: Should trap on capability bounds violation, preventing data exposure
 * 
 * @section TEST_PHASES
 * 1. SETUP: Initialize test environment and allocate buffers
 * 2. ALLOCATE_HEARTBEAT: Create heartbeat message buffer
 * 3. ALLOCATE_SENSITIVE: Create sensitive data buffer (simulated RSA key)
 * 4. POPULATE_DATA: Fill buffers with test data
 * 5. CALCULATE_LAYOUT: Determine memory layout and offsets
 * 6. HEARTBLEED_ATTACK: Attempt to read beyond heartbeat buffer
 * 7. ANALYZE_EXPOSURE: Check if sensitive data was exposed
 * 8. VERIFY_PROTECTION: Validate CHERI protection effectiveness
 * 9. CLEANUP: Free allocated memory
 * 10. REPORT: Generate security assessment report
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
#define TEST_NAME "HeartBleed Vulnerability"
#define TEST_ID "TTU_R1"
#define TEST_CATEGORY "REAL_WORLD_EXPLOIT"

/** @brief Buffer and data constants */
#define HEARTBEAT_BUFFER_SIZE 16
#define SENSITIVE_DATA_SIZE 32
#define MAX_READ_ATTEMPT 128
#define HEARTBEAT_MESSAGE "HB_REQUEST"
#define SENSITIVE_DATA "RSA_PRIVATE_KEY_DATA_SENSITIVE"

/** @brief Test execution parameters */
#define NUM_THREADS 4
#define MAX_MEMORY_EXPOSURE 1024

/** @brief Test phases for structured execution */
typedef enum {
    PHASE_SETUP = 0,
    PHASE_ALLOCATE_HEARTBEAT,
    PHASE_ALLOCATE_SENSITIVE,
    PHASE_POPULATE_DATA,
    PHASE_CALCULATE_LAYOUT,
    PHASE_HEARTBLEED_ATTACK,
    PHASE_ANALYZE_EXPOSURE,
    PHASE_VERIFY_PROTECTION,
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
    char* heartbeat_buffer;
    char* sensitive_buffer;
    size_t memory_offset;
    int data_exposed;
    int attack_successful;
    size_t bytes_exposed;
    char exposed_data[MAX_MEMORY_EXPOSURE];
} heartbleed_context_t;

//=============================================================================
// GLOBAL STATE AND SIGNAL HANDLING
//=============================================================================

/** @brief Global test state */
static struct {
    jmp_buf recovery_point;
    volatile sig_atomic_t signal_caught;
    volatile sig_atomic_t current_phase;
    heartbleed_context_t contexts[NUM_THREADS];
    int total_attacks;
    int successful_attacks;
    size_t total_data_exposed;
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

/** @brief Display memory layout information */
static void display_memory_layout(heartbleed_context_t* ctx) {
    printf("[Thread %d] 🏗️  Memory Layout Analysis:\n", ctx->thread_index);
    
    analyze_pointer("Heartbeat buffer", ctx->heartbeat_buffer, ctx->thread_index);
    analyze_pointer("Sensitive buffer", ctx->sensitive_buffer, ctx->thread_index);
    
    // Calculate memory relationship
    if (ctx->heartbeat_buffer && ctx->sensitive_buffer) {
        ptrdiff_t offset = ctx->sensitive_buffer - ctx->heartbeat_buffer;
        printf("[Thread %d] 📏 Memory offset: %td bytes (%s)\n", 
               ctx->thread_index, offset, 
               (offset > 0) ? "sensitive after heartbeat" : "sensitive before heartbeat");
        ctx->memory_offset = abs(offset);
    }
}

//=============================================================================
// HEARTBLEED ATTACK SIMULATION
//=============================================================================

/** @brief Simulate heartbeat request processing (vulnerable) */
static int simulate_heartbeat_response(heartbleed_context_t* ctx, size_t requested_length) {
    printf("[Thread %d] 💓 Processing heartbeat request for %zu bytes\n", 
           ctx->thread_index, requested_length);
    
    // This is the vulnerable code pattern from HeartBleed
    // It trusts the client-provided length without validation
    
    char* response_buffer = malloc(requested_length);
    if (!response_buffer) {
        printf("[Thread %d] ❌ Failed to allocate response buffer\n", ctx->thread_index);
        return 0;
    }
    
    printf("[Thread %d] 📖 Reading %zu bytes starting from heartbeat buffer...\n", 
           ctx->thread_index, requested_length);
    
    // The vulnerability: memcpy with user-controlled length
    // This may read beyond the heartbeat buffer into sensitive memory
    memcpy(response_buffer, ctx->heartbeat_buffer, requested_length);
    
    // Analyze what was read
    printf("[Thread %d] 📊 HeartBleed response analysis:\n", ctx->thread_index);
    
    size_t actual_heartbeat_len = strlen(HEARTBEAT_MESSAGE);
    int sensitive_data_found = 0;
    
    for (size_t i = 0; i < requested_length && i < MAX_MEMORY_EXPOSURE; i++) {
        char c = response_buffer[i];
        ctx->exposed_data[i] = c;
        
        // Check if we've read beyond the legitimate heartbeat data
        if (i >= actual_heartbeat_len) {
            // Check if this looks like sensitive data
            if (c >= 32 && c <= 126) {  // Printable ASCII
                printf("[Thread %d] 🚨 Byte %zu: '%c' (potentially sensitive)\n", 
                       ctx->thread_index, i, c);
                
                // Check if this matches our sensitive data
                if (ctx->sensitive_buffer && 
                    i < actual_heartbeat_len + ctx->memory_offset + SENSITIVE_DATA_SIZE) {
                    size_t sensitive_idx = i - actual_heartbeat_len - ctx->memory_offset;
                    if (sensitive_idx < strlen(SENSITIVE_DATA) && 
                        c == SENSITIVE_DATA[sensitive_idx]) {
                        sensitive_data_found = 1;
                        printf("[Thread %d] 💥 SENSITIVE DATA EXPOSED: Position %zu matches RSA key!\n", 
                               ctx->thread_index, i);
                    }
                }
            } else {
                printf("[Thread %d] 📊 Byte %zu: \\x%02x (binary data)\n", 
                       ctx->thread_index, i, (unsigned char)c);
            }
        }
    }
    
    ctx->bytes_exposed = requested_length;
    ctx->data_exposed = sensitive_data_found;
    
    free(response_buffer);
    return sensitive_data_found;
}

//=============================================================================
// CORE TEST LOGIC
//=============================================================================

/** @brief Execute HeartBleed vulnerability test for a single thread */
static void* execute_heartbleed_test(void* arg) {
    heartbleed_context_t* ctx = (heartbleed_context_t*)arg;
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
    ctx->data_exposed = 0;
    ctx->attack_successful = 0;
    ctx->bytes_exposed = 0;
    
    // PHASE 2: ALLOCATE_HEARTBEAT
    test_state.current_phase = PHASE_ALLOCATE_HEARTBEAT;
    printf("[Thread %d] 📋 Phase %d: Allocate heartbeat buffer\n", 
           ctx->thread_index, PHASE_ALLOCATE_HEARTBEAT);
    
    ctx->heartbeat_buffer = malloc(HEARTBEAT_BUFFER_SIZE);
    if (!ctx->heartbeat_buffer) {
        printf("[Thread %d] ❌ Failed to allocate heartbeat buffer\n", ctx->thread_index);
        return NULL;
    }
    
    // PHASE 3: ALLOCATE_SENSITIVE
    test_state.current_phase = PHASE_ALLOCATE_SENSITIVE;
    printf("[Thread %d] 📋 Phase %d: Allocate sensitive data buffer\n", 
           ctx->thread_index, PHASE_ALLOCATE_SENSITIVE);
    
    ctx->sensitive_buffer = malloc(SENSITIVE_DATA_SIZE);
    if (!ctx->sensitive_buffer) {
        printf("[Thread %d] ❌ Failed to allocate sensitive buffer\n", ctx->thread_index);
        free(ctx->heartbeat_buffer);
        return NULL;
    }
    
    // PHASE 4: POPULATE_DATA
    test_state.current_phase = PHASE_POPULATE_DATA;
    printf("[Thread %d] 📋 Phase %d: Populate buffers with data\n", 
           ctx->thread_index, PHASE_POPULATE_DATA);
    
    // Initialize heartbeat buffer
    memset(ctx->heartbeat_buffer, 0, HEARTBEAT_BUFFER_SIZE);
    strncpy(ctx->heartbeat_buffer, HEARTBEAT_MESSAGE, HEARTBEAT_BUFFER_SIZE - 1);
    
    // Initialize sensitive buffer
    memset(ctx->sensitive_buffer, 0, SENSITIVE_DATA_SIZE);
    strncpy(ctx->sensitive_buffer, SENSITIVE_DATA, SENSITIVE_DATA_SIZE - 1);
    
    printf("[Thread %d] 📝 Heartbeat message: '%s'\n", ctx->thread_index, ctx->heartbeat_buffer);
    printf("[Thread %d] 🔐 Sensitive data: '%s'\n", ctx->thread_index, ctx->sensitive_buffer);
    
    // PHASE 5: CALCULATE_LAYOUT
    test_state.current_phase = PHASE_CALCULATE_LAYOUT;
    printf("[Thread %d] 📋 Phase %d: Calculate memory layout\n", 
           ctx->thread_index, PHASE_CALCULATE_LAYOUT);
    
    display_memory_layout(ctx);
    
    // PHASE 6: HEARTBLEED_ATTACK
    test_state.current_phase = PHASE_HEARTBLEED_ATTACK;
    printf("[Thread %d] 📋 Phase %d: Execute HeartBleed attack\n", 
           ctx->thread_index, PHASE_HEARTBLEED_ATTACK);
    printf("[Thread %d] 🚨 CRITICAL: Attempting HeartBleed exploit\n", ctx->thread_index);
    
    // Calculate malicious request length
    size_t legitimate_length = strlen(HEARTBEAT_MESSAGE);
    size_t malicious_length = legitimate_length + ctx->memory_offset + SENSITIVE_DATA_SIZE;
    
    printf("[Thread %d] 💥 Requesting %zu bytes (legitimate: %zu, extra: %zu)\n", 
           ctx->thread_index, malicious_length, legitimate_length, 
           malicious_length - legitimate_length);
    
    // Perform the attack
    ctx->attack_successful = simulate_heartbeat_response(ctx, malicious_length);
    
    // PHASE 7: ANALYZE_EXPOSURE
    test_state.current_phase = PHASE_ANALYZE_EXPOSURE;
    printf("[Thread %d] 📋 Phase %d: Analyze data exposure\n", 
           ctx->thread_index, PHASE_ANALYZE_EXPOSURE);
    
    if (ctx->attack_successful) {
        printf("[Thread %d] 🚨 ATTACK SUCCESS: Sensitive data exposed!\n", ctx->thread_index);
        printf("[Thread %d] 💀 HeartBleed vulnerability exploited\n", ctx->thread_index);
        test_state.successful_attacks++;
    } else {
        printf("[Thread %d] ✅ ATTACK FAILED: No sensitive data exposed\n", ctx->thread_index);
        printf("[Thread %d] 🔒 HeartBleed vulnerability mitigated\n", ctx->thread_index);
    }
    
    test_state.total_attacks++;
    test_state.total_data_exposed += ctx->bytes_exposed;
    
cleanup_and_exit:
    // PHASE 8: CLEANUP
    test_state.current_phase = PHASE_CLEANUP;
    printf("[Thread %d] 📋 Phase %d: Cleanup\n", ctx->thread_index, PHASE_CLEANUP);
    
    if (ctx->heartbeat_buffer) {
        free(ctx->heartbeat_buffer);
        ctx->heartbeat_buffer = NULL;
    }
    
    if (ctx->sensitive_buffer) {
        free(ctx->sensitive_buffer);
        ctx->sensitive_buffer = NULL;
    }
    
    // PHASE 9: REPORT
    test_state.current_phase = PHASE_REPORT;
    printf("[Thread %d] 📋 Phase %d: Generate report\n", ctx->thread_index, PHASE_REPORT);
    
    if (recovery_signal != 0) {
        printf("[Thread %d] ✅ Test completed with CHERI protection (signal %d)\n", 
               ctx->thread_index, recovery_signal);
        printf("[Thread %d] 🔒 HeartBleed attack prevented by capability bounds\n", 
               ctx->thread_index);
    } else {
        printf("[Thread %d] ❌ Test completed without protection\n", ctx->thread_index);
        if (ctx->attack_successful) {
            printf("[Thread %d] 💥 HeartBleed vulnerability exploitable\n", ctx->thread_index);
        }
    }
    
    printf("[Thread %d] 🏁 %s test finished\n\n", ctx->thread_index, TEST_NAME);
    return NULL;
}

//=============================================================================
// MAIN FUNCTION AND TEST ORCHESTRATION
//=============================================================================

/**
 * @brief Main function - orchestrates the multi-threaded HeartBleed test
 * @return 0 on success, non-zero on failure
 */
int main(void) {
    printf("=================================================================\n");
    printf("🔬 xBGAS Security Test: %s\n", TEST_NAME);
    printf("📊 Test ID: %s | Category: %s\n", TEST_ID, TEST_CATEGORY);
    printf("🎯 Platform: CHERI-Morello | Exploit: CVE-2014-0160\n");
    printf("=================================================================\n\n");
    
    printf("📖 HeartBleed Vulnerability Background:\n");
    printf("   - CVE-2014-0160: OpenSSL Heartbeat Extension Vulnerability\n");
    printf("   - Allows reading up to 64KB of server memory per request\n");
    printf("   - Can expose private keys, passwords, and sensitive data\n");
    printf("   - Affected millions of servers worldwide (2014)\n\n");
    
    // Initialize test state
    memset(&test_state, 0, sizeof(test_state));
    
    printf("🧵 Starting multi-threaded HeartBleed simulation...\n");
    printf("📊 Number of threads: %d\n\n", NUM_THREADS);
    
    // Create and start threads
    for (int i = 0; i < NUM_THREADS; i++) {
        test_state.contexts[i].thread_index = i;
        
        if (pthread_create(&test_state.contexts[i].thread_id, NULL, 
                          execute_heartbleed_test, &test_state.contexts[i]) != 0) {
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
    printf("📈 HeartBleed Test Summary Report\n");
    printf("=================================================================\n");
    printf("🎯 Total attack attempts: %d\n", test_state.total_attacks);
    printf("💥 Successful attacks: %d\n", test_state.successful_attacks);
    printf("📊 Attack success rate: %.1f%%\n", 
           test_state.total_attacks > 0 ? 
           (100.0 * test_state.successful_attacks / test_state.total_attacks) : 0.0);
    printf("📏 Total data exposed: %zu bytes\n", test_state.total_data_exposed);
    
    if (test_state.successful_attacks > 0) {
        printf("🚨 VULNERABILITY STATUS: EXPLOITABLE\n");
        printf("💀 HeartBleed attacks succeeded - sensitive data exposed\n");
        printf("⚠️  System vulnerable to information disclosure\n");
    } else {
        printf("✅ VULNERABILITY STATUS: MITIGATED\n");
        printf("🔒 HeartBleed attacks failed - no data exposure\n");
        printf("🛡️  CHERI capability system provided protection\n");
    }
    
    printf("=================================================================\n");
    printf("🔒 CHERI-Morello HeartBleed protection evaluation complete\n");
    printf("=================================================================\n");
    
    return 0;
}

/**
 * @brief Test Summary
 * 
 * This refactored test provides comprehensive evaluation of:
 * 
 * 1. **Real-World Exploit Simulation**: Accurate HeartBleed vulnerability reproduction
 * 2. **Information Disclosure Testing**: Validates protection against data exposure
 * 3. **CHERI Capability System**: Tests spatial memory safety enforcement
 * 4. **Buffer Bounds Checking**: Prevents out-of-bounds memory access
 * 5. **Multi-threaded Attack**: Concurrent exploitation attempts
 * 6. **Memory Layout Analysis**: Detailed buffer relationship examination
 * 7. **Attack Success Metrics**: Quantitative vulnerability assessment
 * 8. **Educational Value**: Demonstrates famous real-world security flaw
 * 
 * Expected behavior on CHERI-Morello:
 * - Memory reads should be bounded by capability limits
 * - Attempt to read beyond buffer bounds should trap
 * - Sensitive data exposure should be prevented
 * - Attack should fail with capability violation
 * 
 * On traditional systems:
 * - Buffer over-read may succeed
 * - Sensitive data may be exposed
 * - HeartBleed vulnerability would be exploitable
 * - Information disclosure attack would succeed
 */
