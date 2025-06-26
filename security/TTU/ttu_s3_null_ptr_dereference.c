/*
 * Based on Null Pointer Dereference not on Heap by STAM
 * Adapted by Mert Side for Texas Tech University
 *
 * Key Notes:
 *   This code creates a set of threads where each thread performs a specific 
 *     operation based on its PE ID. Specifically, one thread (PE 0) prints the 
 *     hexadecimal characters of a known string, while another thread (PE 1) 
 *     attempts to dereference a NULL pointer.
 * 
 * Understanding the Error "Illegal instruction (core dumped)" on Morello:
 *   "Illegal instruction (core dumped)":
 *     On platforms like Morello (which implements the CHERI capability model), 
 *     dereferencing a NULL pointer is considered an illegal operation because 
 *     NULL is not a valid memory address. When the CPU tries to execute this 
 *     operation, it triggers an exception, leading to the "Illegal instruction" 
 *     error and the program being terminated (core dumped).
 *   CHERI and Morello:
 *      On CHERI-enabled architectures like Morello, pointers are capabilities 
 *      that include metadata like bounds and permissions. When bad_ptr is set 
 *      to NULL, it becomes a capability with no valid bounds or permissions. 
 *      Trying to dereference it results in an exception because it violates 
 *      the safety guarantees enforced by the CHERI architecture.
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include "xbrtime_morello.h"

int abort_flag = 0;

// Function to simulate the null pointer dereference vulnerability
void* null_pointer_dereference(void* arg) {
  long tid = (long)arg;
  printf("[Thread %ld] Starting test: Null pointer dereference\n", tid);

  char *complete = "Hello World!";
  int sizeOfComplete = sizeof(complete);
  printf("  [Thread %ld] Address of string:            %p\n", 
          tid, (void *)complete);
  printf("  [Thread %ld] Size of string:               %d\n", 
          tid, sizeOfComplete);
  printf("  [Thread %ld] Full capability (inc. meta.): %#p\n", 
          tid, (void *)complete);

  char *bad_ptr;
  printf("  [Thread %ld] Address of bad pointer:         %p\n", 
          tid, (void *)bad_ptr);
  printf("  [Thread %ld] Full capability (inc. meta.):   %#p\n", 
          tid, (void *)bad_ptr);

  // Assign known string to bad_ptr
  bad_ptr = complete;

  printf("[Thread %ld] Printing hex characters of known string:\n", tid);
  for (int i = 0; i < sizeOfComplete; i++) {
    printf("%x", bad_ptr[i]);
  }
  printf("\n");

  // Set pointer to NULL and attempt to dereference it
  bad_ptr = NULL;

  printf("[Thread %ld] Attempting to dereference NULL pointer:\n", tid);
  for (int i = 0; i < sizeOfComplete; i++) {
    printf("%x", bad_ptr[i]);  // This should trigger an error
  }
  printf("\n");

  printf("[Thread %ld] EXPLOITED! (message may not appear if crashed).\n", 
          tid);

  return NULL;
}

int main() {
  if (abort_flag > 0) {
    printf("Abort Reached: Mitigated!\n");
    abort();  // Force the program to terminate
  }

  xbrtime_init();
  int num_pes = xbrtime_num_pes();

  printf("Starting multi-threaded test: Null pointer dereference\n");

  // Add work to each thread in the thread pool
  for (long i = 0; i < num_pes; i++) {
    abort_flag++;
    tpool_add_work(threads[i].thread_queue, null_pointer_dereference, (void*)i);
  }

  // Wait for all threads to complete their work
  for (int i = 0; i < num_pes; i++) {
    tpool_wait(threads[i].thread_queue);
  }

  printf("Completed test: Null pointer dereference:              EXPLOITED!\n");

  xbrtime_close();

  return 0;
}
