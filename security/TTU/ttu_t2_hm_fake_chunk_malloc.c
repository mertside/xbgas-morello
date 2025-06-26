/*  Benchmark: Heap Manipulation
 *  @author  : Adapted by Mert Side for Texas Tech University
 *    
 * This vulnerability demonstrates a heap allocation exploit. It involves freeing   
 * chunks onto the tcache and then performing a use-after-free to modify chunk address.
 * Thus, a chunk is artificially added to the tcache and can be allocated during a malloc
 */

#include <stdio.h>
#include <stdlib.h>
#include "xbrtime_morello.h"

// Function to perform heap manipulation in a multi-threaded context
void* heap_manipulation(void* arg) {
  long tid = (long)arg;

  printf("[Thread %ld] Starting test: Heap Manipulation\n", tid);

  unsigned long int *ptr0, *ptr1, *target;

  ptr0 = malloc(0x10);
  ptr1 = malloc(0x10);
  target = malloc(0x10);

  printf("  [%ld] ptr0: %p\n", tid, ptr0);
  printf("  [%ld] ptr1: %p\n", tid, ptr1);
  printf("  [%ld] Target: %p\n\n", tid, target);

  free(ptr0);
  free(ptr1);

  // Address of previous chunk is stored in ptr1. Increment by 64 to change this value to 
  // the address of the target
  *ptr1 = *ptr1 + 64; 
  // *ptr1 = *ptr1 - 64; //Use this line if previous line returns an address different from target 

  unsigned long int *ptr_x, *ptr_y;

  ptr_x = malloc(0x10);
  ptr_y = malloc(0x10);

  printf("  [%ld] ptr_x: %p\n", tid, ptr_x);
  printf("  [%ld] ptr_y: %p\n", tid, ptr_y); // Value would be same as target

  if(ptr_y == target)
    printf("[Thread %ld] Test Failed: Heap manipulation leading to allocation on specific address\n", tid);
  else
    printf("[Thread %ld] Test Passed: Heap manipulation did not allocate on specific address\n", tid);

  return NULL;
}

int main() {
  xbrtime_init();

  int num_pes = xbrtime_num_pes();

  printf("Starting multi-threaded test: Heap Manipulation\n");

  // Add work to each thread in the thread pool
  for (long i = 0; i < num_pes; i++) {
    tpool_add_work(threads[i].thread_queue, heap_manipulation, (void*)i);
  }

  // Wait for all threads to complete their work
  for (int i = 0; i < num_pes; i++) {
    tpool_wait(threads[i].thread_queue);
  }

  printf("Completed multi-threaded test: Heap Manipulation            ??? !\n");

  xbrtime_close();

  return 0;
}
