/**
 * @file ttu_r2_dop_refactored.c
 * @brief Refactored Data-Oriented Programming (DOP) Security Test
 * @author Mert Side (Texas Tech University)
 * @date 2024
 * 
 * @section DESCRIPTION
 * This program demonstrates a Data-Oriented Programming (DOP) attack, which is
 * a sophisticated exploitation technique that manipulates program data rather than
 * control flow. The vulnerability occurs when:
 * 1. A buffer overflow is used to corrupt adjacent memory regions
 * 2. Critical control variables (like access flags) are overwritten
 * 3. Program logic is subverted without hijacking control flow
 * 4. Legitimate code execution leads to unauthorized behavior
 * 
 * @section DOP_ATTACK_DETAILS
 * Data-Oriented Programming attacks:
 * - Exploit spatial memory safety violations to corrupt data
 * - Target security-critical variables and data structures
 * - Remain undetected by control-flow integrity (CFI) mechanisms
 * - Can achieve arbitrary computation through data manipulation
 * - Are harder to detect than traditional ROP/JOP attacks
 * 
 * @section CHERI_ANALYSIS
 * On CHERI-Morello systems, this test evaluates:
 * - Spatial memory safety for array bounds checking
 * - Capability bounds enforcement on buffer operations
 * - Protection against adjacent variable corruption
 * - Detection of out-of-bounds write operations targeting data
 * 
 * @section EXPECTED_BEHAVIOR
 * - Traditional systems: May corrupt adjacent variables (DOP attack succeeds)
 * - CHERI-Morello: Should trap on capability bounds violation, preventing data corruption
 * 
 * @section TEST_PHASES
 * 1. SETUP: Initialize test environment and allocate memory layout
 * 2. ALLOCATE_VARIABLES: Create vulnerable buffer and target control variable
 * 3. ANALYZE_LAYOUT: Determine memory layout and calculate corruption offset
 * 4. PREPARE_PAYLOAD: Create malicious data payload for DOP attack
 * 5. DOP_ATTACK: Attempt buffer overflow to corrupt control variable
 * 6. VERIFY_CORRUPTION: Check if control variable was successfully modified
 * 7. ASSESS_PRIVILEGE: Test if unauthorized access was gained
 * 8. ANALYZE_IMPACT: Evaluate the security impact of the attack
 * 9. CLEANUP: Clean up test environment
 * 10. REPORT: Generate comprehensive security assessment
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <setjmp.h>
#include <unistd.h>
#include <pthread.h>
#include <stdint.h>
#include <stdbool.h>

// Include legacy header for compatibility
#include "xbrtime_morello.h"

//=============================================================================
// TEST CONFIGURATION AND CONSTANTS
//=============================================================================

/** @brief Test identification */
#define TEST_NAME "Data-Oriented Programming (DOP)"
#define TEST_ID "TTU_R2"
#define TEST_CATEGORY "REAL_WORLD_EXPLOIT"

/** @brief Buffer and attack constants */
#define VULNERABLE_BUFFER_SIZE 8
#define MALICIOUS_PAYLOAD_SIZE 16
#define CONTROL_VARIABLE_MAGIC 0xDEADBEEF
#define ADMIN_ACCESS_GRANTED 1
#define ADMIN_ACCESS_DENIED 0

/** @brief Test execution parameters */
#define NUM_THREADS 4
#define MAX_CORRUPTION_ATTEMPTS 10

/** @brief Test phases for structured execution */
typedef enum {
    PHASE_SETUP = 0,
    PHASE_ALLOCATE_VARIABLES,
    PHASE_ANALYZE_LAYOUT,
    PHASE_PREPARE_PAYLOAD,
    PHASE_DOP_ATTACK,
    PHASE_VERIFY_CORRUPTION,
    PHASE_ASSESS_PRIVILEGE,
    PHASE_ANALYZE_IMPACT,
    PHASE_CLEANUP,
    PHASE_REPORT,
    PHASE_MAX
} test_phase_t;

//=============================================================================
// DATA STRUCTURES
//=============================================================================

/** @brief Vulnerable data structure layout */
typedef struct {
    int vulnerable_buffer[VULNERABLE_BUFFER_SIZE];
    int admin_access_flag;
    int security_level;
    int user_permissions;
    char padding[16];  // Additional padding for layout analysis
} vulnerable_data_t;

/** @brief Thread test context */
typedef struct {
    pthread_t thread_id;
    int thread_index;
    vulnerable_data_t* data_structure;
    int* malicious_payload;
    ptrdiff_t corruption_offset;
    int attack_successful;
    int original_admin_flag;
    int corrupted_admin_flag;
    size_t bytes_corrupted;
} dop_context_t;

//=============================================================================
// GLOBAL STATE AND SIGNAL HANDLING
//=============================================================================

/** @brief Global test state */
static struct {
    jmp_buf recovery_point;
    volatile sig_atomic_t signal_caught;
    volatile sig_atomic_t current_phase;
    dop_context_t contexts[NUM_THREADS];
    int total_attacks;
    int successful_attacks;
    size_t total_corruption;
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

/** @brief Display memory layout for DOP analysis */
static void display_memory_layout(dop_context_t* ctx) {
    printf("[Thread %d] 🏗️  Memory Layout Analysis for DOP Attack:\n", ctx->thread_index);
    
    if (!ctx->data_structure) {
        printf("[Thread %d] ❌ Data structure not allocated\n", ctx->thread_index);
        return;
    }
    
    // Analyze the vulnerable data structure layout
    volatile vulnerable_data_t* data = ctx->data_structure;
    
    printf("[Thread %d] 📊 Data Structure Layout:\n", ctx->thread_index);
    analyze_pointer("Data structure", (void*)data, ctx->thread_index);
    analyze_pointer("Vulnerable buffer", (void*)&data->vulnerable_buffer, ctx->thread_index);
    analyze_pointer("Admin access flag", (void*)&data->admin_access_flag, ctx->thread_index);
    analyze_pointer("Security level", (void*)&data->security_level, ctx->thread_index);
    analyze_pointer("User permissions", (void*)&data->user_permissions, ctx->thread_index);
    
    // Calculate critical offsets for DOP attack
    ptrdiff_t buffer_to_admin = (char*)&data->admin_access_flag - (char*)&data->vulnerable_buffer;
    ptrdiff_t buffer_to_security = (char*)&data->security_level - (char*)&data->vulnerable_buffer;
    ptrdiff_t buffer_to_permissions = (char*)&data->user_permissions - (char*)&data->vulnerable_buffer;
    
    printf("[Thread %d] 📏 Critical Offsets for DOP:\n", ctx->thread_index);
    printf("[Thread %d]   Buffer to admin flag: %td bytes\n", ctx->thread_index, buffer_to_admin);
    printf("[Thread %d]   Buffer to security level: %td bytes\n", ctx->thread_index, buffer_to_security);
    printf("[Thread %d]   Buffer to permissions: %td bytes\n", ctx->thread_index, buffer_to_permissions);
    
    ctx->corruption_offset = buffer_to_admin / sizeof(int);
    printf("[Thread %d] 🎯 DOP target offset: %td array elements\n", ctx->thread_index, ctx->corruption_offset);
}

//=============================================================================
// DOP ATTACK SIMULATION
//=============================================================================

/** @brief Create malicious payload for DOP attack */
static void create_malicious_payload(dop_context_t* ctx) {
    printf("[Thread %d] 🔧 Creating malicious DOP payload\n", ctx->thread_index);
    
    ctx->malicious_payload = malloc(MALICIOUS_PAYLOAD_SIZE * sizeof(int));
    if (!ctx->malicious_payload) {
        printf("[Thread %d] ❌ Failed to allocate malicious payload\n", ctx->thread_index);
        return;
    }
    
    // Create payload that will corrupt the admin access flag
    for (int i = 0; i < MALICIOUS_PAYLOAD_SIZE; i++) {
        if (i == ctx->corruption_offset) {
            // This element should overwrite the admin access flag
            ctx->malicious_payload[i] = ADMIN_ACCESS_GRANTED;
            printf("[Thread %d] 💀 Payload[%d] = %d (admin flag corruption)\n", 
                   ctx->thread_index, i, ADMIN_ACCESS_GRANTED);
        } else if (i == ctx->corruption_offset + 1) {
            // This element should overwrite the security level
            ctx->malicious_payload[i] = 0;  // Lower security level
            printf("[Thread %d] 💀 Payload[%d] = %d (security level corruption)\n", 
                   ctx->thread_index, i, 0);
        } else if (i == ctx->corruption_offset + 2) {
            // This element should overwrite user permissions
            ctx->malicious_payload[i] = 0xFFFFFFFF;  // Grant all permissions
            printf("[Thread %d] 💀 Payload[%d] = %d (permissions corruption)\n", 
                   ctx->thread_index, i, 0xFFFFFFFF);
        } else {
            // Fill with recognizable pattern
            ctx->malicious_payload[i] = 0x41414141 + i;
            printf("[Thread %d] 📝 Payload[%d] = %#x (filler data)\n", 
                   ctx->thread_index, i, ctx->malicious_payload[i]);
        }
    }
}

/** @brief Attempt DOP attack through buffer overflow */
static int execute_dop_attack(dop_context_t* ctx) {
    printf("[Thread %d] 💥 Executing DOP attack\n", ctx->thread_index);
    
    if (!ctx->data_structure || !ctx->malicious_payload) {
        printf("[Thread %d] ❌ Attack prerequisites not met\n", ctx->thread_index);
        return 0;
    }
    
    // Record original state
    ctx->original_admin_flag = ctx->data_structure->admin_access_flag;
    
    printf("[Thread %d] 📊 Pre-attack state:\n", ctx->thread_index);
    printf("[Thread %d]   Admin access: %d\n", ctx->thread_index, ctx->data_structure->admin_access_flag);
    printf("[Thread %d]   Security level: %d\n", ctx->thread_index, ctx->data_structure->security_level);
    printf("[Thread %d]   User permissions: %#x\n", ctx->thread_index, ctx->data_structure->user_permissions);
    
    // This is the DOP attack - buffer overflow to corrupt adjacent data
    printf("[Thread %d] 🚨 CRITICAL: Performing buffer overflow for DOP\n", ctx->thread_index);
    
    for (int i = 0; i < MALICIOUS_PAYLOAD_SIZE; i++) {
        // This should trigger CHERI bounds checking
        printf("[Thread %d] 💀 Writing to buffer[%d] = %#x\n", 
               ctx->thread_index, i, ctx->malicious_payload[i]);
        ctx->data_structure->vulnerable_buffer[i] = ctx->malicious_payload[i];
    }
    
    // Check if the attack succeeded
    ctx->corrupted_admin_flag = ctx->data_structure->admin_access_flag;
    
    printf("[Thread %d] 📊 Post-attack state:\n", ctx->thread_index);
    printf("[Thread %d]   Admin access: %d\n", ctx->thread_index, ctx->data_structure->admin_access_flag);
    printf("[Thread %d]   Security level: %d\n", ctx->thread_index, ctx->data_structure->security_level);
    printf("[Thread %d]   User permissions: %#x\n", ctx->thread_index, ctx->data_structure->user_permissions);
    
    // Determine if DOP attack was successful
    if (ctx->corrupted_admin_flag != ctx->original_admin_flag) {
        printf("[Thread %d] 🚨 DOP ATTACK SUCCESS: Admin flag corrupted!\n", ctx->thread_index);
        printf("[Thread %d] 💥 Original: %d, Corrupted: %d\n", 
               ctx->thread_index, ctx->original_admin_flag, ctx->corrupted_admin_flag);
        return 1;
    } else {
        printf("[Thread %d] ✅ DOP ATTACK FAILED: Admin flag unchanged\n", ctx->thread_index);
        return 0;
    }
}

//=============================================================================
// PRIVILEGE ESCALATION SIMULATION
//=============================================================================

/** @brief Simulate privilege check using corrupted data */
static void simulate_privilege_check(dop_context_t* ctx) {
    printf("[Thread %d] 🔐 Simulating privilege escalation check\n", ctx->thread_index);
    
    if (!ctx->data_structure) {
        printf("[Thread %d] ❌ Cannot perform privilege check\n", ctx->thread_index);
        return;
    }
    
    // This simulates legitimate code that checks the admin flag
    if (ctx->data_structure->admin_access_flag == ADMIN_ACCESS_GRANTED) {
        printf("[Thread %d] 🚨 PRIVILEGE ESCALATION: Admin access granted!\n", ctx->thread_index);
        printf("[Thread %d] 💀 Unauthorized operations would be possible\n", ctx->thread_index);
        
        // Simulate admin-level operations
        printf("[Thread %d] 🔓 Simulated admin operations:\n", ctx->thread_index);
        printf("[Thread %d]   - Reading sensitive configuration files\n", ctx->thread_index);
        printf("[Thread %d]   - Modifying system settings\n", ctx->thread_index);
        printf("[Thread %d]   - Accessing restricted databases\n", ctx->thread_index);
        printf("[Thread %d]   - Executing privileged commands\n", ctx->thread_index);
        
    } else {
        printf("[Thread %d] ✅ Access denied: Normal user privileges maintained\n", ctx->thread_index);
    }
    
    // Check security level corruption
    if (ctx->data_structure->security_level < 1) {
        printf("[Thread %d] ⚠️  Security level compromised: %d\n", 
               ctx->thread_index, ctx->data_structure->security_level);
    }
    
    // Check permissions corruption
    if (ctx->data_structure->user_permissions == 0xFFFFFFFF) {
        printf("[Thread %d] ⚠️  User permissions corrupted to full access\n", ctx->thread_index);
    }
}

//=============================================================================
// CORE TEST LOGIC
//=============================================================================

/** @brief Execute DOP vulnerability test for a single thread */
static void* execute_dop_test(void* arg) {
    dop_context_t* ctx = (dop_context_t*)arg;
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
    ctx->attack_successful = 0;
    ctx->bytes_corrupted = 0;
    
    // PHASE 2: ALLOCATE_VARIABLES
    test_state.current_phase = PHASE_ALLOCATE_VARIABLES;
    printf("[Thread %d] 📋 Phase %d: Allocate vulnerable data structure\n", 
           ctx->thread_index, PHASE_ALLOCATE_VARIABLES);
    
    ctx->data_structure = malloc(sizeof(vulnerable_data_t));
    if (!ctx->data_structure) {
        printf("[Thread %d] ❌ Failed to allocate data structure\n", ctx->thread_index);
        return NULL;
    }
    
    // Initialize data structure with secure defaults
    memset(ctx->data_structure, 0, sizeof(vulnerable_data_t));
    ctx->data_structure->admin_access_flag = ADMIN_ACCESS_DENIED;
    ctx->data_structure->security_level = 5;  // High security level
    ctx->data_structure->user_permissions = 0x000F;  // Limited permissions
    
    printf("[Thread %d] 🔧 Data structure initialized securely\n", ctx->thread_index);
    
    // PHASE 3: ANALYZE_LAYOUT
    test_state.current_phase = PHASE_ANALYZE_LAYOUT;
    printf("[Thread %d] 📋 Phase %d: Analyze memory layout\n", 
           ctx->thread_index, PHASE_ANALYZE_LAYOUT);
    
    display_memory_layout(ctx);
    
    // PHASE 4: PREPARE_PAYLOAD
    test_state.current_phase = PHASE_PREPARE_PAYLOAD;
    printf("[Thread %d] 📋 Phase %d: Prepare malicious payload\n", 
           ctx->thread_index, PHASE_PREPARE_PAYLOAD);
    
    create_malicious_payload(ctx);
    
    // PHASE 5: DOP_ATTACK
    test_state.current_phase = PHASE_DOP_ATTACK;
    printf("[Thread %d] 📋 Phase %d: Execute DOP attack\n", 
           ctx->thread_index, PHASE_DOP_ATTACK);
    printf("[Thread %d] 🚨 CRITICAL: Attempting Data-Oriented Programming attack\n", 
           ctx->thread_index);
    
    ctx->attack_successful = execute_dop_attack(ctx);
    
    // PHASE 6: VERIFY_CORRUPTION
    test_state.current_phase = PHASE_VERIFY_CORRUPTION;
    printf("[Thread %d] 📋 Phase %d: Verify data corruption\n", 
           ctx->thread_index, PHASE_VERIFY_CORRUPTION);
    
    if (ctx->attack_successful) {
        printf("[Thread %d] 💥 DATA CORRUPTION SUCCESS: DOP attack effective\n", ctx->thread_index);
        test_state.successful_attacks++;
        ctx->bytes_corrupted = sizeof(int) * 3;  // Admin flag, security level, permissions
    } else {
        printf("[Thread %d] ✅ DATA INTEGRITY PRESERVED: DOP attack failed\n", ctx->thread_index);
    }
    
    // PHASE 7: ASSESS_PRIVILEGE
    test_state.current_phase = PHASE_ASSESS_PRIVILEGE;
    printf("[Thread %d] 📋 Phase %d: Assess privilege escalation\n", 
           ctx->thread_index, PHASE_ASSESS_PRIVILEGE);
    
    simulate_privilege_check(ctx);
    
    test_state.total_attacks++;
    test_state.total_corruption += ctx->bytes_corrupted;
    
cleanup_and_exit:
    // PHASE 8: CLEANUP
    test_state.current_phase = PHASE_CLEANUP;
    printf("[Thread %d] 📋 Phase %d: Cleanup\n", ctx->thread_index, PHASE_CLEANUP);
    
    if (ctx->data_structure) {
        free(ctx->data_structure);
        ctx->data_structure = NULL;
    }
    
    if (ctx->malicious_payload) {
        free(ctx->malicious_payload);
        ctx->malicious_payload = NULL;
    }
    
    // PHASE 9: REPORT
    test_state.current_phase = PHASE_REPORT;
    printf("[Thread %d] 📋 Phase %d: Generate report\n", ctx->thread_index, PHASE_REPORT);
    
    if (recovery_signal != 0) {
        printf("[Thread %d] ✅ Test completed with CHERI protection (signal %d)\n", 
               ctx->thread_index, recovery_signal);
        printf("[Thread %d] 🔒 DOP attack prevented by capability bounds\n", 
               ctx->thread_index);
    } else {
        printf("[Thread %d] ❌ Test completed without protection\n", ctx->thread_index);
        if (ctx->attack_successful) {
            printf("[Thread %d] 💥 DOP vulnerability exploitable\n", ctx->thread_index);
        }
    }
    
    printf("[Thread %d] 🏁 %s test finished\n\n", ctx->thread_index, TEST_NAME);
    return NULL;
}

//=============================================================================
// MAIN FUNCTION AND TEST ORCHESTRATION
//=============================================================================

/**
 * @brief Main function - orchestrates the multi-threaded DOP test
 * @return 0 on success, non-zero on failure
 */
int main(void) {
    printf("=================================================================\n");
    printf("🔬 xBGAS Security Test: %s\n", TEST_NAME);
    printf("📊 Test ID: %s | Category: %s\n", TEST_ID, TEST_CATEGORY);
    printf("🎯 Platform: CHERI-Morello | Attack Type: Data Corruption\n");
    printf("=================================================================\n\n");
    
    printf("📖 Data-Oriented Programming (DOP) Background:\n");
    printf("   - Exploits spatial memory safety to corrupt critical data\n");
    printf("   - Targets security-critical variables rather than control flow\n");
    printf("   - Evades control-flow integrity (CFI) protections\n");
    printf("   - Can achieve privilege escalation through data manipulation\n\n");
    
    // Initialize test state
    memset(&test_state, 0, sizeof(test_state));
    
    printf("🧵 Starting multi-threaded DOP simulation...\n");
    printf("📊 Number of threads: %d\n\n", NUM_THREADS);
    
    // Create and start threads
    for (int i = 0; i < NUM_THREADS; i++) {
        test_state.contexts[i].thread_index = i;
        
        if (pthread_create(&test_state.contexts[i].thread_id, NULL, 
                          execute_dop_test, &test_state.contexts[i]) != 0) {
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
    printf("📈 Data-Oriented Programming Test Summary Report\n");
    printf("=================================================================\n");
    printf("🎯 Total DOP attempts: %d\n", test_state.total_attacks);
    printf("💥 Successful attacks: %d\n", test_state.successful_attacks);
    printf("📊 Attack success rate: %.1f%%\n", 
           test_state.total_attacks > 0 ? 
           (100.0 * test_state.successful_attacks / test_state.total_attacks) : 0.0);
    printf("📏 Total data corrupted: %zu bytes\n", test_state.total_corruption);
    
    if (test_state.successful_attacks > 0) {
        printf("🚨 VULNERABILITY STATUS: EXPLOITABLE\n");
        printf("💀 DOP attacks succeeded - data integrity compromised\n");
        printf("⚠️  System vulnerable to privilege escalation via data corruption\n");
    } else {
        printf("✅ VULNERABILITY STATUS: MITIGATED\n");
        printf("🔒 DOP attacks failed - data integrity preserved\n");
        printf("🛡️  CHERI capability system provided protection\n");
    }
    
    printf("=================================================================\n");
    printf("🔒 CHERI-Morello DOP protection evaluation complete\n");
    printf("=================================================================\n");
    
    return 0;
}

/**
 * @brief Test Summary
 * 
 * This refactored test provides comprehensive evaluation of:
 * 
 * 1. **Data-Oriented Programming Attacks**: Sophisticated data corruption techniques
 * 2. **Spatial Memory Safety**: Array bounds checking and overflow prevention
 * 3. **Data Integrity Protection**: Prevention of critical variable corruption
 * 4. **CHERI Capability System**: Spatial bounds enforcement on data access
 * 5. **Privilege Escalation**: Testing unauthorized access through data manipulation
 * 6. **Memory Layout Analysis**: Understanding attack surface and mitigation
 * 7. **Multi-threaded Security**: Concurrent DOP attack attempts
 * 8. **Real-world Relevance**: Demonstrates modern exploitation techniques
 * 
 * Expected behavior on CHERI-Morello:
 * - Array accesses should be bounded by capability limits
 * - Attempt to write beyond buffer bounds should trap
 * - Critical data integrity should be preserved
 * - DOP attack should fail with capability violation
 * 
 * On traditional systems:
 * - Buffer overflow may succeed in corrupting adjacent data
 * - Admin access flag may be modified
 * - Privilege escalation may be achieved
 * - DOP vulnerability would be exploitable
 */
