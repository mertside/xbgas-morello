/*  Benchmark: Heap Manipulation
 *   This vulnerability demonstrates a heap manipulation exploit. It involves first
 *   using an out of bounds write to make the range of one chunk extend past the range 
 *   of a formally subsequent chunk. Thus the memory range of the second chunk would be 
 *   inside the range of the first. These chunks are then feed and reallocated. Thus
 *   any edit made on the larger chunk would be legal, regardless of whether the memory
 *   written to or read from was within the space of the smaller chunk.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "xbrtime_morello.h"

// Function to simulate the heap manipulation exploit in a multi-threaded context
void* heap_manipulation_test(void* arg) {
  long tid = (long)arg;

  printf("[Thread %ld] Starting test: Heap Manipulation\n", tid);

  char* c = malloc(0x10);
  char* d = malloc(0x10);
  char* e = malloc(0x10);

  printf("[Thread %ld] c: %#p\n", tid, c);
  printf("[Thread %ld] d: %#p\n", tid, d);
  printf("[Thread %ld] e: %#p\n", tid, e);

  // Manually edit the size of d to a larger size so that it overlaps with e
  *(c + 0x18) = 0x61;

  // Free the chunks to prepare them for reallocation
  free(d);
  free(e);

  // Allocate new chunks with manipulated sizes
  char* g = malloc(0x50);
  char* h = malloc(0x10);

  // If the exploit succeeded, then d and g will be the same, otherwise d and h will be the same
  memcpy(h, "victim's data", 0xe);  // h copies in some data needed for program control

  // This position is still within the legal memory range of g but overlaps with h
  memset(g + 0x20, 0x41, 0xf);

  printf("[Thread %ld] g: %#p -> %#p\n", tid, g, (g + 0x50));
  printf("[Thread %ld] h: %#p\n", tid, h);
  printf("[Thread %ld] h: %s\n", tid, h);

  // Check if the memory overlap was exploited
  if (h[0] == 'A') {
    printf("[Thread %ld] Test Failed: Heap manipulation leading to overlapping memory regions\n", tid);
  }

  return NULL;
}

int main() {
  xbrtime_init();

  int num_pes = xbrtime_num_pes();

  printf("Starting multi-threaded test: Heap Manipulation\n");

  // Add work to each thread in the thread pool
  for (long i = 0; i < num_pes; i++) {
    tpool_add_work(threads[i].thread_queue, heap_manipulation_test, (void*)i);
  }

  // Wait for all threads to complete their work
  for (int i = 0; i < num_pes; i++) {
    tpool_wait(threads[i].thread_queue);
  }

  printf("Completed multi-threaded test: Heap Manipulation:           ??? !\n");

  xbrtime_close();

  return 0;
}
