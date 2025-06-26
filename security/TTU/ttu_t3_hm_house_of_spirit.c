/*  Benchmark: Heap Manipulation
 *    This vulnerability demonstrates a heap manipulation exploit. It involves artificially
 *    creating a fake chunk. The address of this fake chunk is then assigned to a chunk on 
 *    the heap. When the heap chunk is freed, the fake chunk address get placed onto the tcache.
 *    As a result, a subsequent call to malloc would allocate the artificially created chunk
 */

// https://heap-exploitation.dhavalkapil.com/attacks/house_of_spirit

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "xbrtime_morello.h"

// Define the structure for the heap chunks
struct fast_chunk {
  size_t prev_size;
  size_t size;
  struct fast_chunk *fd;
  struct fast_chunk *bk;
  char buf[0x20];  // chunk falls in fastbin size range
};

// Global variables for the chunks and pointers
struct fast_chunk fake_chunks[2];  // Two chunks in consecutive memory
void *ptr, *victim;

// Function to simulate the heap manipulation exploit in a multi-threaded context
void* heap_manipulation_test(void* arg) {
  long tid = (long)arg;

  printf("[Thread %ld] Starting test: Heap Manipulation\n", tid);

  ptr = malloc(0x30);  // First malloc
  printf("[Thread %ld] ptr: %p\n", tid, ptr);

  void *orig_ptr;
  orig_ptr = ptr;

  // Set up the fake chunks
  fake_chunks[0].size = sizeof(struct fast_chunk);  // 0x40
  fake_chunks[1].size = sizeof(struct fast_chunk);  // 0x40

  // Overwrite pointer to simulate attack
  ptr = (void *)&fake_chunks[0].fd;
  printf("[Thread %ld] Overwritten ptr: %p\n", tid, ptr);

  // Free the fake chunk, placing it into the fastbin
  free(ptr);

  // Allocate memory again, expecting it to return the address of the fake chunk
  victim = malloc(0x30);
  printf("[Thread %ld] victim: %p\n", tid, victim);

  // Check if the victim pointer matches the original pointer
  if (victim != orig_ptr)
    printf("[Thread %ld] Test Failed: Heap manipulation leading to arbitrary memory allocation\n", tid);

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
