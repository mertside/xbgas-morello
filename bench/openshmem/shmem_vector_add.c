/*
 *  Explanation of Changes:
 *  1.	Parallel Vector Addition:
 *    •	Each PE operates on a portion of two vectors (local_a and local_b).
 *    •	Each PE computes the sum of its part of the vectors and stores the result in local_sum.
 *  2.	Global Result Gathering:
 *    •	The shmem_int_gather() operation collects all local portions (local_sum) from the PEs and stores them in global_sum on PE 0.
 *    •	PE 0 handles the remainder if the vector size N is not evenly divisible by the number of PEs.
 *  3.	Dynamic Memory Allocation:
 *    •	Dynamically allocate the vectors to simulate a scenario where the vector size is large and cannot be statically declared.
 *  4.	Real-world Parallelism:
 *    •	Divide large datasets into smaller chunks and process them in parallel on multiple PEs (or processors), which is demonstrated by partitioning the vector across PEs.
 *  5.	Communication:
 *    •	After computation, communication is performed using collective operations (shmem_int_gather()) to combine the results.
 * 
 *  To compile this code, use an OpenSHMEM-aware compiler like oshcc:
 *    oshcc -o shmem_vector_add shmem_vector_add.c
 *    oshrun -np 4 ./shmem_vector_add
 * 
 */

#include <shmem.h>
#include <stdio.h>
#include <stdlib.h>

#define N 1000  // Size of the vectors
#define NITER 1000  

int main(void) {
  shmem_init();  // Initialize OpenSHMEM environment

  int me = shmem_my_pe();    // Get the PE ID
  int npes = shmem_n_pes();  // Get the total number of PEs

  // Define the portion of the array each PE will handle
  int local_n = N / npes;  // Assumption: N is divisible by npes
  int remainder = N % npes;

  // Allocate memory for local parts of the vectors
  int* local_a = (int*) malloc(local_n * sizeof(int));
  int* local_b = (int*) malloc(local_n * sizeof(int));
  int* local_sum = (int*) malloc(local_n * sizeof(int));

  // Initialize local parts of the vectors with some dummy values
  for (int i = 0; i < local_n; i++) {
    local_a[i] = me * local_n + i + 1;  // Unique values for each PE
    local_b[i] = (me * local_n + i + 1) * 2;
  }

  // PE 0 will gather the results into the final vector
  int* global_sum = NULL;
  if (me == 0) {
    global_sum = (int*) shmem_malloc(N * sizeof(int));  // Allocate space for the full vector
  } // TODO: shmem malloc

  for(int i = 0; i < NITER; i++) {  
    // Perform element-wise addition of local_a and local_b
    for (int i = 0; i < local_n; i++) {
      local_sum[i] = local_a[i] + local_b[i];
    }

    // Gather all local_sum parts into the global_sum array on PE 0
    shmem_int_gather(global_sum, local_sum, local_n, 0, 0, npes, NULL);

    // Handle the remainder if N is not divisible by npes
    if (remainder != 0 && me == 0) {
      for (int i = npes * local_n; i < N; i++) {
        global_sum[i] = (i + 1) + (i + 1) * 2;
      }
    }
  } 

  // PE 0 prints the final result (for brevity, only first 10 elements)
  if (me == 0) {
    printf("Global sum vector (first 10 elements):\n");
    for (int i = 0; i < 10; i++) {
      printf("%d ", global_sum[i]);
    }
    printf("\n");
  }

  // Cleanup
  free(local_a);
  free(local_b);
  free(local_sum);
  if (me == 0) {
    free(global_sum);
  }

  shmem_finalize();  // Finalize the OpenSHMEM environment
  return 0;
}
