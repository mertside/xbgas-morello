/*
 * Benchmark: Use After Free on function pointer leads to code reuse attack
 * Adapted for xBGAS by Mert Side for Texas Tech University
 * 
 * Key Notes:
 * This program demonstrates a use-after-free (UAF) vulnerability where a 
 * function pointer within a freed structure is reused after the memory has been 
 * reallocated for a different purpose. This version is adapted to run in a 
 * multi-threaded xBGAS environment on Morello.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "xbrtime_morello.h"

// Structure definition with a function pointer and username buffer
struct s_user {
  void (*func_ptr)(void);
  char username[8];
};

int count = 0;

// Function to print the count value
void print_count() {
  printf("Count: %d\n", count);
}

// Function that represents the "gadget" code
void gadget(void) {
  printf("Test Failed: UAF to Code Reuse\n\n");
  exit(0);
}

// Simulates the UAF vulnerability
void* uaf_vulnerability(void* arg) {
  long tid = (long)arg;
  printf("[Thread %ld] Starting test: UAF to Code Reuse\n", tid);

  struct s_user *user = (struct s_user *)malloc(sizeof(struct s_user));
  count++;

  user->func_ptr = &print_count;
  strcpy(user->username, "Hedwig");

  user->func_ptr();

  // Free the struct object
  free(user);
  count--;

  // Realloc same buffer to long value
  unsigned long *value = (unsigned long *)malloc(sizeof(unsigned long));
  
  // Simulated user input: Address to code segment to be reused
  *value = (unsigned long)&gadget;

  // Use after free
  user->func_ptr();

  printf("[Thread %ld] Test Complete: UAF to Code Reuse\n\n", tid);

  return NULL;
}

int main() {
  xbrtime_init();
  int num_pes = xbrtime_num_pes();

  printf("Starting multi-threaded test: UAF to Code Reuse\n");

  // Add work to each thread in the thread pool
  for (long i = 0; i < num_pes; i++) {
    tpool_add_work(threads[i].thread_queue, uaf_vulnerability, (void*)i);
  }

  // Wait for all threads to complete their work
  for (int i = 0; i < num_pes; i++) {
    tpool_wait(threads[i].thread_queue);
  }

  printf("Completed multi-threaded test: UAF to Code Reuse:      EXPLOITED!\n");

  xbrtime_close();

  return 0;
}
