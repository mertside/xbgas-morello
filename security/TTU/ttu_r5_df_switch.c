/*
 * Benchmark: Double Free on Switch Fallthrough
 * Adapted for xBGAS by Mert Side for Texas Tech University
 * 
 * Key Notes:
 * This program demonstrates a double-free vulnerability caused by a switch-case 
 * fallthrough, where the same memory is freed multiple times. This version is 
 * adapted to run in a multi-threaded xBGAS environment on Morello.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "xbrtime_morello.h"

// Function to simulate the double-free vulnerability
void* double_free_vulnerability(void* arg) {
  long tid = (long)arg;
  printf("[Thread %ld] Starting test: Double Free\n", tid);

  char* a = malloc(0x10);

  *a = 'C';

  // Switch case with fallthrough leading to multiple free operations
  switch (*a) {
    case 'A':
      printf("[Thread %ld] Char is %s\n", tid, a);
      free(a);
      // No break included. Default statement runs
    case 'B':
      printf("[Thread %ld] Char is %s\n", tid, a);
      free(a);
    case 'C':
      printf("[Thread %ld] Char is %s\n", tid, a);
      free(a);
    default:
      memcpy(a, "DEFAULT", 0X10);
      printf("[Thread %ld] Char is %s\n", tid, a);
      free(a);
  }

  char* b = malloc(0x10);
  char* c = malloc(0x10);

  printf("[Thread %ld] a: %#p\n", tid, (void*)a);
  printf("[Thread %ld] b: %#p\n", tid, (void*)b);
  printf("[Thread %ld] c: %#p\n", tid, (void*)c);

  // printf("[Thread %ld] capabilites of a: %d\n", tid, xbrtime_get_cap(a));
  // printf("[Thread %ld] capabilites of b: %d\n", tid, xbrtime_get_cap(b));
  // printf("[Thread %ld] capabilites of c: %d\n", tid, xbrtime_get_cap(c));

  if (b == c) {
    printf("[Thread %ld] Test Failed: Switch fallthrough with metadata overwrite leading to Double Free\n", tid);
  } else {
    printf("[%ld] Test Passed: overwrite prevented (may not reach)\n", tid);
  }

  return NULL;
}

int main() {
  xbrtime_init();
  int num_pes = xbrtime_num_pes();

  printf("Starting multi-threaded test: Double Free\n");

  // Add work to each thread in the thread pool
  for (long i = 0; i < num_pes; i++) {
    tpool_add_work(threads[i].thread_queue, 
                    double_free_vulnerability, 
                    (void*)i);
  }

  // Wait for all threads to complete their work
  for (int i = 0; i < num_pes; i++) {
    tpool_wait(threads[i].thread_queue);
  }

  printf("Completed multi-threaded test: Double Free:            EXPLOITED!\n");

  xbrtime_close();

  return 0;
}
