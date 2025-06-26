/*  Benchmark: Memory Leak Simulation
 *  @author  : Mert Side for TTU
 *  @brief   : To demonstrate a memory leak in C, you can create a scenario 
 *             where dynamically allocated memory is not properly released back 
 *             to the system, causing the application to consume increasing 
 *             amounts of memory over time. This can lead to system resource 
 *             exhaustion and potential failure or slowdown of the system.
 * 
 * Explanation of the Code
 * - Function create_memory_leak():
 *    This function allocates 1 KB of memory using malloc and does not release 
 *    it (i.e., it does not call free()). This results in memory being allocated 
 *    but never reclaimed.
 *    Optionally, the function writes to the allocated memory to simulate 
 *    working with data, which is a typical use case in applications.
 * - Repeated Leaks:
 *    The main() function calls create_memory_leak() repeatedly in a loop 
 *    (1000 times in this example), causing the program to leak significant 
 *    amounts of memory. It prints a message every 100 iterations to indicate 
 *    ongoing leaking.
 * - Resource Monitoring:
 *    After the loop, a message prompts to check the system’s resource monitor 
 *    to observe the impact of the memory leak on system resources.
 * 
 */

#include <stdio.h>
#include <stdlib.h>

void create_memory_leak() {
    // Allocate memory and do not free it
    char *leaked_memory = malloc(1024); // Allocate 1 KB
    if (leaked_memory == NULL) {
        fprintf(stderr, "Failed to allocate memory.\n");
        exit(EXIT_FAILURE);
    }
    
    // Optionally simulate usage of memory
    strcpy(leaked_memory, "This is a memory leak example.");
    printf("Data in leaked memory: %s\n", leaked_memory);
    
    // Intentionally omitting free(leaked_memory);
}

int main() {
    for (int i = 0; i < 1000; i++) {
        create_memory_leak();
        if (i % 100 == 0) {
            printf("Leaked 100 times\n");
        }
    }
    
    printf("Memory has been leaked. Check the system's resource monitor.\n");
    return EXIT_SUCCESS;
}
