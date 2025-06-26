/*  Benchmark: Out-of-Bounds Read
 *  @author  : Mert Side for TTU
 *  @brief   : This benchmark is a simple test for an out-of-bounds read.
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "xbrtime_morello.h"

#define PUBLIC_SIZE 6
#define PRIVATE_SIZE 14

void* out_of_bounds_read(void* arg) {
  int test_status = 1;

  // Allocate separate buffers for 'public' and 'private' data
  char* public  = (char *) malloc(PUBLIC_SIZE);
  if (public == NULL) {
    printf("Thread %ld: Memory allocation failed\n", (long)arg);
    return NULL;
  }
  char* private = (char *) malloc(PRIVATE_SIZE);
  if (private == NULL) {
    printf("Thread %ld: Memory allocation failed\n", (long)arg);
    return NULL;
  }

  // Initialize the 'public' and 'private' segments
  strcpy(public, "public");
  strcpy(private, "secretpassword");

  // Print the starting addresses of the 'public' and 'private' segments
  printf("  [%ld] Address of public array:          %p\n", 
          (long)arg, (void *)public);
  printf("    [%ld] Full capability (inc. meta.):   %#p\n", 
          (long)arg, (void *)public);
  printf("  [%ld] Address of private array:         %p\n", 
          (long)arg, (void *)private);
  printf("    [%ld] Full capability (inc. meta.):   %#p\n", 
          (long)arg, (void *)private);

  // Calculate the offset between the 'public' and 'private' segments
  __intptr_t offset = private - public;
  printf("  [%ld] Offset of private to public: %p\n", 
          (long)arg, (void *)offset);

  // Simulate a safe print of the 'public' data
  printf("Printing characters of public array\n");
  for (int i = 0; i < PUBLIC_SIZE; i++) {
    printf("%c", public[i]);
  }
  printf("\n");

  // Simulate an out-of-bounds read from the 'public' to access 'private' data
  printf("Printing characters of private array from public array\n");
  for (int i = 0; i < PRIVATE_SIZE; i++) {
    printf("%c", public[i + offset]);                           // OOB read here

    // Check if the OOB read matches the corresponding 'private' data
    if (public[i + offset] == private[i]) {
      test_status = 0;
    }
  }
  printf("\n");

  // Free the allocated memory
  free(public);
  free(private); 

  // Print the test result
  if(test_status == 0)
    printf("[%ld] Test: Out-of-Bounds Read: EXPLOITED!\n", (long)arg);
  else
    printf("[%ld] Test: Out-of-Bounds Read: Mitigated!\n", (long)arg);
  
  return NULL;
}

int main() {
  xbrtime_init();

  int num_pes = xbrtime_num_pes();

  printf("Starting test: Out of Bounds Read\n");

  for (long i = 0; i < num_pes; i++) {
    bool check = false;
    check = tpool_add_work(threads[i].thread_queue,
                            out_of_bounds_read,
                            (void*)i);
    if (!check) {
      printf("Thread %ld: Failed to add work\n", i);
    }
  }

  printf("Completed: Out-of-Bounds Read:                         EXPLOITED!\n");

  xbrtime_close();

  return 0;
}
