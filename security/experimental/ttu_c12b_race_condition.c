/*  Benchmark: Race Condition in Memory Access
 *  @author  : Mert Side for TTU
 *  @brief   : Multiple threads attempt to manipulate shared memory concurrently 
 *             without proper synchronization mechanisms such as mutexes or 
 *             semaphores.
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#define NUM_THREADS 10
#define NUM_INCREMENTS 100000

// Shared variable
int counter = 0;

// Thread function to increment the counter
void* increment_counter(void* arg) {
    for (int i = 0; i < NUM_INCREMENTS; i++) {
        counter++;  // Critical section
    }
    return NULL;
}

int main() {
    pthread_t threads[NUM_THREADS];
    int thread_args[NUM_THREADS];
    int result_code;

    // Create threads
    for (int i = 0; i < NUM_THREADS; i++) {
        thread_args[i] = i;
        result_code = pthread_create(&threads[i], NULL, increment_counter, (void*) &thread_args[i]);
        if (result_code != 0) {
            printf("Error: pthread_create returned error code %d\n", result_code);
            exit(EXIT_FAILURE);
        }
    }

    // Wait for threads to complete
    for (int i = 0; i < NUM_THREADS; i++) {
        result_code = pthread_join(threads[i], NULL);
        if (result_code != 0) {
            printf("Error: pthread_join returned error code %d\n", result_code);
            exit(EXIT_FAILURE);
        }
    }

    // Check the final value of counter
    printf("Expected counter value: %d\n", NUM_THREADS * NUM_INCREMENTS);
    printf("Actual counter value: %d\n", counter);

    return EXIT_SUCCESS;
}
