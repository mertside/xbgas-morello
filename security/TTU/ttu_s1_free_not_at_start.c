/*
 * Based on Free Not at Start of Buffer by STAM
 * Adapted by Mert Side for Texas Tech University
 *
 * Key Notes:
 * 
 * The code demonstrates creating multiple threads (PEs) 
 *  to manipulate a shared buffer, simulating an environment where memory is 
 *  shared across processing elements. 
 * 
 * It attempts an unsafe free operation on an offset from the allocated memory 
 *  start, illustrating the kind of mistake that could lead to vulnerabilities. 
 *
 * 
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include "xbrtime_morello.h"

// #define NUM_THREADS 2
#define BUFFER_SIZE 85

// Thread function to manipulate the buffer
void *thread_function(void *arg) {
  char **complete_ptr = (char **)arg;
  if(xbrtime_mype() == 0) { // First PE: manipulate the string
    strcpy(*complete_ptr, "Hello World! Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod");
    printf("PE %d: Printing characters of string before free:\n", xbrtime_mype());
    for(int i=0; i<BUFFER_SIZE; i++) {
      printf("%c", (*complete_ptr)[i]);
    }
    printf("\n");
    // Attempt to free an offset into the allocated memory (unsafe operation)
    free(*complete_ptr + 8); // This is the operation that simulates the vulnerability
  } else { // Second PE: attempt to access after the problematic free
    printf("PE %d: Printing characters of string after free attempt:\n", xbrtime_mype());
    for(int i=0; i<BUFFER_SIZE; i++) {
      printf("%c", (*complete_ptr)[i]);
    }
    printf("\n");
  }
  return NULL;
}

int main() {
  xbrtime_init();

  int num_pes = xbrtime_num_pes();

  printf("Starting Test: Free not at start\n");

  char *complete = malloc(sizeof(char) * BUFFER_SIZE); 
  // pthread_t threads[NUM_THREADS];

  for( int i = 0; i < num_pes; i++ ){
    bool check = false;
    check = tpool_add_work( threads[i].thread_queue, 
                            thread_function, 
                            &complete);
  }

  printf("Test Ended: Free not at start:                         EXPLOITED!\n");

  xbrtime_close();
  return 0;
}
