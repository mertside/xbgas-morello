/*
 * Key Points:
 * •	Computation Phase: Each PE calculates the square of its own PE number.
 * •	Communication Phase: The shmem_int_sum_to_all operation sums the local 
 * square results from all PEs.
 * •	shmem_init() / shmem_finalize(): Initializes and finalizes the OpenSHMEM 
 * environment.
 * •	shmem_int_sum_to_all: Reduces the local square values from all PEs to a 
 * global sum (result available on all PEs, but printed only on PE 0).
 * 
 *  To compile this code, use an OpenSHMEM-aware compiler like oshcc:
 *    oshcc -o shmem_sum_example shmem_sum_example.c
 *  And run it with the desired number of PEs:
 *    oshrun -np 4 ./shmem_sum_example
 * 
 */

#include <shmem.h>
#include <stdio.h>

#define NITER 1000  

int main(void) {
  shmem_init();  // Initialize the OpenSHMEM environment

  int me = shmem_my_pe();  // Get the ID of this PE
  int npes = shmem_n_pes();  // Get the total number of PEs

  for(int i = 0; i < NITER; i++) {
    // Computation Phase: Calculate the square of the PE's ID
    int local_square = me * me;

    // Communication Phase: Sum all local_square values across all PEs
    int total_sum = 0; // TODO: check if stack var is ok?
    shmem_int_sum_to_all(&total_sum, &local_square, 1, 0, 0, npes, NULL, NULL);
  }

  // Print the result from PE 0
  if (me == 0) {
    printf("Total sum of squares from all PEs: %d\n", total_sum);
  }

  shmem_finalize();  // Finalize the OpenSHMEM environment
  return 0;
}