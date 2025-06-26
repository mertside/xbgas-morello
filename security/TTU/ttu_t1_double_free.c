/*
 * Based on Double Free by STAM
 * Adapted by Mert Side for Texas Tech University
 *
 * Key Notes:
 * 
 * The code creates a shared resource that includes dynamically allocated memory
 *  and a mutex for synchronization. Each thread attempts to free the shared 
 *  memory. The mutex ensures that only one thread can access the shared memory 
 *  at a time, and setting the pointer to NULL after freeing it helps prevent 
 *  the second thread from attempting a double-free.
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "xbrtime_morello.h"

#define BUFFER_SIZE 85

// Function to simulate the double-free vulnerability in a multi-threaded context
void* double_free_test(void* arg) {
  long tid = (long)arg;

  printf("[Thread %ld] Starting Test: Double Free\n", tid);
  char* complete = malloc(sizeof(char) * BUFFER_SIZE);	
  strcpy(complete, "Hello World! Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod");

  printf("[Thread %ld] Freeing memory(1)\n", tid);
  free(complete);
  printf("[Thread %ld] Freeing memory(2)\n", tid);
  free(complete);

  printf("[Thread %ld] Test Failed: Double Free\n\n", tid);

  return NULL;
}

int main() {
  xbrtime_init();

  int num_pes = xbrtime_num_pes();

  printf("Starting multi-threaded test: Double Free\n");

  // Add work to each thread in the thread pool
  for (long i = 0; i < num_pes; i++) {
      tpool_add_work(threads[i].thread_queue, double_free_test, (void*)i);
  }

  // Wait for all threads to complete their work
  // for (int i = 0; i < num_pes; i++) {
  //     tpool_wait(threads[i].thread_queue);
  // }

  printf("Completed multi-threaded test: Double Free:            EXPLOITED!\n");

  xbrtime_close();

  return 0;
}
