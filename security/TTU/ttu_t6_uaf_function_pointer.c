/*
 * Benchmark: Use After Free on Function Pointer
 * Adapted for xBGAS by Mert Side for Texas Tech University
 * 
 * Key Notes:
 * This program demonstrates a use-after-free (UAF) vulnerability where a 
 * function pointer is used after it has been freed and then reallocated.
 * This version is adapted to run in a multi-threaded xBGAS environment on 
 * Morello.
 */

#include <stdio.h>
#include <stdlib.h>
#include "xbrtime_morello.h"

typedef void (*fp)();
void default_function();
void target_function();

// Function to simulate the UAF vulnerability
void* uaf_vulnerability(void* arg) {
  long tid = (long)arg;
  printf("[Thread %ld] Starting test: Use After Free on Function Pointer\n", tid);

  // Allocate memory for the function pointer
  fp *default_func_pointer = (fp *)malloc(sizeof(fp));

  // Assign the function pointer to point to the default function
  *default_func_pointer = default_function;
  (*default_func_pointer)();

  // Free the function pointer
  free(default_func_pointer);

  // Reallocate memory for the function pointer
  fp *target_func_pointer = (fp *)malloc(sizeof(fp));
  *target_func_pointer = target_function;

  printf("[Thread %ld] Using default pointer after free\n", tid);

  // Use the function pointer after it has been freed (UAF)
  (*default_func_pointer)(); // This should ideally cause a problem

  // Free the target function pointer memory
  free(target_func_pointer);

  return NULL;
}

int main() {
  xbrtime_init();
  int num_pes = xbrtime_num_pes();

  printf("Starting multi-threaded test: Use After Free on Function Pointer\n");

  // Add work to each thread in the thread pool
  for (long i = 0; i < num_pes; i++) {
    tpool_add_work(threads[i].thread_queue, uaf_vulnerability, (void*)i);
  }

  // Wait for all threads to complete their work
  for (int i = 0; i < num_pes; i++) {
    tpool_wait(threads[i].thread_queue);
  }

  printf("Completed multi-threaded test: Use After Free on Function Pointer\n");

  xbrtime_close();

  return 0;
}

// Default function that is initially called
void default_function(){
  printf("Public data\n");
}

// Target function that the UAF attempts to switch control to
void target_function(){
  printf("Test Failed: Use After Free for function pointer\n");
}
