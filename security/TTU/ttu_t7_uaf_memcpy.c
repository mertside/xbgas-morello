/*
 * Benchmark: Use After Free using memcpy
 * Adapted for xBGAS by Mert Side for Texas Tech University
 * 
 * Key Notes:
 * This program demonstrates a use-after-free (UAF) vulnerability where a 
 * memory region is reallocated, and the original pointer is still used to 
 * modify the data in the new allocation. This version is adapted to run in a 
 * multi-threaded xBGAS environment on Morello.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "xbrtime_morello.h"

// Function to simulate the UAF vulnerability using memcpy
void* uaf_memcpy_vulnerability(void* arg) {
  long tid = (long)arg;
  printf("[Thread %ld] Starting test: Use After Free using memcpy\n", tid);

  // Allocate memory for a character array
  char* a = (char *)malloc(0x10);

  // Copy a string into the allocated memory
  memcpy(a, "hello!", 0x10);
  printf("[Thread %ld] a contains: %s\n\n", tid, a);

  printf("[Thread %ld] a: %p\n", tid, (void *)a);

  // Free the allocated memory
  free(a);

  // Allocate memory again, which might reuse the same memory location
  char* b = (char *)malloc(10);
  printf("[Thread %ld] b: %p\n\n", tid, (void *)b);

  // Use the original pointer a, which has been freed, with memcpy
  // This causes the data in the reallocated memory (pointed by b) to be modified
  memcpy(a, "RANDOMSTRING", 0x10);

  printf("[Thread %ld] a contains: %s\n\n", tid, a);
  printf("[Thread %ld] b contains: %s\n\n", tid, b);

  if (a[0] == 'R')
    printf("[Thread %ld] Test Failed: Use After Free using memcpy\n", tid);

  // Free the second allocation
  free(b);

  return NULL;
}

int main() {
  xbrtime_init();
  int num_pes = xbrtime_num_pes();

  printf("Starting multi-threaded test: Use After Free using memcpy\n");

  // Add work to each thread in the thread pool
  for (long i = 0; i < num_pes; i++) {
    tpool_add_work(threads[i].thread_queue, uaf_memcpy_vulnerability, (void*)i);
  }

  // Wait for all threads to complete their work
  for (int i = 0; i < num_pes; i++) {
    tpool_wait(threads[i].thread_queue);
  }

  printf("Completed multi-threaded test: Use After Free using memcpy\n");

  xbrtime_close();

  return 0;
}
