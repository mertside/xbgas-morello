/*
 * Based on Use-After-Free by STAM
 * Adapted by Mert Side for Texas Tech University
 *
 * Key Notes:
 *   This code demonstrates a Use-After-Free vulnerability within a multi-threaded
 *   environment using the xbrtime_morello library. Multiple threads will perform
 *   operations on a memory region after it has been freed, potentially leading to
 *   undefined behavior.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "xbrtime_morello.h"

#define BUFFER_SIZE 85

// Function to simulate the use-after-free vulnerability in a multi-threaded context
void* use_after_free_test(void* arg) {
  long tid = (long)arg;

  printf("[Thread %ld] Starting test: Use-After-Free\n", tid);

  // Allocate memory and initialize it with a string
  char *complete = malloc(sizeof(char) * BUFFER_SIZE);
  strcpy(complete, "Hello World! Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod");

  // Print the characters of the string before freeing the memory
  printf("[Thread %ld] Printing characters of string before free:\n", tid);
  for (int i = 0; i < BUFFER_SIZE; i++) {
    printf("%c", complete[i]);
  }
  printf("\n");

  // Free the allocated memory
  free(complete);

  // Attempt to print the characters of the string after freeing the memory
  printf("[Thread %ld] Printing characters of string after free:\n", tid);
  for (int i = 0; i < BUFFER_SIZE; i++) {
    printf("%c", complete[i]);  // Use-after-free occurs here
  }
  printf("\n");

  // If the program reaches this point, the use-after-free vulnerability was exploited
  printf("[Thread %ld] Test Failed: Use-After-Free\n", tid);

  return NULL;
}

int main() {
  xbrtime_init();

  int num_pes = xbrtime_num_pes();

  printf("Starting multi-threaded test: Use-After-Free\n");

  // Add work to each thread in the thread pool
  for (long i = 0; i < num_pes; i++) {
    tpool_add_work(threads[i].thread_queue, use_after_free_test, (void*)i);
  }

  // Wait for all threads to complete their work
  for (int i = 0; i < num_pes; i++) {
    tpool_wait(threads[i].thread_queue);
  }

  printf("Completed multi-threaded test: Use-After-Free:         EXPLOITED!\n");

  xbrtime_close();

  return 0;
}
