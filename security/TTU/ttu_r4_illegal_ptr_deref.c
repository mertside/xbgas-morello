/*
 * Benchmark: Illegal Pointer Dereference on Large Size Allocation 
 * Adapted for xBGAS by Mert Side for Texas Tech University
 * 
 * Key Notes:
 * This program demonstrates an illegal pointer dereference caused by 
 * an attempt to allocate an extremely large amount of memory.
 * This version is adapted to run in a multi-threaded xBGAS environment on 
 * Morello.
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include "xbrtime_morello.h"

#include <stdint.h>

#define SIZE 0x40000000000 // Larger than the max size for malloc

// Function to simulate the illegal pointer dereference vulnerability
void* illegal_pointer_dereference(void* arg) {
  long tid = (long)arg;
  printf("[Thread %ld] Starting test: Illegal Pointer Dereference\n", tid);
  
  // printf("[Thread %ld] Attempting to allocate %ld bytes of memory\n", tid, SIZE);
  // int* c = malloc(SIZE);

  // Uninitialized pointer
  int* c; 
  // Allocate a size larger than the max size for malloc
  c = malloc(PTRDIFF_MAX+1); 

  printf("  [Thread %ld] Address with cap. of c:   %#p\n", tid, (void *)c);

  // Check if malloc failed
  // if (*c == 0) { 
  if (c == NULL) {
    printf("[Thread %ld] Malloc failed: Could not allocate the requested memory size.\n", 
            tid);
    return NULL;
  } else {
    printf("[Thread %ld] Malloc succeeded: Allocated %ld bytes of memory\n", 
            tid, PTRDIFF_MAX+1);
  }

  printf("  [Thread %ld] Address of c:             %p\n", tid, c);
  printf("  [Thread %ld] Address with cap. of c:   %#p\n", tid, (void *)c);
  printf("  [Thread %ld] Value of c:               %d\n", tid, *c);

  // Free allocated memory if malloc succeeded
  free(c);

  return NULL;
}

int main() {
  xbrtime_init();
  int num_pes = xbrtime_num_pes();

  printf("Starting multi-threaded test: Illegal Pointer Dereference\n");
  printf("  PTRDIFF_MAX: %ld\n", PTRDIFF_MAX);

  // Add work to each thread in the thread pool
  for (long i = 0; i < num_pes; i++) {
    tpool_add_work(threads[i].thread_queue, 
                   illegal_pointer_dereference, 
                   (void*)i);
  }

  // Wait for all threads to complete their work
  for (int i = 0; i < num_pes; i++) {
    tpool_wait(threads[i].thread_queue);
  }

  printf("Completed test: Illegal Pointer Dereference:                     \n");

  xbrtime_close();

  return 0;
}
